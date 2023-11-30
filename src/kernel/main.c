#include <jp/joker.h>

int magic = J_MAGIC;

char msg[] = "hello jp!!!";
char buf[1024];

#define screen_start 0xb8000

void kernel_init(void)
{
    char *video = (char*)screen_start;
    for(int i=0;i<sizeof(msg); ++i) {
        video[i*2] = msg[i];
    }
}