#pragma once
#include <jp/types.h>
#include <jp/list.h>
#include <jp/buffer.h>

// boot|superblock|imap|zmap|inode|

typedef struct super_desc {
    u16 inodes; // 节点数
    u16 zones; // 一个zone有几个逻辑块
    u16 imap_blocks; // i节点位图所占用的数据块数
    u16 zmap_blocks; // 逻辑块位图所占据的数据块数
    u16 firstdatazone; // 第一个数据逻辑块号
    u16 log_zone_size; // log2（zones）
    u32 max_size; // 文件最大长度
    u16 magic; // 文件系统魔数
} super_desc_t;

/// @brief for minix-fs 1.0 only
#define BLOCK_SIZE 1024  // 块大小
#define SECTOR_SIZE 512  // 扇区大小

#define MINX1_MAGIC 0x137f
#define NAME_LEN 14

#define IMAP_MAX_NR 8
#define ZMAP_MAX_NR 8

typedef struct dentry
{
    u16 nr; // 在imap位图 以及 inode 中的第nr-1个
    char name[NAME_LEN]; // 文件名
}dentry_t;

typedef struct inode_desc
{
    u16 mode; // rwx
    u16 uid;
    u32 size;
    u32 mtime;
    u8 gid;
    u8 nlinks; // hard links
    u16 zones[9]; // 0-6(直接)， 7（间接），8二重间接
}inode_desc_t;

#define inode_fmt(node) "mode 0x%x, uid %d, size %u, mtime %u, gid %d, nlinks %d" \
                  ", zone[0]=%d, zone[1]=%d, zone[2]=%d, zone[3]=%d, zone[4]=%d, zone[5]=%d, zone[6]=%d, zone[7]=%d, zone[8]=%d\n", \
                  node->mode, node->uid, node->size, node->mtime, \
                  node->gid, node->nlinks, \
                  node->zones[0], node->zones[1], node->zones[2], \
                  node->zones[3], node->zones[4], node->zones[5], \
                  node->zones[6], node->zones[7], node->zones[8]

#define entry_fmt(node) "nr = %d, name=%s\n",node->nr, node->name
/// end of minix-fs 1.0

//0 inode on mem
typedef struct inode{
    inode_desc_t *desc; // inode 描述符，从磁盘上读过来的
    buffer_t *buf; // inode 所属的 buffer
    dev_t dev;
    u32 nr; // i 节点号
    u32 count;
    time_t atime; // access time
    time_t ctime;
    list_node_t node;
    dev_t mount; // 安装设备？ used in mount
} inode_t;

typedef struct super_block {
    super_desc_t *desc;
    buffer_t *buf; // super块 所属的 buffer
    buffer_t *imaps[IMAP_MAX_NR]; // imaps 所对应的block
    buffer_t *zmaps[ZMAP_MAX_NR]; // zmaps 所对应的block
    dev_t dev; // 设备号
    list_node_t inode_list; // 使用中inode
    inode_t *iroot;  // 根目录inode
    inode_t *imount; // 安装到的inode
} super_block_t;

super_block_t* get_super(dev_t dev);
super_block_t* read_super(dev_t dev);