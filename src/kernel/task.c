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
#include <jp/global.h>
#include <jp/arena.h>
#include <jp/stdlib.h>

extern bitmap_t kernel_map;
extern tss_t tss;
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
            task_t *t = (task_t*)alloc_kpage(1);
            memset(t, 0, PAGE_SIZE);
            t->pid = i;
            task_table[i] = t;
            return t;
        }
    }
    panic("can't create more jobs");
}

int32_t sys_getpid(void)
{
    return current->pid;
}

int32_t sys_getppid(void)
{
    return current->ppid;
}

fd_t task_get_fd(task_t *task)
{
    fd_t fd;
    // @todo: skip stdin stdout, stderr ?
    for (fd = 3; fd < TASK_MAX_OPEN_FILE; ++fd) {
        if (!task->files[fd]) {
            return fd;
        }
    }
    panic("task %s: too many open files", task->name);
}

void task_put_fd(task_t *task, fd_t fd)
{
    assert(fd < TASK_MAX_OPEN_FILE);
    if (fd < 3)
        return;
    assert(task->files[fd]);
    task->files[fd] = NULL;
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

static void task_activate(task_t *t)
{
    if (t->pde != get_cr3()) {
        // 这里已经切换成新的进程的pde了
        // 也就意味着新旧进程的pde对内核空间的映射必须是一致的
        // 不然内核接下来就会出现PF，是灾难性的
        set_cr3(t->pde);
    }
    if (t->uid != KERNEL_USER) {
        // 意味着下一次中断发生， push interrupt frame总是在t那一页最开始的地方
        // 有点奇怪的，但是内核线程也没有什么栈上的数据要保存，所以应该无所谓
        tss.esp0 = (u32)t + PAGE_SIZE; 
    }
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
    task_activate(next);
    task_switch(next);
}

void task_block(task_t *task, list_node_t *blist, task_state_e state)
{
    // assert(!get_interrupt_state()); // in disable state
    bool intr = interrupt_disable();
    assert(node_is_init(&task->node));
    
    if (blist == NULL) {
        blist = &default_block_list;
    }
    list_add(blist, &task->node);
    assert(state!=TASK_RUNNING && state!=TASK_DIED && state != TASK_READY);
    task->state = state;
    if (current == task)
        do_schedule();
    set_interrupt_state(intr);
}
void task_unblock(task_t *task)
{
    assert(!get_interrupt_state()); // in disable state
    assert(!node_is_init(&task->node));
    list_del(&task->node);
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
    task->gid = 0; // @todo group id
    task->vmap = &kernel_map;
    task->iroot = get_root_inode();
    task->ipwd = get_root_inode();
    task->pwd = (void*)alloc_kpage(1);
    strcpy(task->pwd, "/");

    task->umask = (mode_t)0022;
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

// 由于这个函数会对当前任务的栈顶部进行修改，调用者需要满足以下条件
// 1. 在当前任务栈顶留出足够的空间(至少sizeof(intr_frame_t))
// 2. 这个函数永远不会返回
// 3. 调用时需要cpu中断已经被关闭
#define USER_TASK_BITMAP_MAX_LENGTH PAGE_SIZE
void task_to_user_mode(task_func f)
{
    task_t *t = current;
    void *buf = (void*)kmalloc(USER_TASK_BITMAP_MAX_LENGTH + sizeof(bitmap_t)); // @todo 一页内存bitmap(4k*4k*8)仅支持128M va
    t->vmap = buf;
    bitmap_init(t->vmap, (char*)(t->vmap + 1), USER_TASK_BITMAP_MAX_LENGTH, KERNEL_MEMORY_SIZE/PAGE_SIZE);
    
    t->pde = (u32)copy_pde();
    set_cr3(t->pde);

    // brk
    t->brk = KERNEL_MEMORY_SIZE;

    u32 intr_f_p = (u32)t + PAGE_SIZE;
    intr_f_p -= sizeof(intr_frame_t);
    intr_frame_t *iframe = (intr_frame_t*)intr_f_p;
    iframe->vector = iframe->vector0 = 0x20;
    iframe->edi = 1;
    iframe->esi = 2;
    iframe->ebp = 0;
    iframe->dummy_esp = 4;
    iframe->ebx = 5;
    iframe->edx = 6;
    iframe->ecx = 7;
    iframe->eax = 8;

    iframe->gs = 0;
    iframe->ds = USER_DATA_SELECTOR;
    iframe->fs = USER_DATA_SELECTOR;
    iframe->ss = USER_DATA_SELECTOR;
    iframe->es = USER_DATA_SELECTOR;
    iframe->cs = USER_CODE_SELECTOR;

    iframe->error = J_MAGIC;
    iframe->eip = (u32)f;
    iframe->eflags = (0<<12)|0b10|1<<9; // intr enable | 0b10 fixed| IOPL=0
    iframe->esp = USER_STK_TOP;
    asm volatile("movl %0, %%esp\n"
                "jmp interrupt_exit\n"::"m"(iframe));
}

extern void interrupt_exit(void);

static void task_build_stk(task_t *t)
{
    u32 addr = (u32)t + PAGE_SIZE;
    addr -= sizeof(intr_frame_t);
    intr_frame_t *iframe = (intr_frame_t*)addr;
    iframe->eax = 0; // ret value

    addr -= sizeof(task_frame_t);
    task_frame_t *tf = (task_frame_t*)addr;
    tf->ebp = 0x5a5a5a5a;
    tf->ebx = 0x5a5a5a5a;
    tf->edi = 0x5a5a5a5a;
    tf->esi = 0x5a5a5a5a;

    // first schedule goto interrupt_exit
    tf->eip = interrupt_exit; 

    t->stk = (void*)addr;
}

int32_t task_fork(void)
{
    task_t *t = current;
    assert(t->state == TASK_RUNNING && t->uid == NORMAL_USER);
    task_t *child = get_free_task();
    int32_t pid = child->pid;
    memcpy(child, t, PAGE_SIZE);

    child->pid = pid;
    child->ppid = t->pid;
    child->ticks = child->priority;
    child->state = TASK_READY;
    
    // vma
    size_t size = t->vmap->length + sizeof(bitmap_t);
    void *buf = (void*)kmalloc(size);
    child->vmap = buf;
    memcpy(child->vmap, t->vmap, size);
    child->vmap->bits = (char*)(child->vmap+1);

    child->pde = (u32)copy_pde();

    // pwd
    child->pwd = (char*)alloc_kpage(1);
    strncpy(child->pwd, t->pwd, PAGE_SIZE);
    child->iroot = get_root_inode();
    child->ipwd = get_root_inode();

    // file ref
    for (size_t fd = 0; fd < TASK_MAX_OPEN_FILE; ++fd) {
        file_t *file = child->files[fd];
        if (file) {
            file->count += 1;
        }
    }
    
    task_build_stk(child); // build child task stack

    // do_schedule(); // ?
    return child->pid;
}

extern int sys_close(fd_t fd);


void task_exit(u32 status)
{
    task_t *t = current;

    assert(node_is_init(&t->node) && t->state == TASK_RUNNING);

    t->state = TASK_DIED;
    t->status = status;
    
    task_t *parent = task_table[t->ppid];
    if (!parent || parent->state == TASK_DIED) {
        t->ppid = 1; // let initd thread be parent
        parent = task_table[t->ppid];
    }
    assert(parent);
    if (parent->state == TASK_WAITING && (parent->wpid == t->pid || parent->wpid == -1)) {
        // parent is waiting for me to die...
        task_unblock(parent);
    }
    // file ref
    for (fd_t fd = 0; fd < TASK_MAX_OPEN_FILE; ++fd) {
        file_t *file = t->files[fd];
        if (file) {
            sys_close(fd);
        }
    }
    iput(t->ipwd);
    iput(t->iroot);
    free_kpage((u32)t->pwd, 1);
    free_pde();
    kfree(t->vmap);
    // @todo: wait() syscall, parent get status and free `t`
    DEBUGK("task %s, pid %d exit...\n", t->name, t->pid);
    do_schedule();
}

int32_t task_waitpid(int32_t pid, u32 *status)
{
    task_t *now = current;
    size_t i = 2;
again:
    for (; i < NR_TASKS; ++i) {
        task_t *t = task_table[i];
        if (!t || t->ppid != now->pid || (pid != -1 && t->pid != pid))
            continue;
        // find child, but child not died yet
        if (t->state != TASK_DIED) {
            now->wpid = t->pid;
            break;
        }
        task_table[i] = NULL;
        int32_t cpid = t->pid;
        if (USERADDR_W_CHECK(now, status))
            *status = t->status;
        free_kpage((u32)t, 1);
        return cpid;
    }
    //  not child found or child hasn't died yet
    task_block(now, NULL, TASK_WAITING);
    goto again;
}

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
