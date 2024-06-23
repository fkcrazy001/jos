#include <jp/types.h>
#include <jp/debug.h>
#include <jp/joker.h>
#include <jp/assert.h>
#include <jp/memory.h>
#include <jp/stdlib.h>
#include <jp/string.h>

#define ZONE_VALID 1
#define ZONE_RESERVED 2

#define IDX(addr) ((u32)addr >> 12) // page idx
#define PAGE(idx) ((u32)idx << 12)             // 获取页索引 idx 对应的页开始的位置
#define ASSERT_PAGE(addr) assert((addr & 0xfff) == 0)

typedef struct ards {
    u64 base;
    u64 size;
    u32 type;
}_packed ards_t;

static u32 mem_base = 0;
static u32 mem_size = 0;
static u32 total_pages = 0;
static u32 free_pages = 0;

#define used_pages (total_pages - free_pages)

void mem_init(u32 magic, u32 ards_cnt_p)
{
    u32 count;
    ards_t *ptr;
    if (magic == J_MAGIC) {
        count = *(u32*)ards_cnt_p;
        ptr = (ards_t*)(ards_cnt_p + 4);
    } else {
        panic("unknow magic 0x%p\n", magic);
    }

    for (size_t i = 0; i < count; ++i, ptr++) {
        DEBUGK("Memory base 0x%p, size 0x%p, type %d\n",
            (u32)ptr->base, (u32)ptr->size, (u32)ptr->type);
        if (ptr->type == ZONE_VALID && ptr->size > mem_size) {
            mem_base = ptr->base;
            mem_size = ptr->size;
        }
    }
    DEBUGK("ARDS count %d\n", count);
    DEBUGK("Memory base 0x%p\n", mem_base);
    DEBUGK("Memory size 0x%p\n", mem_size);

    assert(mem_base == MEMORY_BASE);
    assert((mem_size & (PAGE_SIZE -1)) == 0);
    
    total_pages = IDX(mem_size) + IDX(mem_base);
    free_pages = IDX(mem_size);

    DEBUGK("Total pages %d, Free pages %d\n", total_pages, free_pages);
}

static u8 *page_info_array;
static u32 page_info_n; // info array number of pages
static u32 start_page;

// use first serveral pages to keep in track of page use info
void mem_map_init(void)
{
    page_info_array = (u8 *)mem_base;
    page_info_n = div_round_up(total_pages, PAGE_SIZE);
    DEBUGK("Memory map page count %d\n", page_info_n);

    memset(page_info_array, 0, page_info_n * PAGE_SIZE);

    free_pages -= page_info_n;
    start_page = IDX(mem_base) + page_info_n;
    for (size_t i = 0; i < start_page; ++i)
        page_info_array[i] = 1;
    DEBUGK("Total pages %d, free pages %d\n", total_pages, free_pages);
}

// alloc one free page
static u32 get_page(void)
{
    if (free_pages == 0) {
        panic("Out Of Memory!");
    }
    for (size_t i = start_page; i < total_pages; i++) {
        if (!page_info_array[i]) {
            page_info_array[i] = 1;
            free_pages--;
            u32 page = PAGE(i);
            DEBUGK("Get page 0x%p\n", page);
            return page;
        }
    }
    panic("can't go to here\n");
}

static void put_page(u32 addr)
{
    ASSERT_PAGE(addr);
    u32 idx = IDX(addr);
    assert(idx >= start_page && idx < total_pages);
    assert(page_info_array[idx] >= 1);
    page_info_array[idx]--;
    if (!page_info_array[idx]) {
        free_pages++;
    }
    assert(free_pages > 0 && free_pages < total_pages);
    DEBUGK("put page 0x%p, ref_cnt %u\n", addr,page_info_array[idx]);
    
}

u32 get_cr3(void)
{
    asm volatile("movl %cr3, %eax\n");
}

void set_cr3(u32 pde)
{
    ASSERT_PAGE(pde); // must be page
    asm volatile("movl %%eax, %%cr3\n" ::"a"(pde));
}

static void enable_page(void)
{
    asm volatile(
        "movl %cr0, %eax\n"
        "orl  $0x80000000, %eax\n"
        "movl %eax, %cr0\n");
}

static void entry_init(page_entry_t *entry, u32 index)
{
    assert(index < (1<<20));
    *(u32*)entry = 0;
    entry->present = 1;
    entry->wirte = 1;
    entry->user = 1;
    entry->index = index;
}

#define KERNEL_PAGE_DIR    0x200000
#define KERNEL_PAGE_ENTRY  0x201000

/// @brief 1. map kernel page dir to 0x200000
///        2. map kernel mem: 0-4M -> 0->4M
///        2. open page map
/// @param  none
void kernel_mm_init(void)
{
    page_entry_t *pde = (page_entry_t*)KERNEL_PAGE_DIR;
    memset(pde, 0, PAGE_SIZE);
    BMB;

    entry_init(&pde[0], IDX(KERNEL_PAGE_ENTRY));
    //reset of page dir not exsit

    page_entry_t *pte = (page_entry_t*)KERNEL_PAGE_ENTRY;
    
    for (int i = 0; i < 1024; ++i) {
        entry_init(&pte[i], i); // map 0->4M to 0->4M
        page_info_array[i] = 1; // first 1024 * 4k was used by kernel
    }
    BMB;
    set_cr3((u32)pde);
    BMB;
    enable_page();
}