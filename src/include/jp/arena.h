#pragma once
#include <jp/types.h>
#include <jp/list.h>

// 7种内存的描述符，分别管理
// 16B，32B, 64B, 128B, 256B, 512B, 1KB的内存
#define DESC_BASE 16
#define DESC_COUNT 7

typedef list_node_t block_t;

typedef struct arena_descriptor
{
    u32 total_block; // 一页内存分成多少块
    u32 block_size; // 块大小
    list_node_t free_list; // 空闲列表
}arena_descriptor_t;

// 一页或者多页内存
typedef struct arena {
    arena_descriptor_t *desc;
    int32_t count; // 剩余多少块 或者页数量
    u32 large; // 是不是超过1024B
    u32 magic; // magic num
} arena_t;

void *kmalloc(size_t n);
void kfree(void *ptr);