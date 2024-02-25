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

void kernel_init(void)
{
    console_init();
    gdt_init();
    interrupt_init();
    //asm volatile("int $0x80\n");
    //int a = 11/0; // modify 0 error
    task_init();
}