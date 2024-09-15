#include <jp/fs.h>
#include <jp/debug.h>
#include <jp/buffer.h>
#include <jp/string.h>

void superblock_init(void)
{
    device_t *device = device_find(DEV_PART, 0);
    assert(device);
    u32 offset = 0;
    buffer_t *boot = bread(device->dev, offset++);
    buffer_t *super = bread(device->dev, offset++);

    super_desc_t *sb = (super_desc_t*)super->data;
    assert(sb->magic == MINX1_MAGIC);

    buffer_t *imap = bread(device->dev, offset);
    offset+=sb->imap_blocks;

    buffer_t *zmap = bread(device->dev, offset);
    offset += sb->zmap_blocks;

    buffer_t *buf1 = bread(device->dev, offset);
    inode_desc_t *inode = (inode_desc_t*)buf1->data; // first inodes, discribe / dir
    DEBUGK("fmt node: "inode_fmt(inode));
    buffer_t *buf2 = bread(device->dev, inode->zones[0]);
    dentry_t *dir = (dentry_t*)buf2->data;
    inode_desc_t *helloi = NULL;

    while (dir->nr)
    {
        DEBUGK("fmt dentry: "entry_fmt(dir));
        if (!strcmp(dir->name, "hello.txt")) {
            helloi = (inode_desc_t *)(inode + dir->nr - 1);
            strcpy(dir->name, "world.txt");
            buf2->dirty = true;
        }
        dir++;
    }
    buffer_t *buf3 = bread(device->dev, helloi->zones[0]);
    DEBUGK("hello.txt reads: %s", (char*)buf3->data);
    strcpy(buf3->data, "response from jos!!!");
    buf3->dirty = true;
    brelease(buf3);

    brelease(buf2);

    helloi->size = strlen(buf3->data); // this is a inode on buf1
    buf1->dirty = true;
    brelease(buf1);
}