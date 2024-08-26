#include <jp/device.h>
#include <jp/string.h>
#include <jp/assert.h>
#include <jp/debug.h>
#define DEVICE_NR 64

static device_t devices[DEVICE_NR];

void device_init(void)
{
    for (size_t i=0; i < DEVICE_NR; ++i) {
        device_t *dev = devices + i;
        memset(dev, 0, sizeof(*dev));
        dev->dev = i;
        dev->type = DEV_NULL;
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