#include <jp/ide.h>
#include <jp/io.h>
#include <jp/printk.h>
#include <jp/stdio.h>
#include <jp/memory.h>
#include <jp/interrupt.h>
#include <jp/task.h>
#include <jp/string.h>
#include <jp/assert.h>
#include <jp/debug.h>
#include <jp/device.h>
#include <jp/arena.h>

// ref https://wiki.osdev.org/PCI_IDE_Controller
// https://wiki.osdev.org/ATA_PIO_Mode

// IDE 寄存器基址
#define IDE_IOBASE_PRIMARY 0x1F0   // 主通道基地址
#define IDE_IOBASE_SECONDARY 0x170 // 从通道基地址

// IDE 寄存器偏移
#define IDE_DATA 0x0000       // 数据寄存器
#define IDE_ERR 0x0001        // 错误寄存器
#define IDE_FEATURE 0x0001    // 功能寄存器
#define IDE_SECTOR 0x0002     // 扇区数量
#define IDE_LBA_LOW 0x0003    // LBA 低字节
#define IDE_LBA_MID 0x0004    // LBA 中字节
#define IDE_LBA_HIGH 0x0005   // LBA 高字节
#define IDE_HDDEVSEL 0x0006   // 磁盘选择寄存器
#define IDE_STATUS 0x0007     // 状态寄存器
#define IDE_COMMAND 0x0007    // 命令寄存器
#define IDE_ALT_STATUS 0x0206 // 备用状态寄存器
#define IDE_CONTROL 0x0206    // 设备控制寄存器
#define IDE_DEVCTRL 0x0207    // 驱动器地址寄存器

// IDE 命令

#define IDE_CMD_READ 0x20     // 读命令
#define IDE_CMD_WRITE 0x30    // 写命令
#define IDE_CMD_IDENTIFY 0xEC // 识别命令
#define IDE_CMD_CACHE_FLUSH 0xE7 // 写完一个扇区后，需要把cache数据flush出去

// IDE 控制器状态寄存器
#define IDE_SR_NULL 0x00 // NULL
#define IDE_SR_ERR 0x01  // Error
#define IDE_SR_IDX 0x02  // Index
#define IDE_SR_CORR 0x04 // Corrected data
#define IDE_SR_DRQ 0x08  // Data request
#define IDE_SR_DSC 0x10  // Drive seek complete
#define IDE_SR_DWF 0x20  // Drive write fault
#define IDE_SR_DRDY 0x40 // Drive ready
#define IDE_SR_BSY 0x80  // Controller busy

// IDE 控制寄存器
#define IDE_CTRL_HD15 0x00 // Use 4 bits for head (not used, was 0x08)
#define IDE_CTRL_SRST 0x04 // Soft reset
#define IDE_CTRL_NIEN 0x02 // Disable interrupts

// IDE 错误寄存器
#define IDE_ER_AMNF 0x01  // Address mark not found
#define IDE_ER_TK0NF 0x02 // Track 0 not found
#define IDE_ER_ABRT 0x04  // Abort
#define IDE_ER_MCR 0x08   // Media change requested
#define IDE_ER_IDNF 0x10  // Sector id not found
#define IDE_ER_MC 0x20    // Media change
#define IDE_ER_UNC 0x40   // Uncorrectable data error
#define IDE_ER_BBK 0x80   // Bad block

#define IDE_LBA_MASTER 0b11100000 // 主盘 LBA
#define IDE_LBA_SLAVE 0b11110000  // 从盘 LBA

typedef struct ide_params
{
    u16 config;                 // 0 General configuration bits
    u16 cylinders;              // 01 cylinders
    u16 RESERVED;               // 02
    u16 heads;                  // 03 heads
    u16 RESERVED[5 - 3];        // 05
    u16 sectors;                // 06 sectors per track
    u16 RESERVED[9 - 6];        // 09
    u8 serial[20];              // 10 ~ 19 序列号
    u16 RESERVED[22 - 19];      // 10 ~ 22
    u8 firmware[8];             // 23 ~ 26 固件版本
    u8 model[40];               // 27 ~ 46 模型数
    u8 drq_sectors;             // 47 扇区数量
    u8 RESERVED[3];             // 48
    u16 capabilities;           // 49 能力
    u16 RESERVED[59 - 49];      // 50 ~ 59
    u32 total_lba;              // 60 ~ 61
    u16 RESERVED;               // 62
    u16 mdma_mode;              // 63
    u8 RESERVED;                // 64
    u8 pio_mode;                // 64
    u16 RESERVED[79 - 64];      // 65 ~ 79 参见 ATA specification
    u16 major_version;          // 80 主版本
    u16 minor_version;          // 81 副版本
    u16 commmand_sets[87 - 81]; // 82 ~ 87 支持的命令集
    u16 RESERVED[118 - 87];     // 88 ~ 118
    u16 support_settings;       // 119
    u16 enable_settings;        // 120
    u16 RESERVED[221 - 120];    // 221
    u16 transport_major;        // 222
    u16 transport_minor;        // 223
    u16 RESERVED[254 - 223];    // 254
    u16 integrity;              // 校验和
} _packed ide_params_t;

typedef struct partition_info {
    u8 bootable; // 0: none-bootable , 0x80 bootable
#define BOOTABLE 0x80
    u8 start_head;
    u16 start_secotr:6,
        start_cylinder:10;
    u8  filesystem; // filesystem type
// ref https://www.win.tue.nl/~aeb/partitions/partition_types-1.html
#define PART_FS_FAT12 1
#define PART_FS_EXTENDED 5
#define PART_FS_MINIX 0x80
#define PART_FS_LINUX 0x83
    u8  end_head;
    u16 end_sector:6,
        end_cylinder:10;
    u32 start; // start lba of disk
    u32 count; // number of this partition
}_packed partition_info_t;

typedef struct boot_sector {
    u8 code[0x1BE];
    partition_info_t part[PARTITION_NR];
    u16 verifycode; // 0x55aa
}_packed boot_sector_t;

ide_ctrl_t controllers[IDE_CTRL_NR];

static u32 ide_error(ide_ctrl_t *ctrl)
{
    u8 error = inb(ctrl->iobase + IDE_ERR);
    if (error & IDE_ER_BBK)
        WARNK("bad block\n");
    if (error & IDE_ER_UNC)
        WARNK("uncorrectable data\n");
    if (error & IDE_ER_MC)
        WARNK("media change\n");
    if (error & IDE_ER_IDNF)
        WARNK("id not found\n");
    if (error & IDE_ER_MCR)
        WARNK("media change requested\n");
    if (error & IDE_ER_ABRT)
        WARNK("abort\n");
    if (error & IDE_ER_TK0NF)
        WARNK("track 0 not found\n");
    if (error & IDE_ER_AMNF)
        WARNK("address mark not found\n");
}

static u32 ide_busy_wait(ide_ctrl_t *ctrl, u8 mask)
{
    while (true)
    {
        // 从备用状态寄存器中读状态
        u8 state = inb(ctrl->iobase + IDE_ALT_STATUS);
        if (state & IDE_SR_ERR) // 有错误
        {
            ide_error(ctrl);
        }
        if (state & IDE_SR_BSY) // 驱动器忙
        {
            continue;
        }
        if ((state & mask) == mask) // 等待的状态完成
            return 0;
    }
}

static void ide_handler(uint32_t vector)
{
    ide_ctrl_t *ctrl = NULL;
    // assert(vector == IRQ_HARDDISK2 || vector==IRQ_HARDDISK);
    if (vector == IRQ_HARDDISK2) {
        ctrl = &controllers[1];
    } else if (vector == IRQ_HARDDISK) {
        ctrl = controllers;
    } else {
        panic("invalid hard disk vector %d\n", vector);
    }
    // 读取常规状态寄存器，表示中断处理结束
    u8 state = inb(ctrl->iobase + IDE_STATUS);
    DEBUGK("harddisk interrupt vector %d state 0x%x\n", vector, state);
    if (ctrl->task) {
        task_unblock(ctrl->task);
        ctrl->task = NULL; //
    } else {
        WARNK("no waiting task but ide interrupt handler was called, ctrl state=0x%x\n", inb(ctrl->iobase + IDE_ALT_STATUS));
    }
}

static u32 ide_busy_async_wait(ide_ctrl_t *ctrl, u8 mask)
{
    task_t *task = current;
    while (true)
    {
        // 从备用状态寄存器中读状态
        u8 state = inb(ctrl->iobase + IDE_ALT_STATUS);
        if (state & IDE_SR_ERR) // 有错误
        {
            ide_error(ctrl);
        }
        if (state & IDE_SR_BSY) // 驱动器忙
        {
            if (task->state == TASK_RUNNING) {
                assert(!ctrl->task);
                ctrl->task = task;
                // @todo: goto sleep state and wakeup timely
                task_block(task, NULL, TASK_BLOCKED);
            }
            continue;
        }
        if ((state & mask) == mask) // 等待的状态完成
            return 0;
    }
}

// 选择磁盘
static void ide_select_drive(ide_disk_t *disk)
{
    outb(disk->ctrl->iobase + IDE_HDDEVSEL, disk->selector);
    disk->ctrl->active = disk;
}

// 选择扇区
static void ide_select_sector(ide_disk_t *disk, u32 lba, u8 count)
{
    // 输出功能，可省略
    outb(disk->ctrl->iobase + IDE_FEATURE, 0);

    // 读写扇区数量
    outb(disk->ctrl->iobase + IDE_SECTOR, count);

    // LBA 低字节
    outb(disk->ctrl->iobase + IDE_LBA_LOW, lba & 0xff);
    // LBA 中字节
    outb(disk->ctrl->iobase + IDE_LBA_MID, (lba >> 8) & 0xff);
    // LBA 高字节
    outb(disk->ctrl->iobase + IDE_LBA_HIGH, (lba >> 16) & 0xff);

    // LBA 最高四位 + 磁盘选择
    outb(disk->ctrl->iobase + IDE_HDDEVSEL, ((lba >> 24) & 0xf) | disk->selector);
    disk->ctrl->active = disk;
}

// 从磁盘读取一个扇区到 buf
static void ide_pio_read_sector(ide_disk_t *disk, u16 *buf)
{
    for (size_t i = 0; i < (SECTOR_SIZE / 2); i++)
    {
        buf[i] = inw(disk->ctrl->iobase + IDE_DATA);
    }
}

// 从 buf 写入一个扇区到磁盘
static void ide_pio_write_sector(ide_disk_t *disk, u16 *buf)
{
    for (size_t i = 0; i < (SECTOR_SIZE / 2); i++)
    {
        outw(disk->ctrl->iobase + IDE_DATA, buf[i]);
    }
}

static void ide_swap_pairs(char *buf, u32 len)
{
    assert(len%2==0);
    for (size_t i = 0; i < len; i += 2)
    {
        register char ch = buf[i];
        buf[i] = buf[i + 1];
        buf[i + 1] = ch;
    }
    buf[len - 1] = '\0';
}


// 重置硬盘控制器
static void ide_reset_controller(ide_ctrl_t *ctrl)
{
    outb(ctrl->iobase + IDE_CONTROL, IDE_CTRL_SRST);
    ide_busy_wait(ctrl, IDE_SR_NULL);
    outb(ctrl->iobase + IDE_CONTROL, ctrl->control);
    ide_busy_wait(ctrl, IDE_SR_NULL);
}

struct disk_rw_handler {
    task_t *task;
    u32 lba;
    bool write;
    list_node_t node;
};


// must call disk_seek_next
// 电梯调度，防止某个进程被饿死
static struct disk_rw_handler* disk_rw_handler_req(ide_disk_t *disk, u32 lba, bool write)
{
    if (current->state != TASK_RUNNING)
        return NULL;
    bool empty = list_empty(&disk->rwlist);
    struct disk_rw_handler *req = kmalloc(sizeof(*req));
    req->write = write;
    req->task = current;
    req->lba = lba;
    
    struct disk_rw_handler *pos;
    list_for_each(pos, &disk->rwlist, node) {
        if (pos->lba > req->lba)
            break;
    }
    list_insert_before(&pos->node, &req->node);

    if (!empty) {
        assert(current->state == TASK_RUNNING);
        task_block(req->task, NULL, TASK_BLOCKED);
    }
    return req;
}

static void disk_seek_next(ide_disk_t* disk, struct disk_rw_handler* handler)
{
    if (handler == NULL) // just for bootup reading use
        return;
    list_node_t current_node;
    memcpy(&current_node, &handler->node, sizeof(list_node_t));
    list_del(&handler->node);
    kfree(handler);
    if (!list_empty(&disk->rwlist)) {
        task_t *next = NULL;
        if (disk->going_up) {
            next = container_of(struct disk_rw_handler, node, current_node.next)->task;
        } else {
            next = container_of(struct disk_rw_handler, node, current_node.prev)->task;
        }
        assert(next->magic == J_MAGIC);
        task_unblock(next);
    } else {
        if (disk->going_up && current_node.next == &disk->rwlist) {
            DEBUGK("disk scan dir changed up->down\n");
            disk->going_up = false;
        }
        if (!disk->going_up && current_node.prev == &disk->rwlist) {
            DEBUGK("disk scan dir changed down->up\n");
            disk->going_up = true;
        }
    }
}

// PIO 方式读取磁盘
int ide_pio_read(ide_disk_t *disk, void *buf, u8 count, u32 lba)
{
    assert(count > 0);

    ide_ctrl_t *ctrl = disk->ctrl;

    // disk do-scheduler first
    struct disk_rw_handler* handler = disk_rw_handler_req(disk, lba, 0);

    lock_up(&ctrl->lock);

    assert(!get_interrupt_state());

    DEBUGK("read disk %s lba 0x%x\n", disk->name, lba);

    // 选择磁盘
    ide_select_drive(disk);

    // 等待就绪
    ide_busy_async_wait(ctrl, IDE_SR_DRDY);

    // 选择扇区
    ide_select_sector(disk, lba, count);

    // 发送读命令
    outb(ctrl->iobase + IDE_COMMAND, IDE_CMD_READ);

    for (size_t i = 0; i < count; i++)
    {
        ide_busy_async_wait(ctrl, IDE_SR_DRQ);
        u32 offset = ((u32)buf + i * SECTOR_SIZE);
        ide_pio_read_sector(disk, (u16 *)offset);
    }

    lock_down(&ctrl->lock);
    disk_seek_next(disk, handler);
    return 0;
}

// PIO 方式写磁盘
int ide_pio_write(ide_disk_t *disk, void *buf, u8 count, u32 lba)
{
    assert(count > 0 && lba < disk->total_lba);

    ide_ctrl_t *ctrl = disk->ctrl;
    struct disk_rw_handler* handler = disk_rw_handler_req(disk, lba, 1);
    lock_up(&ctrl->lock);
    assert(!get_interrupt_state());
    DEBUGK("write disk %s lba 0x%x\n", disk->name, lba);

    // 选择磁盘
    ide_select_drive(disk);

    // 等待就绪
    ide_busy_async_wait(ctrl, IDE_SR_DRDY);

    // 选择扇区
    ide_select_sector(disk, lba, count);

    // 发送写命令
    outb(ctrl->iobase + IDE_COMMAND, IDE_CMD_WRITE);

    for (size_t i = 0; i < count; i++)
    {
        u32 offset = ((u32)buf + i * SECTOR_SIZE);
        ide_pio_write_sector(disk, (u16 *)offset);
        outb(ctrl->iobase + IDE_COMMAND, IDE_CMD_CACHE_FLUSH);
        ide_busy_async_wait(ctrl, IDE_SR_NULL);
    }

    lock_down(&ctrl->lock);
    disk_seek_next(disk, handler);
    return 0;
}

int ide_pio_part_read(ide_part_t *p, void *buf, u8 count, u32 lba)
{
    return ide_pio_read(p->disk, buf, count, p->start + lba);
}

int ide_pio_part_write(ide_part_t *p, void *buf, u8 count, u32 lba)
{
    return ide_pio_write(p->disk, buf, count, p->start + lba);
}


static u32 ide_identify(ide_disk_t *disk, u16 *buf)
{
    DEBUGK("identifing disk %s...\n", disk->name);
    lock_up(&disk->ctrl->lock);
    ide_select_drive(disk);

    // ide_select_sector(disk, 0, 0);

    outb(disk->ctrl->iobase + IDE_COMMAND, IDE_CMD_IDENTIFY);

    ide_busy_wait(disk->ctrl, IDE_SR_NULL);

    ide_pio_read_sector(disk, buf);
    
    ide_params_t *params = (ide_params_t *)buf;

    DEBUGK("disk %s total lba %d\n", disk->name, params->total_lba);

    u32 ret = EOF;
    if (params->total_lba == 0) {
        goto out;
    }

    ide_swap_pairs(params->serial, sizeof(params->serial));
    DEBUGK("disk %s serial number %s\n", disk->name, params->serial);

    ide_swap_pairs(params->firmware, sizeof(params->firmware));
    DEBUGK("disk %s firmware version %s\n", disk->name, params->firmware);

    ide_swap_pairs(params->model, sizeof(params->model));
    DEBUGK("disk %s model number %s\n", disk->name, params->model);

    disk->total_lba = params->total_lba;
    disk->cylinders = params->cylinders;
    disk->heads = params->heads;
    disk->sectors = params->sectors;
    ret = 0;

out:
    lock_down(&disk->ctrl->lock);
    return ret;
}

static void ide_partition_init(ide_disk_t* disk, void *buf)
{
    if (!disk->total_lba) {
        return; // not an available disk
    }
    int err = ide_pio_read(disk, buf, 1, 0); // first sector
    if (err) {
        WARNK("failed reading first sector of %s!!!\n", disk->name);
        return;
    }
    boot_sector_t *bs = (boot_sector_t*)buf;
    // assert(bs->verifycode == 0xaa55);
    for (size_t i = 0; i < PARTITION_NR; ++i) {
        partition_info_t *pi = bs->part + i;
        ide_part_t *p = disk->part + i;
        if (!pi->count) {
            DEBUGK("%d part of disk %s not a valid partition\n", i, disk->name);
            continue;
        }
        sprintf(p->name, "%s%d", disk->name, i+1);
        p->disk = disk;
        p->filesystem = pi->filesystem;
        p->count = pi->count;
        p->start = pi->start;
        DEBUGK("part %s \n", p->name);
        DEBUGK("    bootable %d\n", pi->bootable == BOOTABLE);
        DEBUGK("    start %d\n", pi->start);
        DEBUGK("    count %d\n", pi->count);
        DEBUGK("    filesystem 0x%x\n", pi->filesystem);

        if (pi->filesystem == PART_FS_EXTENDED) {
            WARNK("extended partition fs not support!!!\n");
            void *ebuf = (void*)((char*)buf + SECTOR_SIZE);
            ide_pio_part_read(p, ebuf, 1, 0);
            boot_sector_t *ebs = ebuf;
            for (size_t j = 0; j < PARTITION_NR; ++j) {
                partition_info_t *epi = ebs->part + j;
                if (!epi->count) {
                    DEBUGK("%d part of extended partition on %s not a valid partition\n", i, disk->name);
                    continue;
                }
                DEBUGK("    bootable %d\n", epi->bootable == BOOTABLE);
                DEBUGK("    start %d\n", epi->start);
                DEBUGK("    count %d\n", epi->count);
                DEBUGK("    filesystem 0x%x\n", epi->filesystem);
            }
        }
    }
}

static void ide_ctrl_init()
{
    u16 *buf = (u16*)alloc_kpage(1);
    for (size_t cidx = 0; cidx < IDE_CTRL_NR; cidx++)
    {
        ide_ctrl_t *ctrl = &controllers[cidx];
        sprintf(ctrl->name, "ide%u", cidx);
        lock_init(&ctrl->lock);
        ctrl->active = NULL;
        ctrl->task = NULL;

        if (cidx) // 从通道
        {
            ctrl->iobase = IDE_IOBASE_SECONDARY;
            pic_set_interrupt_handler(IRQ_HARDDISK2,  (pic_handler_t)ide_handler);
            pic_set_interrupt(IRQ_HARDDISK2, true);
        }
        else // 主通道
        {
            ctrl->iobase = IDE_IOBASE_PRIMARY;
            pic_set_interrupt_handler(IRQ_HARDDISK,  (pic_handler_t)ide_handler);
            pic_set_interrupt(IRQ_HARDDISK, true);
        }
        for (size_t didx = 0; didx < IDE_DISK_NR; didx++)
        {
            ide_disk_t *disk = &ctrl->disks[didx];
            sprintf(disk->name, "hd%c", 'a' + cidx * 2 + didx);
            disk->ctrl = ctrl;
            if (didx) // 从盘
            {
                disk->master = false;
                disk->selector = IDE_LBA_SLAVE;
            }
            else // 主盘
            {
                disk->master = true;
                disk->selector = IDE_LBA_MASTER;
            }
            ide_identify(disk, buf);
            ide_partition_init(disk, buf);
        }
    }
    free_kpage((u32)buf, 1);
}

static int disk_ioctl(ide_disk_t* disk, int cmd, void *args, int flags)
{
    switch (cmd)
    {
    case DEV_CMD_SECTOR_START:
        return 0;
    case DEV_CMD_SECTOR_COUNT:
        return disk->total_lba;
    default:
        break;
    }
    panic("cmd %d not implemented\n", cmd);
}

static int part_ioctl(ide_part_t* part, int cmd, void *args, int flags)
{
    switch (cmd)
    {
    case DEV_CMD_SECTOR_START:
        return part->start;
    case DEV_CMD_SECTOR_COUNT:
        return part->count;
    default:
        break;
    }
    panic("cmd %d not implemented\n", cmd);
}

static void ide_dev_init(void)
{
    for (size_t cidx = 0; cidx < IDE_CTRL_NR; cidx++){
        ide_ctrl_t *ctrl = &controllers[cidx];
        for (size_t didx = 0; didx < IDE_DISK_NR; didx++) {
            ide_disk_t *disk = &ctrl->disks[didx];
            if (!disk->total_lba)
                continue;
            list_init(&disk->rwlist);
            disk->going_up = true;
            dev_t dev =  device_register(DEV_BLOCK, DEV_IDE_DISK,
                disk, disk->name, DEV_NULL,
                (f_ioctl)disk_ioctl, (f_read)ide_pio_read,  (f_write)ide_pio_write);
            for (size_t t = 0; t < PARTITION_NR; ++t) {
                ide_part_t *part = &disk->part[t];
                if(!part->count)
                    continue;
                device_register(DEV_BLOCK, DEV_IDE_PART,
                    part, part->name, dev,
                    (f_ioctl)part_ioctl, (f_read)ide_pio_part_read, (f_write)ide_pio_part_write);
            }
        }
    }
}

void ide_init(void)
{
    DEBUGK("ide init...\n");
    ide_ctrl_init();
    ide_dev_init();
}
