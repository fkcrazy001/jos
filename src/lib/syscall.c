#include <jp/syscall.h>


static __always_inline u32 _syscall0(u32 nr)
{
    u32 ret;
    asm volatile(
        "int $0x80\n"
        : "=a"(ret)
        : "a"(nr));
    return ret;
}

static __always_inline u32 _syscall1(u32 nr, u32 arg)
{
    u32 ret;
    asm volatile(
        "int $0x80\n"
        : "=a"(ret)
        : "a"(nr), "b"(arg));
    return ret;
}

static __always_inline u32 _syscall2(u32 nr, u32 arg1, u32 arg2)
{
    u32 ret;
    asm volatile(
        "int $0x80\n"
        : "=a"(ret)
        : "a"(nr), "b"(arg1), "c"(arg2));
    return ret;
}

static __always_inline u32 _syscall3(u32 nr, u32 arg1, u32 arg2, u32 arg3)
{
    u32 ret;
    asm volatile(
        "int $0x80\n"
        : "=a"(ret)
        : "a"(nr), "b"(arg1), "c"(arg2), "d"(arg3));
    return ret;
}

u32 test(void)
{
    return _syscall0(SYS_NR_TEST);
}

void yield(void)
{
    _syscall0(SYS_NR_YIELD);
}

void sleep(u32 ms)
{
    _syscall1(SYS_NR_SLEEP, ms);
}

// addr must be page start
int32_t brk(char *addr)
{
    return _syscall1(SYS_NR_BRK, (u32)addr);
}

int32_t getpid(void)
{
    return _syscall0(SYS_NR_GETPID);
}
int32_t getppid(void)
{
    return _syscall0(SYS_NR_GETPPID);
}

int32_t fork(void)
{
    return _syscall0(SYS_NR_FORK);
}

void exit(int status)
{
    (void)_syscall1(SYS_NR_EXIT, (u32)status);
}

int32_t waitpid(int32_t pid, int *status)
{
    return _syscall2(SYS_NR_WAITPID, (u32)pid, (u32)status);
}

time_t time(void)
{
    return _syscall0(SYS_NR_TIME);
}

mode_t umask(mode_t mask)
{
    return _syscall1(SYS_NR_UMASK, (u32)mask);
}

int rmdir(const char *path)
{
    return _syscall1(SYS_NR_RMDIR, (u32)path);
}

int mkdir(const char *path, uint16 mode)
{
    return _syscall2(SYS_NR_MKDIR, (u32)path, (u32)mode);
}

int unlink(const char *path)
{
    return _syscall1(SYS_NR_UNLINK, (u32)path);
}

int link(const char *path, const char *new_path)
{
    return _syscall2(SYS_NR_LINK, (u32)path, (u32)new_path);
}

int close(int fd)
{
    return _syscall1(SYS_NR_CLOSE, (u32)fd);
}

int open(const char *path, int flags, int mode)
{
    return _syscall3(SYS_NR_OPEN, (u32)path, (u32)flags, (u32)mode);
}

int creat(const char *path, int mode)
{
    return _syscall2(SYS_NR_CREATE, (u32)path, (u32)mode);
}

int32_t read(int fd, char *buf, size_t len)
{
    return _syscall3(SYS_NR_READ, (u32)fd, (u32)buf, (u32)len);
}

int32_t write(int fd, const char *buf, size_t len)
{
    return _syscall3(SYS_NR_WRITE, (u32)fd, (u32)buf, (u32)len);
}

int32_t lseek(int fd, int offset, int mode)
{
    return _syscall3(SYS_NR_LSEEK, (u32)fd, (u32)offset, (u32)mode);
}

char *getcwd(char *path, size_t size)
{
    return (char*)_syscall2(SYS_NR_GETCWD, (u32)path, (u32)size);
}

int chroot(const char *path)
{
    return _syscall1(SYS_NR_CHROOT, (u32)path);
}

int chdir(const char *path)
{
    return _syscall1(SYS_NR_CHDIR, (u32)path);
}

int readdir(int fd, void *buf, int cnt)
{
    return _syscall3(SYS_NR_READDIR, (u32)fd, (u32)buf, (u32)cnt);
}

int clear(void)
{
    return _syscall0(SYS_NR_CLEAR);
}