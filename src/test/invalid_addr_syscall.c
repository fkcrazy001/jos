#include <stdio.h>
#include <stdlib.h>
#define GNU_
#include <unistd.h>

int main(void) {
    // 
    int err = write(stdin, NULL, 128);
    return 0;
}