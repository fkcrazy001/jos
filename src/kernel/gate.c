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
    inode_t *inode = inode_open("/world.txt", O_RDWR | O_CREATE, 0755);
    assert(inode);
    char *buf = (char*)alloc_kpage(1);
    assert(buf);
    int i = inode_read(inode, buf, PAGE_SIZE, 0);
    memset(buf, 'A', PAGE_SIZE);
    inode_write(inode, buf, PAGE_SIZE, 0);
    iput(inode);
    free_kpage((u32)buf, 1);
    device_t *dev = device_find(DEV_KEYBOARD, 0);
    char ch;
    device_read(dev->dev, &ch, 1, 0, 0);
    dev = device_find(DEV_CONSOLE, 0);
    device_write(dev->dev, &ch, 1, 0, 0);
    return 255;
}

extern int console_write();

static int32_t sys_write(u32 fd, u32 buf, u32 len)
{
    // @todo buf is user va, transfer to pa
    if (fd == stdout || fd == stderr) {
        return console_write(0, (char*)buf, len);
    }
    panic("write!!!");
    return -1;
}

extern void task_yield(void);
extern time_t sys_time(void);
extern mode_t sys_umask(mode_t umask);
extern int sys_mkdir(u32 path, u32 mode);
extern int sys_rmdir(u32 path);
extern int sys_link(u32 path, u32 new_path);
extern int sys_unlink(u32 path);
void syscall_init(void)
{
    for (int i=0;i<SYSCALL_SIZE;++i) {
        syscall_table[i] = (syscall_t)sys_default;
    }
    syscall_table[SYS_NR_TEST] = (syscall_t)sys_test;
    syscall_table[SYS_NR_SLEEP] = (syscall_t)task_sleep;
    syscall_table[SYS_NR_YIELD] = (syscall_t)task_yield;
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
}