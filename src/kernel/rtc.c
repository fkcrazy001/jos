#include <jp/rtc.h>
#include <jp/interrupt.h>
#include <jp/io.h>
#include <jp/debug.h>
#include <jp/stdlib.h>
#include <jp/time.h>
// 读 cmos 寄存器的值
u8 cmos_read(u8 addr)
{
    outb(CMOS_ADDR, CMOS_NMI | addr);
    return inb(CMOS_DATA);
};

void cmos_write(u8 addr, u8 value)
{
    outb(CMOS_ADDR, CMOS_NMI | addr);
    outb(CMOS_DATA, value);
};

// 设置 secs 秒后发生实时时钟中断
void set_alarm(u32 secs)
{
    DEBUGK("beeping after %d seconds\n", secs);

    tm time;
    time_read(&time);

    u8 sec = secs % 60;
    secs /= 60;
    u8 min = secs % 60;
    secs /= 60;
    u32 hour = secs;

    time.tm_sec += sec;
    if (time.tm_sec >= 60)
    {
        time.tm_sec %= 60;
        time.tm_min += 1;
    }

    time.tm_min += min;
    if (time.tm_min >= 60)
    {
        time.tm_min %= 60;
        time.tm_hour += 1;
    }

    time.tm_hour += hour;
    if (time.tm_hour >= 24)
    {
        time.tm_hour %= 24;
    }

    DEBUGK("hour=%d, min=%d, sec=%d\n", time.tm_hour, time.tm_min, time.tm_sec);

    cmos_write(CMOS_CLOCK_HOUR, bin_to_bcd(time.tm_hour));
    cmos_write(CMOS_CLOCK_MIN, bin_to_bcd(time.tm_min));
    cmos_write(CMOS_CLOCK_SECOND, bin_to_bcd(time.tm_sec));

    cmos_write(CMOS_B, 0b00100010); // 打开闹钟中断
    cmos_read(CMOS_C);              // 读 C 寄存器，以允许 CMOS 中断
}
extern void start_beep(void);
static void rtc_handler(void)
{
    static u32 cnt = 0;
    // cmos_read(CMOS_C); // read cmos c to enable interrput
    // set_alarm(2);
    DEBUGK("rtc handler, cnt=%d...\n", cnt++);
    start_beep();
}

void rtc_init(void)
{
    // cmos_write(CMOS_B, 0b01000010); // 打开周期中断
    // cmos_read(CMOS_C);

    // 设置中断频率
    // outb(CMOS_A, (inb(CMOS_A) & 0xf) | 0b1110); // 500ms
    
    // set_alarm(2);

    pic_set_interrupt_handler(IRQ_RTC,  (pic_handler_t)rtc_handler);
    pic_set_interrupt(IRQ_RTC, true);
}
