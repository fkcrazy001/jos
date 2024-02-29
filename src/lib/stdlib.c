#include <jp/stdlib.h>
#include <jp/debug.h>

void hang(void)
{
    DEBUGK("program hanged!!!\n");
    while(1);
}
void delay(u32 count)
{
    while (count--);
}