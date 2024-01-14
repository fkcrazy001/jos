#include <jp/global.h>
#include <jp/string.h>
#include <jp/debug.h>

descriptor_t gdt[GDT_SIZE];
pointer_t gdt_ptr;

void gdt_init(void)
{
    DEBUGK("init gdt !!!\n");
    asm volatile("sgdt gdt_ptr");
    memcpy(&gdt, (const void*)gdt_ptr.base, gdt_ptr.limit + 1);

    gdt_ptr.base = (u32)&gdt;
    gdt_ptr.limit = sizeof(gdt) - 1;
    
    asm volatile("lgdt gdt_ptr");
}