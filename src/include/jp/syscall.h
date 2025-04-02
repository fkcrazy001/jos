#pragma once

#include <jp/types.h>

#if 0
#include <asm/unistd_32.h> //keep the same in linux
#endif
typedef enum {
    SYS_NR_TEST,
    SYS_NR_EXIT = 1,
    SYS_NR_FORK = 2,
    SYS_NR_WRITE = 4,
    SYS_NR_OPEN = 5,
    SYS_NR_CLOSE = 6,
    SYS_NR_WAITPID = 7,
    SYS_NR_CREATE = 8,
    SYS_NR_LINK = 9,
    SYS_NR_UNLINK = 10,
    SYS_NR_TIME = 13,
    SYS_NR_GETPID = 20,
    SYS_NR_MKDIR = 39,
    SYS_NR_RMDIR = 40,
    SYS_NR_BRK= 45,
    SYS_NR_UMASK=60,
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

mode_t umask(mode_t mask);

int rmdir(const char *path);
int mkdir(const char *path, uint16 mode);

int unlink(const char *path);
int link(const char *path, const char *new_path);

int close(int fd);

int open(const char *path, int flags, int mode);
int creat(const char *path, int mode);