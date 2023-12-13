#include <jp/debug.h>
#include <jp/stdarg.h>
#include <jp/stdio.h>
#include <jp/printk.h>

static char buf[1024];

void debugk(char *file, const char* func, int line, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    printk("[%s:%d]<%s> %s", file, line, func, buf);
}