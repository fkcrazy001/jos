#include <jp/task.h>
#include <jp/task.h>
#include <jp/printk.h>
#include <jp/debug.h>

#define PAGE_SIZE 0x1000

task_t* a = (task_t*)0x1000;
task_t* b = (task_t*)0x2000;

extern void task_switch(task_t *tsk);

task_t *running_task(void)
{
    asm volatile (
        "movl %esp, %eax\n"
        "andl $0xfffff000, %eax\n");
}

#define current running_task()

void yield(void)
{
    task_t *nxt = (current == a) ? b:a;
    task_switch(nxt);
}

static void task_create(task_t* task, task_func fn)
{
    u32 stack = (u32)task + PAGE_SIZE;
    stack -= sizeof(task_frame_t);
    task_frame_t *tf = stack;
    tf->ebx = 0x11111111;
    tf->esi = 0x22222222;
    tf->edi = 0x33333333;
    tf->ebp = task_create; // trace use
    tf->eip = fn;
    //tf->arg = arg;

    task->stk = stack; // lower down
}

static _ofp void func1(void)
{
    asm volatile("sti\n");
    while (1)
    {
        printk("func1\n");
    }
}

static _ofp void func2(void)
{
    asm volatile("sti\n");
    while (1)
    {
        printk("func2\n");
    }
}

void task_init(void)
{
    task_create(a, func1);
    task_create(b, func2);
    yield();
}
