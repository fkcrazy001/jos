#pragma once

#include <jp/types.h>

void hang(void);
void delay(u32 count);

// 将 bcd 码转成整数
u8 bcd_to_bin(u8 value);
// 将整数转成 bcd 码
u8 bin_to_bcd(u8 value);