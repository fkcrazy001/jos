#pragma once

#include <jp/types.h>

#define MAX(a, b)  (a < b ? b:a)
#define MIN(a, b)  (a < b ? a:b)

void hang(void);
void delay(u32 count);

// 将 bcd 码转成整数
u8 bcd_to_bin(u8 value);
// 将整数转成 bcd 码
u8 bin_to_bcd(u8 value);

u32 div_round_up(u32 num, u32 size);

int atoi(const char *s);