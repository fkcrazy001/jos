#include <jp/joker.h>
#include <jp/types.h>
#include <jp/io.h>
#include <jp/string.h>
#include <jp/console.h>
#include <jp/stdarg.h>

void test_arg(int cnt, ...) {
    va_list args;
    va_start(args, cnt);
    
    int arg;
    while (cnt--)
    {
        arg = va_arg(args, int);
    }
    va_end(args);
}

void kernel_init(void)
{
    console_init();
    test_arg(5, 1, 2, 3, 4, 5);
}