#include <jp/types.h>
#include <stdio.h>

typedef struct descriptor {
    u16 limit_low;
    u32 base_low:24,
        type:4,
        segment:1,
        dpl:2,
        present:1;
    u8  limit_high:4,
        available:1,
        long_mode:1,
        big:1,
        granularity:1;
    u8  base_high;
} descriptor_t;

int main() {
    printf("sizeof u8 is %d\n",sizeof(u8));
    printf("sizeof u16 is %d\n",sizeof(u16));
    printf("sizeof u32 is %d\n",sizeof(u32));
    printf("sizeof u64 is %d\n",sizeof(u64));

    printf("sizeof descriptor_t is %d\n",sizeof(descriptor_t));
    
    descriptor_t des;

    return 0;
}