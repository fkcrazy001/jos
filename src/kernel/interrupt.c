#include <jp/interrupt.h>
#include <jp/global.h>
#include <jp/string.h>
#include <jp/debug.h>

static gate_t idt[IDT_SIZE];
static pointer_t pidt;

extern void interrupt_handler(void);

void interrupt_init(void)
{
    int i = 0;
    for ( i = 0; i < IDT_SIZE; ++i) {
        gate_t *ig = &idt[i];
        memset(ig, 0, sizeof(*ig));
        ig->cs = KERNEL_CS;
        ig->dpl = 0; // only kernel can access
        ig->offset0 = (u32)interrupt_handler & 0xffff;
        ig->offset1 = ((u32)interrupt_handler >> 16) & 0xffff;
        ig->segment = SEG_SYSTEM;
        ig->type = INTERRUPT_GATE;
        ig->present = 1;
    }
    pidt.base = (u32)idt;
    pidt.limit = sizeof(idt) - 1;
    //BMB;

    asm volatile("lidt pidt\n");
}
