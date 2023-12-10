#ifndef __STD_IO__H
#define __STD_IO__H

#include <jp/stdarg.h>

int vsprintf(char *buf, const char *fmt, va_list args);
int sprintf(char *buf, const char *fmt, ...);

#endif