#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <jp/types.h>

#define GDT_SIZE  128 // max = 2^sizeof(selector_t:index) = 8192

#if GDT_SIZE > 8192
#error "GDT_SIZE too big!!! max is 8192"
#endif

typedef struct descriptor_t /* 共 8 个字节 */
{
    unsigned short limit_low;      // 段界限 0 ~ 15 位
    unsigned int base_low : 24;    // 基地址 0 ~ 23 位 16M
    unsigned char type : 4;        // 段类型
    unsigned char segment : 1;     // 1 表示代码段或数据段，0 表示系统段
    unsigned char DPL : 2;         // Descriptor Privilege Level 描述符特权等级 0 ~ 3
    unsigned char present : 1;     // 存在位，1 在内存中，0 在磁盘上
    unsigned char limit_high : 4;  // 段界限 16 ~ 19;
    unsigned char available : 1;   // 该安排的都安排了，送给操作系统吧
    unsigned char long_mode : 1;   // 64 位扩展标志
    unsigned char big : 1;         // 32 位 还是 16 位;
    unsigned char granularity : 1; // 段界限粒度 4KB 或 1B
    unsigned char base_high;       // 基地址 24 ~ 31 位
} _packed descriptor_t;

typedef struct tss {
    u32 backlink;
    u32 esp0; // ring0 esp
    u32 ss0; // ring0 sp selecotr
    u32 esp1;
    u32 ss1;
    u32 esp2;
    u32 ss2;
    u32 cr3;
    u32 eip;
    u32 eflags;
    u32 eax;
    u32 ecx;
    u32 edx;
    u32 ebx;
    u32 esp;
    u32 ebp;
    u32 esi;
    u32 edi;
    u32 es;
    u32 cs;
    u32 ss;
    u32 ds;
    u32 fs;
    u32 gs;
    u32 ldtr; // 局部描述符选择子
    u16 trace:1, // 如果置位，任务切换的时候将触发一个调试异常
        reserved:15;
    u16 iobase; // io 位图基地址,如果不用，那么这个值等于tss的长度，如果要用的话，得在这个tss的末尾加iomap信息，注意的是tss中的长度是可以变动的
    u32 ssp; // shadow stack pointer, for stack overflow detect 
    u8 iomap[0]; // extend if has iomap 
}_packed tss_t;

#define SEG_CODE_OR_DATA  1
// sub type when seg is code or data
#define SEG_CODE 0b1010 // 代码|非依从|可读|没有被访问过
#define SEG_DATA 0b0010 // 数据|向上增长|可写|没有被访问过

#define SEG_SYSTEM        0

// sub type when seg is system
#define TASK_GATE 0b0101
#define INTERRUPT_GATE  0b1110
#define TRAP_GATE  0b1111
#define TSS_32_A   0b1001
// 段选择子
typedef union selector_t
{
    struct {
        u16 RPL : 2; // Request Privilege Level
        u16 TI : 1;  // Table Indicator 1: 局部描述符，0：全局描述符
        u16 index : 13;
    };
    u16 selector;
} selector_t;

// 全局描述符表指针
typedef struct pointer_t
{
    u16 limit;
    u32 base;
} _packed pointer_t;

void gdt_init(void);

#define KERNEL_CODE_IDX 1
#define KERNEL_DATA_IDX 2
#define KERNEL_TSS_IDX 3 // kernel task descriptor

#define USER_CODE_IDX 4
#define USER_DATA_IDX 5

#define KERNEL_CS   (KERNEL_CODE_IDX << 3)
#define KERNEL_DS   (KERNEL_DATA_IDX << 3)
#define KERNEL_TSS_SELECTOR (KERNEL_TSS_IDX << 3)

// dpl 0x3
#define USER_CODE_SELECTOR (USER_CODE_IDX<<3|0b11)
#define USER_DATA_SELECTOR (USER_DATA_IDX<<3|0b11) 

#endif