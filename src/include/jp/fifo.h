#pragma once

#include <jp/types.h>

typedef struct fifo {
    char *buf;
    u32 length; // len bytes
    u32 r;
    u32 w;
}fifo_t;

void fifo_init(fifo_t *fifo, char *buf, u32 length);
bool fifo_empty(fifo_t *fifo);
bool fifo_full(fifo_t *fifo);
char fifo_get(fifo_t *fifo);
void fifo_put(fifo_t *fifo, char byte);