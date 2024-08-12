#include <jp/types.h>
#include <jp/debug.h>
#include <jp/assert.h>
#include <jp/syscall.h>
#include <jp/task.h>
#include <jp/console.h>
#include <jp/memory.h>

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

static u32 sys_test(void)
{
    return 255;
}

static int32_t sys_write(u32 fd, u32 buf, u32 len)
{
    // @todo buf is user va, transfer to pa
    if (fd == stdout || fd == stderr) {
        return console_write((char*)buf, len);
    }
    panic("write!!!");
    return -1;
}

extern void task_yield(void);
extern time_t sys_time(void);
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
}