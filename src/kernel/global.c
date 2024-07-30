#include <jp/global.h>
#include <jp/string.h>
#include <jp/debug.h>

descriptor_t gdt[GDT_SIZE]; // 全局描述符表
pointer_t gdt_ptr; // 全局描述符指针
tss_t tss; // 内核任务状态段

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
    descriptor_init(desc, 0, (u32)-1);
    desc->segment = SEG_CODE_OR_DATA;
    desc->present = 1;
    desc->DPL = 0;
    desc->type = SEG_CODE; // ref to book

    desc = &gdt[KERNEL_DATA_IDX];
    descriptor_init(desc, 0, (u32)-1);
    desc->segment = SEG_CODE_OR_DATA;
    desc->present = 1;
    desc->DPL = 0;
    desc->type = SEG_DATA;

    desc = &gdt[USER_CODE_IDX];
    descriptor_init(desc, 0, (u32)-1);
    desc->segment = SEG_CODE_OR_DATA;
    desc->present = 1;
    desc->DPL = 3;
    desc->type = SEG_CODE;

    desc = &gdt[USER_DATA_IDX];
    descriptor_init(desc, 0, (u32)-1);
    desc->segment = SEG_CODE_OR_DATA;
    desc->present = 1;
    desc->DPL = 3;
    desc->type = SEG_DATA;

    gdt_ptr.base = (u32)&gdt;
    gdt_ptr.limit = sizeof(gdt) - 1;
}
// init kernel tss
void tss_init(void)
{
    memset(&tss, 0, sizeof(tss));
    
    tss.ss0 = KERNEL_DS;
    tss.iobase = sizeof(tss); // indicates that iobase map not exist
    descriptor_t *td = &gdt[KERNEL_TSS_IDX];
    descriptor_init(td, (u32)&tss, (sizeof(tss)-1) << 12);
    td->segment = SEG_SYSTEM;
    td->granularity = 0; // 字节
    td->big = 0; // fixed 0
    td->long_mode = 0; // fixed 0
    td->present = 1;
    td->DPL = 0;
    td->type = TSS_32_A;

    // BMB;
    asm volatile("ltr %%ax\n"::"a"(KERNEL_TSS_SELECTOR));
}