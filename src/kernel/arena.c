#include <jp/arena.h>
#include <jp/memory.h>
#include <jp/string.h>
#include <jp/stdlib.h>
#include <jp/assert.h>
#include <jp/joker.h>

extern u32 free_pages;
static arena_descriptor_t descs[DESC_COUNT];

// mm layout
// 0: arena_t
// sizeof(arena_t): memory
// page_size

void arena_init(void)
{
    u32 block_size = DESC_BASE;
    for (size_t i = 0; i < DESC_COUNT; i++, block_size<<=1)
    {
        arena_descriptor_t *d = &descs[i];
        d->block_size = block_size;
        d->total_block = (PAGE_SIZE-sizeof(arena_t))/block_size;
        list_init(&d->free_list);
    }
}

static void* get_arena_block(arena_t *arena, u32 idx)
{
    assert(arena->desc->total_block > idx);
    void *addr = (void*)(arena+1);
    return (void*)((u32)addr + idx * arena->desc->block_size);
}

static arena_t *get_block_arena(block_t *block)
{
    return (arena_t*)((u32)block & 0xfffff000); // page start
}

void *kmalloc(size_t size)
{
    arena_descriptor_t *desc = NULL;
    arena_t *arena;
    block_t *block;
    if (size > 1024) {
        u32 asize = size + sizeof(arena_t);
        u32 count = div_round_up(asize, PAGE_SIZE);
        arena = (arena_t *)alloc_kpage(count);

        memset(arena, 0, count*PAGE_SIZE);
        arena->large = true;
        arena->count = count;
        arena->desc = NULL;
        arena->magic = J_MAGIC;
        return (void*)(arena + 1);
    }
    for (size_t i = 0; i < DESC_COUNT; i++)
    {
        desc = &descs[i];
        if (desc->block_size >= size) {
            break;
        }
    }
    assert(desc != NULL);
    if (list_empty(&desc->free_list)) {
        arena = (arena_t*)alloc_kpage(1);
        memset(arena, 0, PAGE_SIZE);
        arena->desc = desc;
        arena->large = false;
        arena->count = desc->total_block ;
        arena->magic = J_MAGIC;
        for (size_t i=0;i<desc->total_block;++i) {
            block = get_arena_block(arena, i);
            // 当这个内存不被使用的时候，用来作为链表
            assert(!list_search(&arena->desc->free_list, block));
            list_add(&arena->desc->free_list, block);
            assert(list_search(&arena->desc->free_list, block));
        }
    }
    block = list_pop(&desc->free_list);
    arena = get_block_arena(block);
    assert(arena->count>0 && arena->magic == J_MAGIC && !arena->large);
    arena->count--;
    return (void*)block;
}

void kfree(void *ptr)
{
    assert(ptr);
    arena_t *arena = get_block_arena(ptr);
    assert(arena->magic == J_MAGIC);
    assert(arena->large == 0 || arena->large==1);
    assert(arena->count >= 0);
    if (arena->large) {
        assert(arena->count > 0);
        free_kpage((u32)arena, arena->count);
        return;
    }
    block_t *block = ptr;
    list_add(&arena->desc->free_list, block);
    arena->count++;
    if (arena->count == arena->desc->total_block) {
        for (size_t i = 0; i < arena->desc->total_block; i++)
        {
            block = get_arena_block(arena, i);
            assert(list_search(&arena->desc->free_list, block));
            list_del(block);
            assert(!list_search(&arena->desc->free_list, block));
        }
        free_kpage((u32)arena, 1);
    }
}