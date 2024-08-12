#pragma once

#include <jp/types.h>

// #include <asm/unistd_32.h> //keep the same in linux

typedef enum {
    SYS_NR_TEST,
    SYS_NR_EXIT = 1,
    SYS_NR_FORK = 2,
    SYS_NR_WRITE = 4,
    SYS_NR_WAITPID = 7,
    SYS_NR_TIME = 13,
    SYS_NR_GETPID = 20,
    SYS_NR_BRK= 45,
    SYS_NR_GETPPID = 64,
    SYS_NR_YIELD = 158,
    SYS_NR_SLEEP = 162,
} syscall_e;
u32 test(void);
void yield(void);
// sleep ms ms, min is 10ms
void sleep(u32 ms);

int32_t write(fd_t fd, char *buf, u32 len);
int32_t brk(char *addr);
int32_t getpid(void);
int32_t getppid(void);

int32_t fork(void);
#define noreturn
noreturn void exit(int status);
int32_t waitpid(int32_t pid, int *status);
time_t time(void);