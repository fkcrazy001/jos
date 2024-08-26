#include <jp/stdarg.h>
#include <jp/console.h>
#include <jp/stdio.h>

static char buf[1024];

extern int console_write();

int printk(const char *fmt, ...)
{
    va_list args;
    int i;

    va_start(args, fmt);

    i = vsprintf(buf, fmt, args);

    va_end(args);

    console_write(NULL, buf, i);

    return i;
}
