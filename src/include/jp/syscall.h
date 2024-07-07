#pragma once

#include <jp/types.h>

typedef enum {
    SYS_NR_TEST,
    SYS_NR_YIELD,
} syscall_e;

u32 test();
void yield();