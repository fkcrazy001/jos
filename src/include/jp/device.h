#pragma once

#include <jp/types.h>

#define DEV_MAX_NAMELEN 16

typedef uint32_t dev_t;

enum device_type {
    DEV_NULL,
    DEV_CHAR,
    DEV_BLOCK,
};

enum device_subtype {
    DEV_CONSOLE,
    DEV_KEYBOARD,
    DEV_DISK,
    DEV_PART,
};

enum device_cmd {
    DEV_CMD_SECTOR_START = 1,
    DEV_CMD_SECTOR_COUNT = 2,
};

typedef int (*f_ioctl)(void* dev_priv, int cmd, void *args, int flags);
typedef int (*f_read)(void* dev_priv, void *buf, size_t count, uint32_t offset, int flags);
typedef int (*f_write)(void* dev_priv, void *buf, size_t count, uint32_t offset, int flags);

typedef struct device {
    char name[DEV_MAX_NAMELEN];
    int type;
    int subtype;
    dev_t dev;
    dev_t parent;
    void *dev_priv; // pointer to this dev
    f_ioctl ioctl;
    f_read read;
    f_write write;
} device_t;

void device_init(void);

dev_t device_register(
    int type, int subtype,
    void *dev_priv, char *name, dev_t parent,
    f_ioctl ioctl, f_read read, f_write write
);

device_t *device_find(int subtype, uint32_t idx);

device_t *device_get(dev_t dev);

int device_ioctl(dev_t dev, int cmd, void *args, int flags);

// @todo device_read/write -> device_requset

int device_read(dev_t dev, void* buf, size_t count, u32 offset, int flags);

int device_write(dev_t dev, void* buf, size_t count, u32 offset, int flags);

enum {
    REQ_READ = 0,
    REQ_WRITE,
};

// request with schedule
void device_requset(dev_t dev, void* buf, size_t count, u32 offset, int flags, u32 type);