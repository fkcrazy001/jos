#ifndef __STD_ARG__H__
#define __STD_ARG__H__

typedef char *va_list;

#define va_start(ap, v) (ap = (va_list)&v + sizeof(va_list))
#define va_arg(ap, t)   (*(t *)((ap += sizeof(va_list)) - sizeof(va_list)))
#define va_end(ap)  (ap = (va_list)0)

#endif