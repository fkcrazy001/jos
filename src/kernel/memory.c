#include <jp/types.h>
#include <jp/debug.h>
#include <jp/joker.h>
#include <jp/assert.h>
#include <jp/memory.h>

#define ZONE_VALID 1
#define ZONE_RESERVED 2

#define IDX(addr) ((u32)addr >> 12) // page idx

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