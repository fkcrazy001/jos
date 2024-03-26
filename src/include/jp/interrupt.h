#ifndef __INTERRUPT__H__
#define __INTERRUPT__H__

#include <jp/types.h>

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

#define PIC_INT_VEC_START    0x20
#define PIC_INT_VEC_END      0x2f

void pic_int_handler(u32 vector);
void pic_int_init(void);

void interrupt_init(void);

#endif