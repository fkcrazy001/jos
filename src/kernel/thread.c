#include <jp/interrupt.h>
#include <jp/debug.h>
#include <jp/syscall.h>
#include <jp/mutex.h>
#include <jp/printk.h>
#include <jp/stdio.h>
#include <jp/arena.h>
#include <jp/stdlib.h>
#include <jp/task.h>

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
    u32 counter=0;
    char buf[256];
    fd_t fd = open("/hello.txt", O_RDWR, 0755);
    assert(fd);
    int len = read(fd, buf, 256);
    printf("hello.txt read %s, length %d\n", buf, len);
    close(fd);

    fd = open("/world.txt", O_RDWR|O_CREATE, 0755);
    len = write(fd, (const char *)buf, len);
    close(fd);

    while (true)
    {
        // @todo user cant access kernel mm
        // *(char*)0xB8000 = 'b';
        char ch;
        read(stdin, &ch, 1);
        write(stdout, (const char *)&ch, 1);
        sleep(10);
        // printf("old is %d\n", umask(mask++));
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
    // mkdir("/world.txt", 0755);
    // rmdir("/empty");
    // link("/hello.txt", "world.txt");
    // unlink("/hello.txt");
    while (true)
    {
        sleep(10);
    }
}