#pragma once

#include <jp/list.h>

typedef struct mutex {
    bool value;
    list_node_t waiter;
}mutex_t;

void mutex_init(mutex_t *mtx);
void mutex_lock(mutex_t *mtx);
void mutex_unlock(mutex_t *mtx);

struct task;

typedef struct lock {
    mutex_t mtx;
    u32 repeat;
    struct task *owner;
} lock_t;
// 这只是一个可重入的互斥锁
void lock_init(lock_t *mtx);
void lock_up(lock_t *mtx);
void lock_down(lock_t *mtx);