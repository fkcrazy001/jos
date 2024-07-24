#include <jp/fifo.h>
#include <jp/assert.h>
#define fifo_next(idx, n) ((idx+1) & ((n)-1))
void fifo_init(fifo_t *fifo, char *buf, u32 length)
{
    fifo->buf = buf;
    fifo->length = length;
    fifo->r = fifo->w = 0;
}
bool fifo_empty(fifo_t *fifo)
{
    return fifo->r == fifo->w;
}
bool fifo_full(fifo_t *fifo)
{
    return fifo_next(fifo->w, fifo->length) == fifo->r;
}
char fifo_get(fifo_t *fifo)
{
    // @todo block if empty;
    assert(!fifo_empty(fifo));
    char bytes = fifo->buf[fifo->r];
    fifo->r = fifo_next(fifo->r, fifo->length);
    return bytes;
}
void fifo_put(fifo_t *fifo, char byte)
{
    while (fifo_full(fifo))
    {
        fifo_get(fifo);
    }
    
    fifo->buf[fifo->w] = byte;
    fifo->w = fifo_next(fifo->w, fifo->length);
}