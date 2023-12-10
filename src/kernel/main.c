#include <jp/joker.h>
#include <jp/types.h>
#include <jp/io.h>
#include <jp/string.h>
#include <jp/console.h>

char msg[]="hello jp!!!\n";

void kernel_init(void)
{
    console_init();
    int cnt = 30;
    while (1)
    {
        console_write(msg, sizeof(msg)-1);
    }
}