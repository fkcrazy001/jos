#include <stdio.h>

char msg[] = "hello world!\n";
char buf[1024]={1}; //bss

struct hello
{
    char buf[1024];
}buf2={1};


int main() {
    buf[0]=1;
    printf(msg);
    return 0;
}