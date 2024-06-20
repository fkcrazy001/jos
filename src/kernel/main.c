#include <jp/types.h>
extern void console_init();
extern void gdt_init();
extern void interrupt_init();
extern void hang();
extern void clock_init();
extern void time_init(void);
extern void rtc_init(void);
extern void set_alarm(u32 sec);
void kernel_init(void)
{
    console_init();
    gdt_init();
    interrupt_init();
    // task_init();
    clock_init();
    time_init();
    rtc_init();
    set_alarm(5);
    asm volatile("sti\n"
                "movl %eax, %eax\n");
    hang();
}