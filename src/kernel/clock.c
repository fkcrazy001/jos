#include <jp/io.h>
#include <jp/interrupt.h>
#include <jp/assert.h>
#include <jp/debug.h>

#define PIT_CHAN0_REG 0X40
#define PIT_CHAN2_REG 0X42
#define PIT_CTRL_REG 0X43

#define HZ 100
#define OSCILLATOR 1193182
#define CLOCK_COUNTER (OSCILLATOR / HZ)
#define JIFFY (1000 / HZ)


u32 volatile jiffies = 0;
u32 jiffy = JIFFY;

void clock_handler(void)
{
    jiffies++;
    DEBUGK("clock jiffies %d...\n", jiffies);
}

void pit_init(void)
{
    outb(PIT_CTRL_REG, 0b00110100);
    outb(PIT_CHAN0_REG, CLOCK_COUNTER & 0xff);
    outb(PIT_CHAN0_REG, (CLOCK_COUNTER >> 8) & 0xff);
}

void clock_init(void)
{
    pit_init();
    pic_set_interrupt_handler(IRQ_CLOCK, clock_handler);
    pic_set_interrupt(IRQ_CLOCK, true);
}