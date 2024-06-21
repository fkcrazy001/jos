#include <jp/types.h>
extern void interrupt_init();
extern void hang();
// extern void clock_init();
// extern void time_init(void);
// extern void rtc_init(void);
extern mem_map_init();
extern mem_test();
void kernel_init(void)
{
    interrupt_init();
    mem_map_init();
    // task_init();
    // clock_init();
    // time_init();
    // rtc_init();
    mem_test();

    asm volatile("sti\n"
                "movl %eax, %eax\n");
    hang();
}