#include <jp/task.h>
#include <jp/printk.h>
#include <jp/debug.h>
#include <jp/memory.h>
#include <jp/assert.h>
#include <jp/interrupt.h>
#include <jp/string.h>
#include <jp/bitmap.h>
#include <jp/joker.h>

extern bitmap_t kernel_map;

extern void task_switch(task_t *tsk);

#define NR_TASKS 64
static task_t *task_table[NR_TASKS];

static task_t* get_free_task(void)
{
    for (int i=0; i<NR_TASKS; ++i) {
        if (task_table[i] == NULL) {
            // @todo free_kpage
            task_table[i] = (task_t*)alloc_kpage(1);
            return task_table[i];
        }
    }
    panic("can't create more jobs");
}

/// @brief 找到除了自己之外的优先级最高的（tick最大的，或者jiffies最小的，一种调度算法吧）
///        call me in interrupt disable state
/// @param state target state
/// @return task_
static task_t* task_search(task_state_e state)
{
    assert(!get_interrupt_state());
    task_t *task = NULL;
    task_t *tmp = NULL;
    task_t *now = current;
    for (int i=0;i<NR_TASKS;++i) {
        tmp = task_table[i];
        if (tmp == NULL || tmp == now)
            continue;
        if (tmp->state != state)
            continue;
        if (!task || task->ticks < tmp->ticks || task->jiffies > tmp->jiffies)
            task = tmp;
    }
    return task;
}

void schedule(void)
{
    task_t *now = current;
    task_t *next = task_search(TASK_READY);
    if (next == NULL)
        return;
    assert(next != NULL);
    assert(next->magic == J_MAGIC);
    if (now->state == TASK_RUNNING)
        now->state = TASK_READY;
    next->state = TASK_RUNNING;
    task_switch(next);
}

/***
 * 4k: task_frame
 *    eip
 *    ebp
 *    ebx
 *    esi
 *    edi
 *    ...
 *    ...
 *    stack(esp)
 * 0: pcb 
 */
static task_t* task_create(task_func fn,  const char* name, u32 priority, u32 uid)
{
    task_t *task = get_free_task();
    memset(task, 0, PAGE_SIZE);
    u32 stack = (u32)task + PAGE_SIZE;
    stack -= sizeof(task_frame_t);
    task_frame_t *tf = (task_frame_t*)stack;
    tf->ebx = 0x11111111;
    tf->esi = 0x22222222;
    tf->edi = 0x33333333;
    tf->ebp = 0; // trace use
    tf->eip = fn;
    //tf->arg = arg;

    task->magic = J_MAGIC;
    strncpy(task->name, name, sizeof(task->name));
    task->stk = (u32*)stack; // lower down
    task->priority = priority;
    task->ticks = priority;
    task->jiffies = 0;
    task->state = TASK_READY;
    task->uid = uid;
    task->vmap = &kernel_map;
    return task;
}

static void task_setup()
{
    task_t *t = current;
    t->magic = J_MAGIC;
    t->ticks = 1; // this task never come back
    memset(task_table, 0, sizeof(task_table));
}

/// stk back trace
// 0. interrupt handler
// 1. pit handler: clock handler
// 2. schedule_parta: push current task ROP in stk
// 2.5 schedule_partb task stack switch
// 3. schedule_partc: pop next task's ROP
// 4. iret from interrupt handler
// 5. finally thread_a
void thread_a(void)
{
    set_interrupt_state(true);
    while (true)
    {
        printk("A");
    }
    
}

void thread_b(void)
{
    set_interrupt_state(true);
    while (true)
    {
        printk("B");
    }
    
}

void thread_c(void)
{
    set_interrupt_state(true);
    while (true)
    {
        printk("C");
    }
    
}

void task_init(void)
{
    task_setup();
    task_create(thread_a, "a", 5, KERNEL_USER);
    task_create(thread_b, "b", 5, KERNEL_USER);
    task_create(thread_c, "c", 5, KERNEL_USER);
}
