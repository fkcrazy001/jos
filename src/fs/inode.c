#include <jp/fs.h>
#include <jp/debug.h>
#include <jp/string.h>

#include <jp/syscall.h>

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
    if (node)
        return node;
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
}

static void put_free_inode(inode_t *node)
{
    memset(node, 0, sizeof(*node));
    node_init(&node->node);
    node->dev = DEV_NULL;
}

void input(inode_t *inode)
{
    assert(inode && inode->count > 0);
    inode->count--;
    if (inode->count)
        return;
    brelease(inode->buf);
    list_del(&inode->node);
    put_free_inode(inode);
}


int bmap(inode_t *inode, int block, bool create)
{

}


void inode_init(void)
{
    for (int i = 0; i < INODE_MAX; ++i) {
        put_free_inode(&inode_table[i]);
    }
}