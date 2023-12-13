#include <jp/joker.h>
#include <jp/types.h>
#include <jp/io.h>
#include <jp/string.h>
#include <jp/console.h>
#include <jp/printk.h>
#include <jp/assert.h>


void kernel_init(void)
{
    console_init();
    assert(1<3);
    assert(1>3);
    panic("test panic");
}