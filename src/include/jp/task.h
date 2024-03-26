#ifndef __TASK_H__
#define __TASK_H__
#include <jp/types.h>

typedef struct task {
    u32 *stk;
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

void task_init(void);

#endif