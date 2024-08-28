#include <jp/assert.h>
#include <jp/printk.h>
#include <jp/types.h>
#include <jp/stdarg.h>
#include <jp/stdio.h>

int debug = 0;
static void spin(char *name) 
{
    printk("spinning in %s...\n", name);
    while(!debug);
}

void assertion_failure(char *exp, char *file, const char *func, int line)
{
    printk("\n --> assert(%s) failed!!!\n"
            "--> file: %s \n"
            "--> func: %s \n"
            "--> line: %d \n",
            exp, file, func, line);
    spin("assertion_failure()");
    // asm volatile("ud2"); // never run to here
}
static char buf[1024];
void panic(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int i = vsprintf(buf, fmt, ap);
    va_end(ap);

    printk("!!! PANIC !!!\n--> %s \n", buf);
    spin("panic");
    asm volatile("ud2"); // never run to here
}