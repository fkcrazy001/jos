#include <jp/types.h>
#include <jp/debug.h>
#include <jp/assert.h>

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

static u32 sys_test(u32 param1, u32 param2, u32 param3)
{
    DEBUGK("syscall test... p1 = %d, p2=%d, p3=%d\n", param1, param2, param3);
    return 255;
}

void syscall_init(void)
{
    for (int i=0;i<SYSCALL_SIZE;++i) {
        syscall_table[i] = (syscall_t)sys_default;
    }
    syscall_table[0] = sys_test;
}