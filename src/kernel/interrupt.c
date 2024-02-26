#include <jp/interrupt.h>
#include <jp/global.h>
#include <jp/string.h>
#include <jp/debug.h>
#include <jp/printk.h>
#include <jp/joker.h>

static gate_t idt[IDT_SIZE];
static pointer_t pidt;

#define   INTERRUPT_ENTRY_SIZE   0x20
extern void* handler_entry_table[INTERRUPT_ENTRY_SIZE];

typedef void (*handler_t)(u32 vector, u32 eflags);

handler_t handler_table[INTERRUPT_ENTRY_SIZE];

static char *messages[] = {
    "#DE Divide Error\0",
    "#DB RESERVED\0",
    "--  NMI Interrupt\0",
    "#BP Breakpoint\0",
    "#OF Overflow\0",
    "#BR BOUND Range Exceeded\0",
    "#UD Invalid Opcode (Undefined Opcode)\0",
    "#NM Device Not Available (No Math Coprocessor)\0",
    "#DF Double Fault\0",
    "    Coprocessor Segment Overrun (reserved)\0",
    "#TS Invalid TSS\0",
    "#NP Segment Not Present\0",
    "#SS Stack-Segment Fault\0",
    "#GP General Protection\0",
    "#PF Page Fault\0",
    "--  (Intel reserved. Do not use.)\0",
    "#MF x87 FPU Floating-Point Error (Math Fault)\0",
    "#AC Alignment Check\0",
    "#MC Machine Check\0",
    "#XF SIMD Floating-Point Exception\0",
    "#VE Virtualization Exception\0",
    "#CP Control Protection Exception\0",
};

static void exception_handler(u32 vector, u32 error)
{
    const char* msg = NULL;
    if (vector < NELE(messages)) {
        msg = messages[vector];
    } else {
        msg = messages[15]; // GP
    }
    printk("Exception: [0x%02x] %s, error 0x%x", vector, msg, error);

    //while (true); // spin here
}

void interrupt_init(void)
{
    int i = 0;
    for ( i = 0; i < INTERRUPT_ENTRY_SIZE; ++i) {
        gate_t *ig = &idt[i];
        void* hfunc = handler_entry_table[i];
        memset(ig, 0, sizeof(*ig));
        ig->cs = KERNEL_CS;
        ig->dpl = 0; // only kernel can access
        ig->offset0 = (u32)hfunc & 0xffff;
        ig->offset1 = ((u32)hfunc >> 16) & 0xffff;
        ig->segment = SEG_SYSTEM;
        ig->type = INTERRUPT_GATE;
        ig->present = 1;
    }
    for (; i < IDT_SIZE; ++i) {
        memset(idt+i, 0 , sizeof(idt[0]));
    }
    for (i = 0; i < INTERRUPT_ENTRY_SIZE; ++i) {
        handler_table[i] = exception_handler;
    }
    pidt.base = (u32)idt;
    pidt.limit = sizeof(idt) - 1;
    //BMB;

    asm volatile("lidt pidt\n");
}
