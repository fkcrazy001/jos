#include <jp/types.h>
#include <jp/debug.h>
#include <jp/assert.h>
#include <jp/syscall.h>
#include <jp/task.h>
#include <jp/console.h>
#include <jp/memory.h>
#include <jp/device.h>

#define SYSCALL_SIZE 256
// args above p3 is common args pushed by syscall_handler
typedef u32 (*syscall_t)(u32 param1, u32 param2, u32 param3, ...);
syscall_t syscall_table[SYSCALL_SIZE];

void syscall_check(u32 nr)
{
    if (nr >= SYSCALL_SIZE) {
        panic("invalid syscall nr");
    }
}

static u32 sys_default(void)
{
    DEBUGK("syscall to be implement...\n");
    return 0;
}

static task_t *task = NULL;
#include <jp/buffer.h>
#include <jp/string.h>

static u32 sys_test(void)
{   
    // device_t *dev = device_find(DEV_KEYBOARD, 0);
    // char ch;
    // device_read(dev->dev, &ch, 1, 0, 0);
    // dev = device_find(DEV_CONSOLE, 0);
    // device_write(dev->dev, &ch, 1, 0, 0);
    DEBUGK("syscall 0 called");
    return 255;
}

extern int console_write();

extern void task_yield(void);
extern time_t sys_time(void);
extern mode_t sys_umask(mode_t umask);
extern int sys_mkdir(u32 path, u32 mode);
extern int sys_rmdir(u32 path);
extern int sys_readdir(fd_t fd, u32 buf_addr, u32 count);
extern int sys_link(u32 path, u32 new_path);
extern int sys_unlink(u32 path);

extern int sys_open(u32 path, int flags, int mode);
extern int sys_create(u32 path, int mode);
extern int sys_close(fd_t fd);

extern int sys_write(fd_t fd, u32 buf_addr, u32 len);
extern int sys_read(fd_t fd, u32 buf_addr, u32 len);
extern int sys_lseek(fd_t fd, int offset, int whence);

extern int sys_chdir(u32 newpath);
extern int sys_chroot(u32 new_root);
extern char* sys_getcwd(u32 user_ptr, size_t size);
void syscall_init(void)
{
    for (int i=0;i<SYSCALL_SIZE;++i) {
        syscall_table[i] = (syscall_t)sys_default;
    }
    syscall_table[SYS_NR_TEST] = (syscall_t)sys_test;
    syscall_table[SYS_NR_SLEEP] = (syscall_t)task_sleep;
    syscall_table[SYS_NR_YIELD] = (syscall_t)task_yield;
    syscall_table[SYS_NR_READ] = (syscall_t)sys_read;
    syscall_table[SYS_NR_WRITE] = (syscall_t)sys_write;
    syscall_table[SYS_NR_BRK] = (syscall_t)sys_brk;

    syscall_table[SYS_NR_GETPID] = (syscall_t)sys_getpid;
    syscall_table[SYS_NR_GETPPID] = (syscall_t)sys_getppid;
    syscall_table[SYS_NR_FORK] = (syscall_t)task_fork;
    syscall_table[SYS_NR_EXIT] = (syscall_t)task_exit;
    syscall_table[SYS_NR_WAITPID] = (syscall_t)task_waitpid;
    syscall_table[SYS_NR_TIME] = (syscall_t)sys_time;
    syscall_table[SYS_NR_UMASK] = (syscall_t)sys_umask;
    
    syscall_table[SYS_NR_MKDIR] = (syscall_t)sys_mkdir;
    syscall_table[SYS_NR_RMDIR] = (syscall_t)sys_rmdir;

    syscall_table[SYS_NR_LINK] = (syscall_t)sys_link;
    syscall_table[SYS_NR_UNLINK] = (syscall_t)sys_unlink;

    syscall_table[SYS_NR_OPEN] = (syscall_t)sys_open;
    syscall_table[SYS_NR_CLOSE] = (syscall_t)sys_close;
    syscall_table[SYS_NR_CREATE] = (syscall_t)sys_create;

    syscall_table[SYS_NR_LSEEK] = (syscall_t)sys_lseek;
    syscall_table[SYS_NR_READDIR] = (syscall_t)sys_readdir;

    syscall_table[SYS_NR_CHDIR]= (syscall_t)sys_chdir;
    syscall_table[SYS_NR_CHROOT] = (syscall_t)sys_chroot;
    syscall_table[SYS_NR_GETCWD] = (syscall_t)sys_getcwd;

    syscall_table[SYS_NR_CLEAR] = (syscall_t)console_clear;
}