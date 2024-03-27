#include <jp/joker.h>
#include <jp/types.h>
#include <jp/io.h>
#include <jp/string.h>
#include <jp/console.h>
#include <jp/printk.h>
#include <jp/assert.h>
#include <jp/debug.h>
#include <jp/global.h>
#include <jp/task.h>
#include <jp/interrupt.h>
#include <jp/stdlib.h>

void kernel_init(void)
{
    console_init();
    gdt_init();
    interrupt_init();
    // task_init();
    asm volatile("sti\n"
                "movl %eax, %eax\n");
    hang();
}