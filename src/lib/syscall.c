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

u32 test(void)
{
    return _syscall0(SYS_NR_TEST);
}

void yield(void)
{
    _syscall0(SYS_NR_YIELD);
}