#include <jp/types.h>
extern void interrupt_init();
extern void hang();
// extern void clock_init();
// extern void time_init(void);
// extern void rtc_init(void);
extern void mem_map_init();
extern void kernel_mm_init(void);
extern void mm_test(void);
void kernel_init(void)
{
    interrupt_init();
    mem_map_init();
    kernel_mm_init();
    // task_init();
    // clock_init();
    // time_init();
    // rtc_init();
    mm_test();

    asm volatile("sti\n"
                "movl %eax, %eax\n");
    hang();
}