#pragma once

#include <jp/list.h>
#include <jp/task.h>

typedef struct mutex {
    bool value;
    list_node_t waiter;
    // owner for debug
    task_t *owner;
}mutex_t;

void mutex_init(mutex_t *mtx);
void mutex_lock(mutex_t *mtx);
void mutex_unlock(mutex_t *mtx);