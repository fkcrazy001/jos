#ifndef __INTERRUPT__H__
#define __INTERRUPT__H__

#include <jp/types.h>

typedef void (*pic_handler_t)(void);

/*
- 0b0101 - 任务门 (Task Gate)：很复杂，而且很低效 x64 就去掉了这种门
- 0b1110 - **中断门** (Interrupt Gate) `IF` 位自动置为 0
- 0b1111 - 陷阱门 (Trap Gate)
*/

typedef struct gate {
    u16 offset0; // 0-15
    u16 cs;
    u8  rsv;
    u8  type:4; // 中断门，陷阱门，任务门
    u8  segment:1;  // 0 FIXed for interrupte gate
    u8  dpl:2; // 特权级
    u8  present:1;
    u16 offset1; // 16-32
} _packed gate_t;

#define TASK_GATE 0b0101
#define INTERRUPT_GATE  0b1110
#define TRAP_GATE  0b1111

#define IDT_SIZE 256

#define SYSCALL_GATE 0x80

#define PIC_INT_VEC_START    0x20
#define PIC_INT_VEC_END      0x2f

#define IRQ_CLOCK 0      // 时钟
#define IRQ_KEYBOARD 1   // 键盘
#define IRQ_CASCADE 2    // 8259 从片控制器
#define IRQ_SERIAL_2 3   // 串口 2
#define IRQ_SERIAL_1 4   // 串口 1
#define IRQ_PARALLEL_2 5 // 并口 2
#define IRQ_SB16 5       // SB16 声卡
#define IRQ_FLOPPY 6     // 软盘控制器
#define IRQ_PARALLEL_1 7 // 并口 1
#define IRQ_RTC 8        // 实时时钟
#define IRQ_REDIRECT 9   // 重定向 IRQ2
#define IRQ_NIC 11       // 网卡
#define IRQ_MOUSE 12     // 鼠标
#define IRQ_MATH 13      // 协处理器 x87
#define IRQ_HARDDISK 14  // ATA 硬盘第一通道
#define IRQ_HARDDISK2 15 // ATA 硬盘第二通道

void pic_int_handler(u32 vector);
void pic_int_init(void);

void interrupt_init(void);

// disable interrupt and return pre IF
bool interrupt_disable(void);
bool get_interrupt_state(void);
void set_interrupt_state(bool state);

void pic_set_interrupt(u32 irq, bool enable);
void pic_set_interrupt_handler(u32 irq, pic_handler_t handler);
#endif