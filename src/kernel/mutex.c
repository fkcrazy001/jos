#include <jp/mutex.h>
#include <jp/task.h>
#include <jp/interrupt.h>


void mutex_init(mutex_t *mtx)
{
    assert(mtx != NULL);
    mtx->value = false; // no one holds this
    list_init(&mtx->waiter);
}

void mutex_lock(mutex_t *mtx)
{
    task_t *t = current;
    assert(t->state == TASK_RUNNING);
    bool intr = interrupt_disable();
    while (mtx->value == true)
    {
        task_block(t, &mtx->waiter, TASK_BLOCKED);
    }

    assert(mtx->value == false);
    mtx->value++;
    assert(mtx->value == true);

    set_interrupt_state(intr);
}

void mutex_unlock(mutex_t *mtx)
{
    assert(mtx->value == true);
    bool intr = interrupt_disable();

    mtx->value = false;
    if (!list_empty(&mtx->waiter)) {
        task_unblock(container_of(task_t, node, mtx->waiter.next));
        // 最好yield一下，防止别的进程被饿死
        task_yield(); 
    }

    set_interrupt_state(intr);
}

void lock_init(lock_t *l)
{
    l->repeat = 0;
    l->owner = NULL;
    mutex_init(&l->mtx);
}

void lock_up(lock_t *l)
{
    task_t *t = current;
    if (l->owner != t) {
        mutex_lock(&l->mtx);
        assert(l->repeat == 0 && l->owner == NULL);
        l->owner = t;
        l->repeat = 1;
    } else {
        l->repeat++;
    }
}

void lock_down(lock_t *l)
{
    assert(l->owner == current);
    if (--l->repeat > 0)
        return;
    l->owner = NULL;
    mutex_unlock(&l->mtx);
}