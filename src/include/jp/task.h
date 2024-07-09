#ifndef __TASK_H__
#define __TASK_H__
#include <jp/types.h>
#include <jp/list.h>

#define KERNEL_USER 0
#define NORMAL_USER 1

#define TASK_NAME_LEN 16

typedef enum task_state {
    TASK_INIT,
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_SLEEP,
    TASK_WAITING,
    TASK_DIED,
} task_state_e;

typedef struct task {
    u32 *stk;
    list_node_t  blk_node;
    task_state_e state;
    u32 priority;
    u32 ticks;
    u32 jiffies;
    u8  name[TASK_NAME_LEN];
    u32 uid;
    u32 pde;  // page dir entry
    struct bitmap_t *vmap;//
    u32 magic;
} task_t;

typedef void (*task_func)(void);

typedef struct task_frame {
    u32 edi;
    u32 esi;
    u32 ebx;
    u32 ebp; 
    task_func eip;
    //u32 padding[1];
    //void *arg; // arg = $esp + 4
} task_frame_t;

void schedule(void);

static inline task_t *running_task(void)
{
    asm volatile (
        "movl %esp, %eax\n"
        "andl $0xfffff000, %eax\n");
}

#define current running_task()

extern void task_yield(void);
extern void task_block(task_t *task, list_node_t *blist, task_state_e state);
extern void task_unblock(task_t *task);
#endif