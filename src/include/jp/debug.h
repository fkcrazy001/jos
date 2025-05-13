#ifndef __DEBUG_H__
#define __DEBUG_H__
#include <jp/printk.h>
void debugk(char *file, const char* func, int line, const char* fmt, ...);

#define BMB asm volatile("xchgw %bx, %bx") // bochs magic bk
#ifdef DEBUG_LOG
#define DEBUGK(fmt, args...) do { \
    debugk(__FILE__, __func__, __LINE__, fmt, ##args); \
    printk("\n"); \
}while(0)
#else 
#define DEBUGK
#endif
#define WARNK(fmt, args...) debugk(__FILE__, __func__, __LINE__, fmt, ##args)
#endif