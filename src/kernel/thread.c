#include <jp/interrupt.h>
#include <jp/debug.h>
#include <jp/syscall.h>
#include <jp/mutex.h>
#include <jp/printk.h>
#include <jp/stdio.h>
#include <jp/arena.h>
#include <jp/stdlib.h>
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

extern u32 keyboard_read(char *buf, u32 count);
void task_to_user_mode(task_func f);
void user_init_thread(void)
{
    u32 counter;
    char ch;
    while (true)
    {
        // BMB;
        // asm volatile("in $0x64, %al\n");
        // printk("user mode\n");
        // @todo user cant access kernel mm
        *(char*)0xB8000 = 'b';
        int32_t pid = fork();
        if (pid > 0) {
            counter=2;
            printf("parent process, pid %d, getpid %d, ppid %d\n", pid, getpid(), getppid());
        } else if (pid == 0) {
            counter=3;
            printf("child process, pid %d, getpid %d, ppid %d\n", pid, getpid(), getppid());
        } else {
            printf("fork failed \n");
        }
        printf("counter %d\n", counter++);
        hang();
        sleep(1000);
    }
}

void init_thread(void)
{
    char tmp[100];
    task_to_user_mode(user_init_thread);
}

void test_thread(void)
{
    set_interrupt_state(true);
    u32 counter = 0;
    while (true)
    {
        // lock_up(&lock);
        // lock_up(&lock);
        DEBUGK("task test %d, pid %d, ppid %d....\n", counter++, getpid(), getppid());
        // lock_down(&lock);
        // lock_down(&lock);
        sleep(5000);
    }
    
}