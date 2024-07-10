#include <jp/io.h>
#include <jp/interrupt.h>
#include <jp/assert.h>
#include <jp/debug.h>
#include <jp/task.h>
#include <jp/joker.h>

#define PIT_CHAN0_REG 0X40
#define PIT_CHAN2_REG 0X42
#define PIT_CTRL_REG 0X43

#define HZ 100
#define OSCILLATOR 1193182
#define CLOCK_COUNTER (OSCILLATOR / HZ)
#define JIFFY (1000 / HZ)

#define SPEAKER_REG 0x61
#define BEEP_HZ 440 // c1 in music
#define BEEP_COUNTER (OSCILLATOR/BEEP_HZ)

static volatile u32 beeping=0;
u32 volatile jiffies = 0;
u32 jiffy = JIFFY;

void start_beep(void)
{
    if (!beeping) {
        outb(SPEAKER_REG, inb(SPEAKER_REG)|0x3);
    }
    beeping = jiffies + 5; // 5 个 JIFFY ms 后停止beep
}

void stop_beep(void)
{
    if (beeping && jiffies > beeping) {
        outb(SPEAKER_REG, inb(SPEAKER_REG) & 0xfc);
        beeping = 0;
    }
}

void clock_handler(void)
{
    jiffies++;
    // DEBUGK("clock jiffies %d...\n", jiffies);
    stop_beep();
    // wake up potential sleeping tasks
    task_wakeup();
    
    // avoid stack overflow
    schedule();
}

void pit_init(void)
{
    // 计数器0 配置为 时钟
    outb(PIT_CTRL_REG, 0b00110100);
    outb(PIT_CHAN0_REG, CLOCK_COUNTER & 0xff);
    outb(PIT_CHAN0_REG, (CLOCK_COUNTER >> 8) & 0xff);

    // 计数器2 配置为蜂鸣器，频率为440hz
    outb(PIT_CTRL_REG, 0b10110110);
    outb(PIT_CHAN2_REG, (u8)BEEP_COUNTER);
    outb(PIT_CHAN2_REG, (u8)(BEEP_COUNTER >> 8));
}

void clock_init(void)
{
    pit_init();
    pic_set_interrupt_handler(IRQ_CLOCK, clock_handler);
    pic_set_interrupt(IRQ_CLOCK, true);
}