#include <jp/stdlib.h>
#include <jp/debug.h>
#include <jp/stdlib.h>
#include <jp/stdio.h>

void hang(void)
{
    printf("program hanged!!!\n");
    while(1);
}
void delay(u32 count)
{
    while (count--);
}

// 将 bcd 码转成整数
u8 bcd_to_bin(u8 value)
{
    return (value & 0xf) + (value >> 4) * 10;
}

// 将整数转成 bcd 码
u8 bin_to_bcd(u8 value)
{
    return (value / 10) * 0x10 + (value % 10);
}

u32 div_round_up(u32 num, u32 size)
{
    return (num + size - 1) / size;
}

int atoi(const char *str)
{
    if (str == NULL)
        return 0;
    int sign = 1;
    int res = 0;
    if (*str == '-'){
        sign = -1;
        str++;
    }
    while(*str) {
        char c = *str;
        if (c < '0' || c > '9') {
            return 0;
        }
        res = res * 10 + c - '0';
        str++;
    }
    return sign * res;
}