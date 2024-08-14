#pragma once

#include <jp/types.h>
#include <jp/mutex.h>

// ata总线，28位lba模式，最多有四个ide磁盘

#define SECTOR_SIZE 512 // 扇区大小

#define IDE_CTRL_NR 2 // 控制器数量，固定为 2
#define IDE_DISK_NR 2 // 每个控制器可挂磁盘数量，固定为 2

// IDE 磁盘, ata总线
typedef struct ide_disk_t
{
    char name[8];            // 磁盘名称
    struct ide_ctrl_t *ctrl; // 控制器指针
    u8 selector;             // 磁盘选择
    bool master;             // 主盘
} ide_disk_t;

// IDE 控制器
typedef struct ide_ctrl_t
{
    char name[8];                  // 控制器名称
    lock_t lock;                   // 控制器锁
    u16 iobase;                    // IO 寄存器基址
    ide_disk_t disks[IDE_DISK_NR]; // 磁盘
    ide_disk_t *active;            // 当前选择的磁盘
} ide_ctrl_t;
void ide_init(void);
int ide_pio_read(ide_disk_t *disk, void *buf, u8 count, u32 lba);
int ide_pio_write(ide_disk_t *disk, void *buf, u8 count, u32 lba);