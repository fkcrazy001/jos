#include <jp/fs.h>
#include <jp/debug.h>
#include <jp/buffer.h>
#include <jp/string.h>

#define SUPER_NR 16

static super_block_t super_block_table[SUPER_NR];
static super_block_t *root; // root super

static super_block_t* get_free_super(void) {
    for (size_t i = 0; i < SUPER_NR; ++i) {
        super_block_t *sb = &super_block_table[i];
        if (sb->dev == DEV_NULL)
            return sb;
    }
    panic("no more superblocks");
    return NULL;
}

super_block_t* get_super(dev_t dev)
{
    for (size_t i = 0; i < SUPER_NR; ++i) {
        super_block_t *sb = &super_block_table[i];
        if (sb->dev == dev)
            return sb;
    }
    return NULL;
}

super_block_t* read_super(dev_t dev)
{
    super_block_t *sb = NULL;
    sb = get_super(dev);
    if (sb)
        return sb;
    sb = get_free_super();

    // boot|superblock|imap|zmap|inode|

    buffer_t *buf = bread(dev, 1); // block 1
    sb->buf = buf;
    sb->desc = (super_desc_t*)buf->data;
    sb->dev = dev;

    assert(sb->desc->magic == MINX1_MAGIC); // only support minix now

    // inode map
    u32 block = 2;
    for(size_t i = 0; i < sb->desc->imap_blocks; ++i) {
        assert(i < IMAP_MAX_NR);
        if((sb->imaps[i] = bread(dev, block)))
            block++;
        else
            break;
    }
    // znode map
    for(size_t i = 0; i < sb->desc->zmap_blocks; ++i) {
        assert(i < ZMAP_MAX_NR);
        if((sb->zmaps[i] = bread(dev, block)))
            block++;
        else
            break;
    }
    return sb;
}

static void mount_root(void)
{
    DEBUGK("Mount root file system...\n");
    
    device_t *device = device_find(DEV_PART, 0);
    root = read_super(device->dev);
}

void superblock_init(void)
{
    for(size_t i=0; i < SUPER_NR; ++i) {
        super_block_t *sb = &super_block_table[i];
        memset(sb, 0 , sizeof(*sb));
        sb->dev = DEV_NULL;
        list_init(&sb->inode_list);
    }
    mount_root();
}