#include <jp/task.h>
#include <jp/printk.h>
#include <jp/debug.h>
#include <jp/memory.h>
#include <jp/assert.h>
#include <jp/interrupt.h>
#include <jp/string.h>
#include <jp/bitmap.h>
#include <jp/joker.h>
#include <jp/clock.h>

extern bitmap_t kernel_map;

extern void task_switch(task_t *tsk);

#define NR_TASKS 64
static task_t *task_table[NR_TASKS];
static list_node_t default_block_list=LIST_INIT(&default_block_list);
static list_node_t sleep_list=LIST_INIT(&sleep_list);
static task_t *idle_task;
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
    if (!task && state == TASK_READY)
        task = idle_task;
    return task;
}

/// @brief when calling this func, @current will be blocked on no cases
///        if no ready task is found, will cause an assert failure
/// @param  
static inline void do_schedule(void)
{
    assert(!get_interrupt_state());
    task_t *now = current;
    task_t *next = task_search(TASK_READY);
    // if (next == NULL)
    //     return;
    assert(next != NULL);
    assert(next->magic == J_MAGIC);
    if (now->state == TASK_RUNNING)
        now->state = TASK_READY;
    next->state = TASK_RUNNING;
    task_switch(next);
}

void task_block(task_t *task, list_node_t *blist, task_state_e state)
{
    assert(!get_interrupt_state()); // in disable state
    assert(node_is_init(&task->node));
    
    if (blist == NULL) {
        blist = &default_block_list;
    }
    list_add(blist, &task->node);
    assert(state!=TASK_RUNNING && state!=TASK_DIED && state != TASK_READY);
    task->state = state;
    if (current == task)
        do_schedule();
}
void task_unblock(task_t *task)
{
    assert(!get_interrupt_state()); // in disable state
    list_del(&task->node);
    assert(node_is_init(&task->node));
    task->state = TASK_READY;
}

void task_sleep(u32 ms)
{
    assert(!get_interrupt_state());
    u32 ticks = ms /jiffy;
    ticks = ticks>0?ticks:1;
    
    task_t *pos = NULL, *now = current;
    assert(node_is_init(&now->node));
    now->ticks = jiffies + ticks;
    list_for_each(pos, &sleep_list, node) {
        if (pos->ticks > now->ticks) {
            break;
        }
    }
    list_insert_before(&pos->node, &now->node);
    current->state = TASK_SLEEP;
    do_schedule(); // to other thread
}

void task_wakeup(void)
{
    assert(!get_interrupt_state());// in if disable
    task_t *task, *next;
    list_for_each_safe(task, next, &sleep_list, node) {
        if (task->ticks > jiffies) {
            break; // min task is still sleeping
        }
        task->ticks = 1; // 1 for schedule assert
        task_unblock(task); // use unblock here just for convinence
    }
}

void schedule(void)
{
    task_t *now = current;
    assert(now->magic == J_MAGIC);

    now->jiffies = jiffies;
    assert(now->ticks); // avoid overflow
    if (--now->ticks > 0) {
        return;
    }
    now->ticks = now->priority;
    do_schedule();
}

void task_yield(void)
{
    do_schedule();
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
    task->pde = KERNEL_PAGE_DIR;
    task->uid = uid;
    task->vmap = &kernel_map;
    node_init(&task->node);
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
// void thread_a(void)
// {
//     set_interrupt_state(true);
//     while (true)
//     {
//         printk("A");
//         test();
//     }
    
// }

extern void idle_thread(void);
extern void init_thread(void);
extern void test_thread(void);

void task_init(void)
{
    task_setup();
    idle_task = task_create(idle_thread, "idle", 1, KERNEL_USER); // lowest priority
    assert(task_create(init_thread, "initd", 5, NORMAL_USER));
    assert(task_create(test_thread, "test", 5, NORMAL_USER));
}
