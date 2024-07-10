#include <jp/types.h>
#include <jp/debug.h>
extern void interrupt_init();
// extern void hang();
extern void task_init();
extern void clock_init();
// extern void time_init(void);
// extern void rtc_init(void);
extern void mem_map_init();
extern void kernel_mm_init(void);
extern bool interrupt_disable(void);
extern bool get_interrupt_state(void);
extern void set_interrupt_state(bool state);
extern void syscall_init(void);
extern void list_test(void);
void kernel_init(void)
{
    interrupt_init();
    mem_map_init();
    kernel_mm_init();
    // time_init();
    // rtc_init();
    clock_init();
    task_init();
    syscall_init();
    list_test();
    set_interrupt_state(true);
}