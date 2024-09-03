#include <jp/buffer.h>
#include <jp/memory.h>

// mm layout
/* |buffer_t|buffer_t|...|buffer_t|gap|block|...|block|block|*/
static buffer_t *start_buf = (buffer_t*)KERNEL_BUFFER_MEM;

static char *start_buf_data = (char*)(KERNEL_BUFFER_MEM+KERNEL_BUFFER_SIZE-BLOCK_SIZE);


#define HASH_TLB_SIZE 31 
static list_node_t buffer_hash_table[HASH_TLB_SIZE];

static list_node_t wait_list = list

buffer_t *getblk(dev_t dev, u32 block)
{

}
buffer_t *bread(dev_t dev, u32 block)
{

}
void bwrite(buffer_t *buf)
{

}
void brelease(buffer_t *buf)
{

}

void buffer_init(void)
{

}