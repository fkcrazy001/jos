#include <jp/interrupt.h>
#include <jp/debug.h>
#include <jp/syscall.h>
#include <jp/mutex.h>

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

static mutex_t mtx;

void init_thread(void)
{
    mutex_init(&mtx);
    set_interrupt_state(true);
    while (true)
    {
        mutex_lock(&mtx);
        DEBUGK("task initd....\n");
        mutex_unlock(&mtx);
        // sleep(500);
    }
    
}

void test_thread(void)
{
    set_interrupt_state(true);
    u32 counter = 0;
    while (true)
    {
        mutex_lock(&mtx);
        DEBUGK("task test %d....\n", counter++);
        mutex_unlock(&mtx);
        // sleep(1000);
    }
    
}