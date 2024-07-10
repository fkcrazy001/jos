#pragma once

#include <jp/types.h>

typedef enum {
    SYS_NR_TEST,
    SYS_NR_SLEEP,
    SYS_NR_YIELD,
} syscall_e;

u32 test(void);
void yield(void);
// sleep ms ms, min is 10ms
void sleep(u32 ms);