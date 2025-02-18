#pragma once

// buffer stores harddisk buffer

#include <jp/list.h>
#include <jp/device.h>

#define BLOCK_SIZE 1024
#define SECTOR_SIZE 512
#define BLOCK_SECS (BLOCK_SIZE/SECTOR_SIZE)

typedef struct buffer {
    void *data;
    dev_t dev;
    u32 block;
    u32 pid;
    int count; // ref cnt
    list_node_t hnode;
    list_node_t rnode;
    bool dirty; // if this  is dirty
    bool valid; // if this is valid
} buffer_t;

buffer_t *getblk(dev_t dev, u32 block);
buffer_t *bread(dev_t dev, u32 block);
void bwrite(buffer_t *buf);
void brelease(buffer_t *buf);