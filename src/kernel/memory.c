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

void mem_test(void) 
{
    u32 pages[10];
    for (size_t i = 0; i < 10; ++i) {
        pages[i] = get_page();
    }
    for (size_t i = 0; i < 10; ++i) {
        put_page(pages[i]);
    }
}