#include <jp/types.h>
extern void interrupt_init();
extern void hang();
// extern void clock_init();
// extern void time_init(void);
// extern void rtc_init(void);
extern mem_map_init();
extern void kernel_mm_init(void);
void kernel_init(void)
{
    interrupt_init();
    mem_map_init();
    char *addr = 0x100000 * 20; // 20M
    *addr = 0x55; // should success here
    kernel_mm_init();
    // task_init();
    // clock_init();
    // time_init();
    // rtc_init();
    
    // *addr = 0xaa; // should fail here

    asm volatile("sti\n"
                "movl %eax, %eax\n");
    hang();
}