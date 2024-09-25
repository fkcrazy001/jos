#include <jp/buffer.h>
#include <jp/memory.h>
#include <jp/mutex.h>
#include <jp/interrupt.h>
#include <jp/debug.h>

// mm layout
/* |buffer_t|buffer_t|...|buffer_t|gap|block|...|block|block|*/
static buffer_t *buf_addr = (buffer_t*)KERNEL_BUFFER_MEM;
static u32 buffer_count = 0;
static char *buf_data_addr = (char*)(KERNEL_BUFFER_MEM+KERNEL_BUFFER_SIZE-BLOCK_SIZE);

#define HASH_TLB_SIZE 31 
static list_node_t buffer_hash_table[HASH_TLB_SIZE];
static lock_t buffer_lock;

// stores tasks waiting for buffer
static list_node_t wait_list = LIST_INIT(&wait_list);
// stores free buffers
static list_node_t free_list = LIST_INIT(&free_list);

union hash_key {
    struct {   
        u32 dev;
        u32 block;
        u32 pid;
    };
    u32 k[3];
};
// buffer_t 处于三种状态
// 1. 内存中
// 2. 使用中：在hash表中
// 3. 空闲中：在hash表和free_list中

static int hash(union hash_key *key)
{
    key->pid = 0; // pid 
    return (key->k[0] ^ key->k[1] ^ key->k[2]) & (HASH_TLB_SIZE-1);
}

static buffer_t *hash_get(dev_t dev, u32 block, u32 pid)
{
    union hash_key k = {
        .block = block,
        .dev = dev,
        .pid = pid,
    };
    int h = hash(&k);
    lock_up(&buffer_lock);
    buffer_t *pos, *tbuffer = NULL;
    list_for_each(pos, &buffer_hash_table[h], hnode) {
        if ( pos->dev == dev && pos->block == block
        /*
             && pos->pid == pid
        */
             ) {
            tbuffer = pos;
            break;
        }
    }
    if (!tbuffer)
        goto out;
    if (list_search(&free_list, &tbuffer->rnode)) {
        list_del(&tbuffer->rnode);
    }
out:
    lock_down(&buffer_lock);
    return tbuffer;
}

static void hash_insert(buffer_t *buf)
{
    union hash_key k = {
        .block = buf->block,
        .dev = buf->dev,
        .pid = buf->pid
    };
    u32 h = hash(&k);
    lock_up(&buffer_lock);
    list_add(&buffer_hash_table[h], &buf->hnode);
    lock_down(&buffer_lock);
}


static void hash_remove(buffer_t *buf)
{
    union hash_key k = {
        .block = buf->block,
        .dev = buf->dev,
        .pid = buf->pid
    };
    u32 h = hash(&k);
    
    assert(list_search(&buffer_hash_table[h], &buf->hnode));
    lock_up(&buffer_lock);
    list_del(&buf->hnode);
    lock_down(&buffer_lock);
}


static buffer_t *get_new_buffer(void)
{
    assert(!get_interrupt_state());
    buffer_t *buf = NULL;
    if ((u32)buf_addr + sizeof(buffer_t) < (u32)buf_data_addr) {
        buf = buf_addr;
        buf->data = (void*)buf_data_addr;
        buf->dev = DEV_NULL;
        buf->block = 0;
        buf->pid = 0;
        buf->count = 0;
        buf->dirty = false;
        buf->valid = false;
        node_init(&buf->rnode);
        node_init(&buf->hnode);
        buf_addr++;
        buf_data_addr -= BLOCK_SIZE;
        DEBUGK("buffer count %d\n", buffer_count);
    }
    return buf;
}

static buffer_t *get_free_buffer(void)
{
    buffer_t *bf = NULL;
    while (true) {
        bf = get_new_buffer();
        if (bf)
            return bf;
        lock_up(&buffer_lock);
        if (!list_empty(&free_list)) {
            bf = container_of(buffer_t, rnode, list_tail_pop(&free_list));
            hash_remove(bf); // this buf will be use by other process to other block, del from origin hashlist
            lock_down(&buffer_lock);
            return bf;
        }
        lock_down(&buffer_lock);
        task_block(current, &wait_list, TASK_BLOCKED);
    }
    panic("can't happened");
}

buffer_t *getblk(dev_t dev, u32 block)
{
    u32 pid = current->pid;
    buffer_t *bf = hash_get(dev, block, pid);
    if (bf) {
        assert(bf->valid);
        return bf;
    }
    bf = get_free_buffer();
    assert(bf && bf->count == 0 && !bf->dirty);
    
    bf->count = 1;
    bf->block = block;
    bf->dev = dev;
    bf->pid = pid;
    hash_insert(bf);
    return bf;
}

buffer_t *bread(dev_t dev, u32 block)
{
    buffer_t *bf = getblk(dev, block);
    assert(bf);
    if (bf->valid) {
        bf->count++;
        return bf;
    }
    device_requset(dev, bf->data, BLOCK_SECS, bf->block * BLOCK_SECS, 0, REQ_READ);
    bf->dirty = false;
    bf->valid = true;
    return bf;
}

void bwrite(buffer_t *buf)
{
    assert(buf);
    if (!buf->dirty)
        return;
    device_requset(buf->dev, buf->data, BLOCK_SECS, buf->block * BLOCK_SECS, 0, REQ_WRITE);
    buf->dirty = false;
    buf->valid = true;
}

void brelease(buffer_t *buf)
{
    assert(buf);
    buf->count--;
    assert(buf->count>=0);
    // do it now?
    bwrite(buf);
    lock_up(&buffer_lock);
    if (!buf->count) {
        assert(node_is_init(&buf->rnode));
        list_add(&free_list, &buf->rnode);
    }
    if (!list_empty(&wait_list)) {
        task_t *task = container_of(task_t, node, wait_list.next);
        assert(task->magic == J_MAGIC);
        task_unblock(task);
    }
    lock_down(&buffer_lock);
}

void buffer_init(void)
{
    lock_init(&buffer_lock);
    for (size_t i = 0; i < HASH_TLB_SIZE; ++i) {
        list_init(&buffer_hash_table[i]);
    }
}