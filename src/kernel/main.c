#include <jp/joker.h>
#include <jp/types.h>
#include <jp/io.h>
#include <jp/string.h>
#include <jp/console.h>
#include <jp/printk.h>
#include <jp/assert.h>
#include <jp/debug.h>
#include <jp/global.h>

void kernel_init(void)
{
    console_init();
    gdt_init();
}