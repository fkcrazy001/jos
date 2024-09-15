#pragma once
#include <jp/types.h>

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