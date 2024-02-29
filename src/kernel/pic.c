// programable interrput controller 
// for cascode 8259a chips

#include <jp/debug.h>
#include <jp/io.h>
#include <jp/interrupt.h>

// master chip io port
#define PIC_M_CTRL 0x20
#define PIC_M_DATA 0x21

// slave chip io port
#define PIC_S_CTRL 0xa0
#define PIC_S_DATA 0xa1

// end of interrupt 
#define PIC_EOI 0x20

#define M_INTS_START   PIC_INT_VEC_START
// exclude
#define M_INTS_END     (PIC_INT_VEC_START + 8)

#define S_INTS_START   M_INTS_END
// exclude
#define S_INTS_END     (PIC_INT_VEC_END + 1)
static void send_eoi(int vector)
{
    if (vector >= M_INTS_START && vector < M_INTS_END)
        outb(PIC_M_CTRL, PIC_EOI);
    if (vector >= S_INTS_START && vector < S_INTS_END) {
        outb(PIC_M_CTRL, PIC_EOI);
        outb(PIC_S_CTRL, PIC_EOI);
    }
}

u32 counter;

void pic_int_handler(u32 vector, u32 errno)
{
    send_eoi(vector);
    DEBUGK("func called, counter is %u\n", counter++);
}

void pic_int_init(void)
{
    outb(PIC_M_CTRL, 0b00010001); // ICW1: 边沿触发, 级联 8259, 需要ICW4.
    outb(PIC_M_DATA, M_INTS_START);       // ICW2: 起始中断向量号 0x20
    outb(PIC_M_DATA, 0b00000100); // ICW3: IR2接从片.
    outb(PIC_M_DATA, 0b00000001); // ICW4: 8086模式, 正常EOI

    outb(PIC_S_CTRL, 0b00010001); // ICW1: 边沿触发, 级联 8259, 需要ICW4.
    outb(PIC_S_DATA, S_INTS_START);       // ICW2: 起始中断向量号 0x28
    outb(PIC_S_DATA, 2);          // ICW3: 设置从片连接到主片的 IR2 引脚
    outb(PIC_S_DATA, 0b00000001); // ICW4: 8086模式, 正常EOI

    outb(PIC_M_DATA, 0b11111010); // 关闭所有中断,除了0和2号中断
    outb(PIC_S_DATA, 0b11111111); // 关闭所有中断
}