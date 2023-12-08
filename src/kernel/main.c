#include <jp/joker.h>
#include <jp/types.h>
#include <jp/io.h>
#include <jp/string.h>


//#define CRT_ADDR_REG 0x3d4
//#define CRT_DATA_REG 0x3d5

//#define CRT_CURSOR_H 0xe
//#define CRT_CURSOR_L 0xf

char msg[]="hello world!";
char buf[1024];

void kernel_init(void)
{
    int res;
    res = strcmp(buf, msg);
    strcpy(buf, msg);
}