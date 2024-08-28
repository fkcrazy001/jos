#include <jp/device.h>
#include <jp/string.h>
#include <jp/assert.h>
#include <jp/debug.h>
#include <jp/arena.h>
#include <jp/task.h>

#define DEVICE_NR 64

static device_t devices[DEVICE_NR];

void device_init(void)
{
    for (size_t i=0; i < DEVICE_NR; ++i) {
        device_t *dev = devices + i;
        memset(dev, 0, sizeof(*dev));
        dev->dev = i;
        dev->type = DEV_NULL;
        dev->sche_list_head = NULL;
        strncpy(dev->name, "null", sizeof(dev->name));
    }
}

static device_t *device_find_null(void)
{
    // device 0 is always "null"
    for (size_t i=1; i < DEVICE_NR; ++i) {
        device_t *dev = devices + i;
        if (dev->type == DEV_NULL) {
            return dev;
        }
    }
    panic("no more devices");
}

dev_t device_register(
    int type, int subtype,
    void *ptr, char *name, dev_t parent,
    f_ioctl ioctl, f_read read, f_write write
)
{
    device_t *idle = device_find_null();
    idle->type = type;
    idle->subtype = subtype;
    idle->dev_priv = ptr;
    strncpy(idle->name, name, sizeof(idle->name));
    idle->parent = parent;
    idle->ioctl = ioctl;
    idle->read = read;
    idle->write = write;
    // @todo schedule should done in ide.c
    if (type == DEV_BLOCK) {
        if(parent == DEV_NULL) {
            idle->sche_list_head = kmalloc(sizeof(*idle->sche_list_head));
            list_init(idle->sche_list_head);
        } else {
            idle->sche_list_head = device_get(parent)->sche_list_head;
        }
    }
    return idle->dev;
}

device_t *device_find(int subtype, uint32_t idx)
{
    int nr = 0;
    for(size_t i = 0; i < DEVICE_NR; ++i) {
        device_t *dev = devices + i;
        if (dev->type == DEV_NULL || dev->subtype != subtype)
            continue;
        if (nr++ == idx)
            return dev;
    }
    return NULL;
}
device_t *device_get(dev_t dev)
{
    assert(dev < DEVICE_NR);
    device_t *ret = &devices[dev];
    assert(ret->dev != DEV_NULL);
    return ret;
}

int device_ioctl(dev_t dev, int cmd, void *args, int flags)
{
    device_t *device = device_get(dev);
    if (device->ioctl) {
        return device->ioctl(device->dev_priv, cmd, args, flags);
    }
    WARNK("ioctl of %s is unimplemented\n", device->name);
    return -1;
}

int device_read(dev_t dev, void* buf, size_t count, u32 offset, int flags)
{
    device_t *device = device_get(dev);
    if (device->read) {
        return device->read(device->dev_priv, buf, count, offset, flags);
    }
    WARNK("read of %s is unimplemented\n", device->name);
    return -1;
}

int device_write(dev_t dev, void* buf, size_t count, u32 offset, int flags)
{
    device_t *device = device_get(dev);
    if (device->write) {
        return device->write(device->dev_priv, buf, count, offset, flags);
    }
    WARNK("write of %s is unimplemented\n", device->name);
    return -1;
}

struct block_request {
    dev_t dev;
    void* buf;
    size_t count;
    u32 offset;
    int flags;
    u32 type;
    task_t *task;
    list_node_t node;
};

static void do_request(struct block_request *req)
{
    switch (req->type)
    {
    case REQ_READ:
        device_read(req->dev, req->buf, req->count, req->offset, req->flags);
        break;
    case REQ_WRITE:
        device_write(req->dev, req->buf, req->count, req->offset, req->flags);
        break;
    default:
        panic("invalid type %d\n", req->type);
        break;
    }
}

void device_requset(dev_t dev, void* buf, size_t count, u32 offset, int flags, u32 type)
{
    device_t *device = device_get(dev);
    assert(device->type == DEV_BLOCK);
    struct block_request *req = kmalloc(sizeof(*req));

    req->dev = dev;
    req->buf = buf;
    req->count = count;
    req->offset = offset;
    req->flags = flags;
    req->type = type;

    // 这里由于要做调度，不能用默认的lock——up/down，因为后者是fifo调度

    bool empty = list_empty(device->sche_list_head);

    // fifo now
    list_add(device->sche_list_head, &req->node);

    if (!empty) {
        req->task = current;
        task_block(req->task, NULL, TASK_BLOCKED);
    }
    do_request(req);
    list_del(&req->node);
    kfree(req);
    if (!list_empty(device->sche_list_head)) {
        task_t *next = container_of(struct block_request, node, device->sche_list_head->next)->task;
        assert(next->magic == J_MAGIC);
        task_unblock(next);
    }
}