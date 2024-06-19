// programable interrput controller 
// for cascode 8259a chips

#include <jp/debug.h>
#include <jp/io.h>
#include <jp/interrupt.h>
#include <jp/assert.h>
#include <jp/debug.h>

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

static pic_handler_t pic_table[S_INTS_END - S_INTS_START] = {0};
static void pic_send_eoi(int vector)
{
    if (vector >= M_INTS_START && vector < M_INTS_END)
        outb(PIC_M_CTRL, PIC_EOI);
    if (vector >= S_INTS_START && vector < S_INTS_END) {
        outb(PIC_M_CTRL, PIC_EOI);
        outb(PIC_S_CTRL, PIC_EOI);
    }
}

u32 counter;

extern void yield(void);
void pic_int_handler(u32 vector)
{
    pic_send_eoi(vector);
    u32 irq = vector - M_INTS_START;
    if (pic_table[irq] == NULL) {
        DEBUGK("pic default func called, vector is %u, counter is %u\n", vector, counter++);
    } else {
        pic_table[irq]();
    }
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

    outb(PIC_M_DATA, 0b11111111); // 关闭所有中断,除了0和2号中断
    outb(PIC_S_DATA, 0b11111111); // 关闭所有中断
}

void pic_set_interrupt_handler(u32 irq, pic_handler_t handler)
{
    
    assert(irq >= 0 && irq < (PIC_INT_VEC_END - PIC_INT_VEC_START));
    if (pic_table[irq] != NULL) {
        panic("same irq %d assigned\n", irq);
    }
    pic_table[irq] = handler;
}

void pic_set_interrupt(u32 irq, bool enable)
{
    assert(irq >= 0 && irq < (PIC_INT_VEC_END - PIC_INT_VEC_START));
    u16 port;
    u32 n = irq + M_INTS_START;
    bool cascade = false; // need to open cascade irq when irq >= 8
    if (n < M_INTS_END) {
        port = PIC_M_DATA;
    } else {
        port = PIC_S_DATA;
        irq -= 8;
        cascade = true;
    }

    if (enable) {
        outb(port, inb(port) & ~(1<<irq));
    } else {
        outb(port, inb(port)|(1<<irq));
    }
    if (cascade) {
        if (enable) {
            outb(PIC_M_DATA, inb(PIC_M_DATA) & ~(1<<IRQ_CASCADE));
        } else {
            u8 all_disable = (inb(PIC_S_DATA) == 0xff);
            if (all_disable) {
                outb(PIC_M_DATA, inb(PIC_M_DATA) | (1<<IRQ_CASCADE));
            }
        }
    }
}