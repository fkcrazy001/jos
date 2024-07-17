#include <jp/global.h>
#include <jp/string.h>
#include <jp/debug.h>

descriptor_t gdt[GDT_SIZE];
pointer_t gdt_ptr;

static inline void descriptor_init(descriptor_t *d, u32 base, u32 limit)
{
    // common
    d->big = 1;
    d->long_mode = 0;

    d->base_low |= (base & 0xffffff);
    d->base_high = (u8)((base>>24) & 0xf);
    d->granularity = 1;  // 4k
    limit = limit >> 12;
    d->limit_low = (u16)(limit & 0xffff);
    d->limit_high |= ((limit >> 16) & 0xf);
}

void gdt_init(void)
{
    DEBUGK("init gdt !!!\n");
    memset(gdt, 0, sizeof(gdt));
    descriptor_t *desc = NULL;
    desc = &gdt[KERNEL_CODE_IDX];
    descriptor_init(desc, 0, 0xffffffff);
    desc->segment = 1;
    desc->present = 1;
    desc->DPL = 0;
    desc->type = 0b1010; // ref to book

    desc = &gdt[KERNEL_DATA_IDX];
    descriptor_init(desc, 0, 0xffffffff);
    desc->segment = 1;
    desc->present = 1;
    desc->DPL = 0;
    desc->type = 0b0010;

    gdt_ptr.base = (u32)&gdt;
    gdt_ptr.limit = sizeof(gdt) - 1;
}