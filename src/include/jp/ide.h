#pragma once

#include <jp/types.h>
#include <jp/mutex.h>
#include <jp/task.h>
#include <jp/list.h>
// ata总线，28位lba模式，最多有四个ide磁盘

#define SECTOR_SIZE 512 // 扇区大小

#define IDE_CTRL_NR 2 // 控制器数量，固定为 2
#define IDE_DISK_NR 2 // 每个控制器可挂磁盘数量，固定为 2

#define PARTITION_NR 4

struct ide_disk;

typedef struct ide_part {
    char name[8]; // partition name 
    struct ide_disk *disk;
    u32 filesystem;
    u32 start;
    u32 count;
}ide_part_t;

// IDE 磁盘, ata总线
typedef struct ide_disk
{
    char name[8];            // 磁盘名称
    struct ide_part part[PARTITION_NR]; // 分区
    struct ide_ctrl_t *ctrl; // 控制器指针
    u8 selector;             // 磁盘选择
    bool master;             // 主盘
    u32 total_lba;    // 可用扇区数目
    u32 cylinders; // 柱面数？
    u32 heads; // 磁头数
    u32 sectors; // 扇区数
    list_node_t rwlist;
} ide_disk_t;

// IDE 控制器
typedef struct ide_ctrl_t
{
    char name[8];                  // 控制器名称 
    lock_t lock;                   // 控制器锁
    u16 iobase;                    // IO 寄存器基址
    ide_disk_t disks[IDE_DISK_NR]; // 磁盘
    ide_disk_t *active;            // 当前选择的磁盘
    task_t *task;
    u8 control; // 控制字节
} ide_ctrl_t;

extern ide_ctrl_t controllers[IDE_CTRL_NR];
void ide_init(void);
int ide_pio_read(ide_disk_t *disk, void *buf, u8 count, u32 lba);
int ide_pio_write(ide_disk_t *disk, void *buf, u8 count, u32 lba);