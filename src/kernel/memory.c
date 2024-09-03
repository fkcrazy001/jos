#include <jp/types.h>
#include <jp/debug.h>
#include <jp/joker.h>
#include <jp/assert.h>
#include <jp/memory.h>
#include <jp/stdlib.h>
#include <jp/string.h>
#include <jp/bitmap.h>
#include <jp/multiboot2.h>
#include <jp/task.h>

#define ZONE_VALID 1
#define ZONE_RESERVED 2

#define IDX(addr) ((u32)(addr) >> 12) // page idx
#define DIDX(addr) (((u32)(addr) >> 22) & 0x3ff) // first 10bits
#define TIDX(addr) (((u32)(addr) >> 12) & 0x3ff) // next 10bits
#define PAGE(idx) ((u32)(idx) << 12)             // 获取页索引 idx 对应的页开始的位置
#define ASSERT_PAGE(addr) assert(((u32)(addr) & 0xfff) == 0)

typedef struct ards {
    u64 base;
    u64 size;
    u32 type;
}_packed ards_t;

static u32 mem_base = 0;
static u32 mem_size = 0;
static u32 total_pages = 0;
u32 free_pages = 0;

#define used_pages (total_pages - free_pages)

// physical address
static u32 KERNEL_PAGE_TABLE[] = {
    0x2000,
    0x3000,
    0x4000,
    0x5000,
};
#define KERNEL_PTE_SIZE ARRAY_SIZE(KERNEL_PAGE_TABLE)
#define KERNEL_MAP_BITS 0x6000
// one page (4k) has 1024 entries, each entry can map a page(4k)
// thus one page can manage 4M memory
// 4 = ARRAY_SIZE(KERNEL_PAGE_TABLE)
#define KERNEL_PAGE_TABLE_SZIE 4
#define KPDE_SIZE (PAGE_SIZE * 1024 * KERNEL_PAGE_TABLE_SZIE)
#if KERNEL_MEMORY_SIZE != KPDE_SIZE
#error "KERNEL MEMORY SIZE != defined in kernel pde, can't map"
#endif
void mem_init(u32 magic, u32 addr)
{
    u32 count = 0;
    if (magic == J_MAGIC) {
        ards_t *ptr;
        count = *(u32*)addr;
        ptr = (ards_t*)(addr + 4);
        for (size_t i = 0; i < count; ++i, ptr++) {
            DEBUGK("Memory base 0x%p, size 0x%p, type %d\n",
                (u32)ptr->base, (u32)ptr->size, (u32)ptr->type);
            if (ptr->type == ZONE_VALID && ptr->size > mem_size) {
                mem_base = ptr->base;
                mem_size = ptr->size;
            }
        }
    } else if (magic == MULTIBOOT2_MAGIC) {
        u32 size = *(u32*)addr;
        DEBUGK("total multiboot info size 0x%x\n", size);
        multi_tag_t *t = (multi_tag_t*)(addr+8);
        while (t->type != MULTIBOOT_TAG_TYPE_END)
        {
            DEBUGK("tag type = %d, size=%d\n", t->type, t->size);
            if (t->type == MULTIBOOT_TAG_TYPE_MMAP)
                break;
            t = (multi_tag_t*)((u32)t + ALIGN(t->size, 8));
        }
        multi_tag_mmap_t *mt = (multi_tag_mmap_t*)t;
        multi_mmap_entry_t *e = mt->entries;
        while ((u32)e < (u32)t + t->size)
        {
            DEBUGK("Memory base 0x%p, size 0x%p, type %d\n",
                (u32)e->addr, (u32)e->len, (u32)e->type);
            if (e->type == MULTIBOOT_MEMORY_AVAILABLE && e->len > mem_size) {
                mem_base = (u32)e->addr;
                mem_size = (u32)e->len;
            }
            count++;
            e = (multi_mmap_entry_t*)((u32)e + mt->entry_size);
        }
    } else {
        panic("unknow magic 0x%p\n", magic);
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
    // kernel has 0-KERNEL_BUFFER_MEM memory, managed by bitmap
    static_assert((IDX(KERNEL_BUFFER_MEM) - IDX(MEMORY_BASE)) % 8 == 0);
    static_assert((IDX(KERNEL_BUFFER_MEM) - IDX(MEMORY_BASE)) <= PAGE_SIZE);
    bitmap_init(&kernel_map, (char*)KERNEL_MAP_BITS, (IDX(KERNEL_BUFFER_MEM) - IDX(MEMORY_BASE))/BITLEN, IDX(MEMORY_BASE));
    bitmap_scan(&kernel_map, page_info_n);
}

// alloc one free page from 8M above
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

u32 get_cr2(void)
{
    asm volatile("movl %cr2, %eax\n");
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
    entry->write = 1;
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
    static_assert(ARRAY_SIZE(KERNEL_PAGE_TABLE) == KERNEL_PAGE_TABLE_SZIE);
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


static __always_inline void flush_tlb(u32 vaddr) 
{
    asm volatile("invlpg (%0)" ::"r"(vaddr):"memory");
}

#define PDE_MASk 0xffc00000

static inline page_entry_t *get_pte(u32 vaddr, bool create)
{
    page_entry_t *pde = get_pde();
    u32 idx = DIDX(vaddr);
    page_entry_t *entry = &pde[idx];

    assert(create || entry->present); // either create or exist

    page_entry_t *table = (page_entry_t*)(PDE_MASk|(idx<<12));
    if (!entry->present) {
        DEBUGK("get and create new page table entry for 0x%p\n", vaddr);
        u32 page = get_page();
        entry_init(entry, IDX(page));
        flush_tlb((u32)table);
        memset(table, 0, PAGE_SIZE);
    }
    return table;
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
 // this api either success or panic
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

void link_page(u32 vaddr)
{
    // must be page
    ASSERT_PAGE(vaddr);

    page_entry_t *pte = get_pte(vaddr, true);
    page_entry_t *entry = &pte[TIDX(vaddr)];
    task_t *now = current;
    bitmap_t *vmmap=now->vmap;
    u32 idx = IDX(vaddr);
    if (entry->present) {
        assert(bitmap_test(vmmap, idx));
        return;
    }
    assert(!bitmap_test(vmmap, idx));
    u32 pa = get_page();
    entry_init(entry, IDX(pa));
    flush_tlb(vaddr);
    bitmap_set(vmmap, idx, true);

    DEBUGK("link va 0x%p to pa 0x%p\n", vaddr, pa);
}

void unlink_page(u32 vaddr)
{
    ASSERT_PAGE(vaddr);
    page_entry_t *pte = get_pte(vaddr, false);
    page_entry_t *entry = &pte[TIDX(vaddr)];

    task_t *task = current;
    bitmap_t *vmmap = task->vmap;
    u32 idx = IDX(vaddr);
    if (!entry->present) {
        assert(!bitmap_test(vmmap, idx));
        return;
    }
    assert(entry->present && bitmap_test(vmmap, idx));
    entry->present = false;
    bitmap_set(vmmap, idx, 0);
    u32 pa = PAGE(entry->index);
    put_page(pa);
    flush_tlb(vaddr);
    DEBUGK("unlink va 0x%p to pa 0x%p\n", vaddr, pa);
}

/// @brief copy va(page aligned) to a user physical page
/// @param page 
/// @return new physical page in 8m above
static u32 copy_page(u32 va)
{
    ASSERT_PAGE(va);
    u32 page = get_page();
    page_entry_t *pte0 = (page_entry_t*)PDE_MASk;
    assert(!pte0->present); // address 0-4k shouldn't be mapped
    entry_init(pte0, IDX(page));
    flush_tlb(0);
    memcpy((void*)0, (void*)va, PAGE_SIZE);
    pte0->present = false;
    flush_tlb(0);
    return page;
}

/// @brief copy current task page dir entry and table entry, copy on write
/// @param  none
/// @return pde
page_entry_t *copy_pde(void)
{
    task_t *t = current;
    page_entry_t *new = (page_entry_t*)alloc_kpage(1); // process pde belongs to kernel
    
    // new pde is almost the same as old, except for last item pointer itself
    memcpy(new, (void*)t->pde, PAGE_SIZE);
    entry_init(&new[1023], IDX(new)); // last item points itself for get_pde

    // copy each page dir entry
    for (size_t didx = KERNEL_PTE_SIZE; didx < 1023; ++didx) {
        page_entry_t *pde = &new[didx];
        if (!pde->present)
            continue;
        page_entry_t *pte = (page_entry_t*)(PDE_MASk | (didx << 12));
        for (size_t tidx = 0; tidx<1024; ++tidx) {
            page_entry_t *e = &pte[tidx];
            if (!e->present)
                continue;
            assert(page_info_array[e->index] > 0);
            e->write = false; // copy on write
            page_info_array[e->index]++;
            assert(page_info_array[e->index] < 255); // can't ref more than 255
        }
        // page table entries don't support COW now, may support in future
        pde->index = IDX(copy_page((u32)pte));
    }

    // pde content changed, set pde to force a tlb refresh
    set_cr3(t->pde); 

    return new;
}

// 在调用完这个接口后，程序会使用kernel pde。也就是说只能访问 0-8M 的va了。
void free_pde(void)
{
    task_t *t = current;
    page_entry_t *now = get_pde();

    // free each page dir entry
    for (size_t didx = KERNEL_PTE_SIZE; didx < 1023; ++didx) {
        page_entry_t *pde = &now[didx];
        if (!pde->present)
            continue;
        page_entry_t *pte = (page_entry_t*)(PDE_MASk | (didx << 12));
        for (size_t tidx = 0; tidx<1024; ++tidx) {
            page_entry_t *e = &pte[tidx];
            if (!e->present)
                continue;
            assert(page_info_array[e->index] > 0);
            put_page(PAGE(e->index));
        }
        assert(page_info_array[pde->index] > 0);
        put_page(PAGE(pde->index));
    }
    free_kpage(t->pde, 1);
    set_cr3(KERNEL_PAGE_DIR); // use kernel pde
}

typedef struct page_error_code {
    u32 present:1,
        rw:1,
        user:1,
        resv0:1,
        fetch:1,
        protection:1,
        shadow:1,
        resv1:8,
        sgx:1,
        resv2:16;
}_packed page_error_code_t;

void page_fault(u32 vector,
                u32 edi, u32 esi, u32 ebp, u32 esp,
                u32 ebx, u32 edx, u32 ecx, u32 eax, //pusha
                u32 gs,  u32 fs,  u32 es,  u32 ds, // push seg
                u32 vector0, u32 error, // push by macro
                u32 eip, u32 cs, u32 eflags)
{
    assert(vector0==vector && vector == 0xe); // pf
    u32 vaddr = get_cr2();
    DEBUGK("page fault @0x%p\n", vaddr);
    task_t *t = current;
    assert(KERNEL_MEMORY_SIZE <= vaddr && vaddr<USER_STK_TOP);
    page_error_code_t *code = (page_error_code_t*)&error;
    // addr present
    if (code->present) {
        assert(code->rw);  // present but write opr
        page_entry_t *pte = get_pte(vaddr, false);
        page_entry_t *e = &pte[TIDX(vaddr)];
        assert(e->present && !e->write);
        if (page_info_array[e->index] == 1) {
            e->write = true;
            DEBUGK("set page %d to writable\n", e->index);
        } else {
            // copy page
            assert(page_info_array[e->index] > 1);
            page_info_array[e->index]--;
            u32 pp = copy_page(PAGE(IDX(vaddr)));
            entry_init(e, IDX(pp));
            flush_tlb(vaddr);
            DEBUGK("copy page for 0x%p\n", vaddr);
        }
        return;
    }
    // not present
    if (vaddr < t->brk || vaddr >= USER_STK_BOTTOM) {
        // stk page fault
        u32 page = PAGE(IDX(vaddr));
        link_page(page);
        return;
    }
    panic("page fault!!!");
}

int32_t sys_brk(u32 addr)
{
    ASSERT_PAGE(addr);
    task_t *now = current;
    assert(now->uid != KERNEL_USER);
    assert(addr >= KERNEL_MEMORY_SIZE && addr < USER_STK_BOTTOM);
    if (now->brk > addr) {
        // unlink some page
        u32 start = addr;
        for (; start < now->brk; start += PAGE_SIZE) {
            unlink_page(start);
        }
    } else if (now->brk < addr) {
        // expend
        if ((addr - now->brk)/PAGE_SIZE > free_pages) {
            WARNK("alloc too many memory\n");
            // fail for this stage... many use page swap
            return -1;
        }
    }
    now->brk = addr;
    return 0;
}