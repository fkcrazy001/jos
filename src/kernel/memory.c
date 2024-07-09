#include <jp/types.h>
#include <jp/debug.h>
#include <jp/joker.h>
#include <jp/assert.h>
#include <jp/memory.h>
#include <jp/stdlib.h>
#include <jp/string.h>
#include <jp/bitmap.h>

#define ZONE_VALID 1
#define ZONE_RESERVED 2

#define IDX(addr) ((u32)addr >> 12) // page idx
#define DIDX(addr) (((u32)addr >> 22) & 0x3ff) // first 10bits
#define TIDX(addr) (((u32)addr >> 12) & 0x3ff) // next 10bits
#define PAGE(idx) ((u32)idx << 12)             // 获取页索引 idx 对应的页开始的位置
#define ASSERT_PAGE(addr) assert(((u32)addr & 0xfff) == 0)

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

// physical address
static u32 KERNEL_PAGE_TABLE[] = {
    0x2000,
    0x3000,
};
#define KERNEL_MAP_BITS 0x4000
// one page (4k) has 1024 entries, each entry can map a page(4k)
// thus one page can manage 4M memory
#define KERNEL_MEMORY_SIZE  (PAGE_SIZE * 1024 * ARRAY_SIZE(KERNEL_PAGE_TABLE))

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
    
    if (mem_size < KERNEL_MEMORY_SIZE) {
        panic("mem too small(0x%p) to boot jos, min(0x%p) needed\n", mem_size, KERNEL_MEMORY_SIZE);
    }

    total_pages = IDX(mem_size) + IDX(mem_base);
    free_pages = IDX(mem_size);

    DEBUGK("Total pages %d, Free pages %d\n", total_pages, free_pages);
}

static u8 *page_info_array;
static u32 page_info_n; // info array number of pages
static u32 start_page;
bitmap_t kernel_map;

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
    // kernel has 0-KERNEL_MEMORY_SIZE memory, managed by bitmap
    static_assert((IDX(KERNEL_MEMORY_SIZE) - IDX(MEMORY_BASE)) % 8 == 0);
    static_assert((IDX(KERNEL_MEMORY_SIZE) - IDX(MEMORY_BASE)) <= PAGE_SIZE);
    bitmap_init(&kernel_map, (char*)KERNEL_MAP_BITS, (IDX(KERNEL_MEMORY_SIZE) - IDX(MEMORY_BASE))/BITLEN, IDX(MEMORY_BASE));
    bitmap_scan(&kernel_map, page_info_n);
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

static __always_inline void enable_page(void)
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

/// @brief 1. map kernel mem: 0-8M -> 0->8M
///        2. map pde[1023] -> pde 
///        3. open page map
/// @param  none
void kernel_mm_init(void)
{
    page_entry_t *pde = (page_entry_t*)KERNEL_PAGE_DIR;
    memset(pde, 0, PAGE_SIZE);
    // BMB;

    int page_idx = 0;
    for (int i = 0; i < ARRAY_SIZE(KERNEL_PAGE_TABLE); ++i) {
        page_entry_t *pte = (page_entry_t*)KERNEL_PAGE_TABLE[i];
        memset(pte, 0, PAGE_SIZE);
        entry_init(&pde[i], IDX(pte));
        for (int j = 0; j < 1024; ++j, ++page_idx) {
            if (!page_idx) // don't map 0-4k to 0-4k, expect null ptr raise pf
                continue;
            entry_init(&pte[j], page_idx);
            page_info_array[page_idx] = 1; //used by kernel
        }
    }
    // 这样的话，0xfffff000就指向pde自己了，坏处是浪费了4M的虚拟地址
    entry_init(&pde[1023], IDX(pde));

    set_cr3((u32)pde);
    enable_page();
}

static inline page_entry_t* get_pde(void)
{
    return (page_entry_t*)0xfffff000;
}

static inline page_entry_t *get_pte(u32 vaddr)
{
    return (page_entry_t*)(0xffc00000 | (DIDX(vaddr) << 12));
}

static __always_inline void flush_tlb(u32 vaddr) 
{
    asm volatile("invlpg (%0)" ::"r"(vaddr):"memory");
}

static u32 scan_page(bitmap_t *map, u32 count)
{
    u32 idx = bitmap_scan(map, count);
    if (idx == EOF) {
        panic("no free pages when trying fetch %d pages\n", count);
    }
    return PAGE(idx);
}

static void reset_page(bitmap_t *map, u32 vaddr, u32 count)
{
    u32 idx=IDX(vaddr);
    for(u32 i=0;i<count;i++) {
        assert(bitmap_test(map, i+idx));
        bitmap_set(map, idx+i, 0);
    }
}

u32 alloc_kpage(u32 count)
{
    assert(count>0);
    u32 vaddr = scan_page(&kernel_map, count);
    DEBUGK("ALLOC kernel pages start from 0x%p, count %d\n", vaddr, count);
    return vaddr;
}

u32 free_kpage(u32 addr, u32 count)
{
    ASSERT_PAGE(addr);
    assert(count>0);
    reset_page(&kernel_map, addr, count);
    DEBUGK("FREE kernel pages start from 0x%p, count %d\n", addr, count);
}