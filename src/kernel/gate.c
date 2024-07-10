#include <jp/types.h>
#include <jp/debug.h>
#include <jp/assert.h>
#include <jp/syscall.h>
#include <jp/task.h>

#define SYSCALL_SIZE 64
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
    if (!task) {
        task = current;
        task_block(task, NULL, TASK_BLOCKED);
    } else {
        task_unblock(task);
        task = NULL;
    }
    return 255;
}

extern void task_yield(void);

void syscall_init(void)
{
    for (int i=0;i<SYSCALL_SIZE;++i) {
        syscall_table[i] = (syscall_t)sys_default;
    }
    syscall_table[SYS_NR_TEST] = (syscall_t)sys_test;
    syscall_table[SYS_NR_SLEEP] = (syscall_t)task_sleep;
    syscall_table[SYS_NR_YIELD] = (syscall_t)task_yield;
}