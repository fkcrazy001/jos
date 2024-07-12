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

static lock_t lock;

void init_thread(void)
{
    lock_init(&lock);
    set_interrupt_state(true);
    while (true)
    {
        lock_up(&lock);
        lock_up(&lock);
        DEBUGK("task initd....\n");
        lock_down(&lock);
        lock_down(&lock);
        // sleep(500);
    }
    
}

void test_thread(void)
{
    set_interrupt_state(true);
    u32 counter = 0;
    while (true)
    {
        lock_up(&lock);
        lock_up(&lock);
        DEBUGK("task test %d....\n", counter++);
        lock_down(&lock);
        lock_down(&lock);
        // sleep(1000);
    }
    
}