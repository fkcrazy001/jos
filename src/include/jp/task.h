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
    list_node_t  node; // this node was for task to sleep, block and etc
    task_state_e state;
    u32 priority;
    u32 ticks; // 当处于 running 态时，ticks代表剩余时间片；当处于sleeping态时，ticks代表需要被唤醒的时间戳
    u32 jiffies;
    u8  name[TASK_NAME_LEN];
    u32 uid;
    int32_t pid; // process id
    int32_t ppid; // parent pid
    u32 pde;  // process page dir entry
    u32 brk; // process heap max addr
    struct bitmap_t *vmap;//va map
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

// 中断帧
typedef struct intr_frame {
    u32 vector;
    u32 edi;
    u32 esi;
    u32 ebp;
    u32 dummy_esp; // 没什么用
    u32 ebx;
    u32 edx;
    u32 ecx;
    u32 eax;
    u32 gs;
    u32 fs;
    u32 es;
    u32 ds;
    u32 vector0;
    u32 error;
    u32 eip;
    u32 cs;
    u32 eflags;
    u32 esp; // user esp
    u32 ss; // user ss
} intr_frame_t;

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

void task_sleep(u32 ms);
void task_wakeup(void);

int32_t sys_getpid(void);
int32_t sys_getppid(void);

int32_t task_fork(void);
#endif