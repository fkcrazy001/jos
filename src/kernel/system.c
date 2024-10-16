#include <jp/task.h>

mode_t sys_umask(mode_t mask)
{
    task_t *t = current;
    mode_t old = t->umask;
    t->umask = mask & 0777;
    return old;
}