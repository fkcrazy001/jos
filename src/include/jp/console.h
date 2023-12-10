#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include <jp/types.h>

void console_init(void);
void console_clear(void);
void console_write(char* buf, u32 count);

#endif