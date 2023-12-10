#include <jp/joker.h>
#include <jp/types.h>
#include <jp/io.h>
#include <jp/string.h>
#include <jp/console.h>
#include <jp/printk.h>

void kernel_init(void)
{
    console_init();
    int cnt = 30;
    printk("very long long long long long long long long long long long long long long long longlong long long long long long long long msg");
    printk("\n");
    printk("very long long long long long long long long long long long long long long long longlong long long long long long long long msg");
    while(cnt--) {
        //printk("hello jp!!! cnt is %d, hex is 0x%x\n, addr format is %p", cnt, cnt, cnt);
    }
}