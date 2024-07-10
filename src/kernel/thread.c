#include <jp/interrupt.h>
#include <jp/debug.h>
#include <jp/syscall.h>


void idle_thread(void)
{
    set_interrupt_state(true);
    u32 counter = 0;
    while (true)
    {
        // DEBUGK("idle task... %d\n", counter++);
        asm volatile (
            "sti\n" // 打开中断
            "hlt\n" // 关闭cpu，进入暂停状态，等待外中断
        );
        yield();
    }
    
}

void init_thread(void)
{
    set_interrupt_state(true);
    while (true)
    {
        DEBUGK("task initd....\n");
        sleep(500);
    }
    
}

void test_thread(void)
{
    set_interrupt_state(true);
    u32 counter = 0;
    while (true)
    {
        DEBUGK("task test %d....\n", counter++);
        sleep(1000);
    }
    
}