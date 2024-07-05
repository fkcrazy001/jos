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

    asm volatile("movl $0, %eax\n"
                "movl $1, %ebx\n"
                "movl $2, %ecx\n"
                "movl $3, %edx\n"
                "int $0x80\n"
                "movl $1, %eax\n"
                "int $0x80\n"
                "movl $256, %eax\n"
                "int $0x80\n");
    // set_interrupt_state(true);
}