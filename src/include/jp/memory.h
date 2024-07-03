#pragma once

#define PAGE_SIZE (1<<12)
#define MEMORY_BASE (0x100000) // 1M 可用内存开始的地方

typedef struct page_entry {
    u32 present:1; // in mem
    u32 wirte:1; // 0:r, 1:rw
    u32 user:1; // 1:all user, 0 : DPL < 3
    u32 pwt:1; // page write through 1：直写模式， 0 会写模式
    u32 pcd:1; // page cache disable 禁止页缓存
    u32 accessed:1; // 被访问过，用于计数统计
    u32 dirty:1; // 脏页，表示该页缓存被写过
    u32 pat:1; // 页大小， 4k/4M
    u32 global:1; // 全局，所有进程都用到了，该页不刷新缓冲
    u32 ignored:3; // 送给内核的
    u32 index:20; // 页缓存
}_packed page_entry_t;

u32 get_cr3(void);
void set_cr3(u32 pde);

u32 alloc_kpage(u32 count);
u32 free_kpage(u32 addr, u32 count);