#include <jp/fs.h>
#include <jp/debug.h>
#include <jp/string.h>
#include <jp/stat.h>
#include <jp/syscall.h>
#include <jp/stdlib.h>

#define INODE_MAX 64
// @todo alloc inode table from kernel memory
static inode_t inode_table[INODE_MAX];

inode_t *get_root_inode(void)
{
    return &inode_table[0]; // 0 is always root
}

static inode_t* inode_get_free(void)
{
    for(size_t i = 0; i < INODE_MAX; ++i) {
        inode_t *node = &inode_table[i];
        if (node->dev == DEV_NULL) {
            return node;
        }
    }
    panic("no inodes avalable");
}

static inode_t* inode_get_from_list(super_block_t *sb, int nr)
{
    inode_t *pos;
    list_for_each(pos, &sb->inode_list, node) {
        if (pos->nr == nr) {
            return pos;
        }
    }
    return NULL;
}

static inline int inode_block(super_block_t *sb, int nr)
{
// boot | superblock | imap| zmap| inodes
    return 2 + sb->desc->imap_blocks + sb->desc->zmap_blocks + ((nr-1)/BLOCK_INODES);
}

inode_t *iget(dev_t dev, int nr)
{
    super_block_t *sb = read_super(dev);
    assert(sb);
    inode_t *node = inode_get_from_list(sb, nr);
    if (node) {
        node->count++;
        return node;
    }
    node = inode_get_free();
    node->dev = dev;
    node->nr = nr;
    node->count = 1;

    list_add(&sb->inode_list, &node->node);

    buffer_t *buf = bread(dev, inode_block(sb, nr));
    node->buf = buf;

    node->desc = &(((inode_desc_t*)buf->data)[(nr-1)%BLOCK_INODES]);
    node->ctime = node->desc->mtime;
    node->atime = time();
    return node;
}

static void put_free_inode(inode_t *node)
{
    memset(node, 0, sizeof(*node));
    node_init(&node->node);
    node->dev = DEV_NULL;
}

void iput(inode_t *inode)
{
    assert(inode && inode->count > 0);
    inode->count--;
    if (inode->buf->dirty) {
        bwrite(inode->buf);
    }
    if (inode->count)
        return;
    brelease(inode->buf);
    list_del(&inode->node);
    put_free_inode(inode);
}

void inode_init(void)
{
    for (int i = 0; i < INODE_MAX; ++i) {
        put_free_inode(&inode_table[i]);
    }
}

int inode_read(inode_t *inode, char *buf, int len, int offset)
{
    assert(ISFILE(inode->desc->mode) || ISDIR(inode->desc->mode));
    if (inode->desc->size <= offset) {
        return EOF;
    }
    int begin = offset;
    int left = MIN(len, inode->desc->size - offset);
    int readn = 0;
    while (0 < left) {
        int nr = bmap(inode, offset / BLOCK_SIZE, false);
        if (!nr) {
            goto out;
        }
        buffer_t *bf = bread(inode->dev, nr);
        assert(bf);
        int start = offset % BLOCK_SIZE;
        int bf_read_n = MIN(left, BLOCK_SIZE - start);
        memcpy(buf+readn, (char*)bf->data + start, bf_read_n);
        left -= bf_read_n;
        offset += bf_read_n;
        readn += bf_read_n;
        brelease(bf);
    }
out:
    inode->ctime = time(); // write to disk?
    return readn;
}


int inode_write(inode_t *inode, const char *buf, int len, int offset)
{
    assert(ISFILE(inode->desc->mode)); // must be file to write. there is other method for dir
    int write_n = 0;
    while (len > 0) {
        int nr = bmap(inode, offset / BLOCK_SIZE, true);
        if (!nr) {
            goto out;
        }
        buffer_t *bf = bread(inode->dev, nr);
        assert(bf);
        int start = offset % BLOCK_SIZE;
        int bf_write_n = MIN(BLOCK_SIZE - start, len);

        memcpy((char*)bf->data+start, buf + write_n, bf_write_n);
        bf->dirty = true;
        brelease(bf);
        write_n += bf_write_n;
        len -= bf_write_n;
        offset += bf_write_n;
    
        if (offset >= inode->desc->size) {
            inode->desc->size = offset;
            inode->buf->dirty = true;
        }
    }
out:
    inode->desc->mtime = inode->ctime  = time();
    inode->buf->dirty = true;
    bwrite(inode->buf); // @todo: need to do it now?
    return write_n;
}