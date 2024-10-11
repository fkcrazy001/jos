#include <jp/fs.h>
#include <jp/bitmap.h>

#define make_idx(sb, offset) ((offset) -1 + (sb)->desc->firstdatazone)
#define make_offset(sb, idx) ((idx) + 1 - (sb)->desc->firstdatazone)

int balloc(dev_t dev)
{
    super_block_t *sb = read_super(dev);
    assert(sb);
    bitmap_t bm;
    int offset = EOF;
    buffer_t *buf = NULL;
    for(size_t i = 0; i < sb->desc->zmap_blocks; ++i) {
        buf = sb->zmaps[i];
        bitmap_make(&bm, buf->data, BLOCK_SIZE, i * BLOCK_SIZE * BITLEN);
        offset = bitmap_scan(&bm, 1);
        if (offset!=EOF) {
            assert(make_idx(sb,offset)<sb->desc->zones);
            buf->dirty = true;
            break;
        }
    }
    bwrite(buf);
    return offset==EOF?:make_idx(sb,offset);
}
void bfree(dev_t dev, int idx)
{
    assert(idx!=EOF);
    super_block_t *sb = read_super(dev);
    assert(sb);
    bitmap_t bm;
    buffer_t *buf = NULL;
    size_t offset = make_offset(sb, idx);
    int zmap_nr = offset / (BITLEN * BLOCK_SIZE);
    assert(zmap_nr < ZMAP_MAX_NR);
    buf = sb->zmaps[zmap_nr];
    assert(buf);
    bitmap_make(&bm, buf->data, BLOCK_SIZE, zmap_nr *(BITLEN * BLOCK_SIZE));
    assert(bitmap_test(&bm, offset));
    {
        bitmap_set(&bm, offset, false);
        buf->dirty = true;
    }
    bwrite(buf);
}
int ialloc(dev_t dev)
{
    super_block_t *sb = read_super(dev);
    assert(sb);    
    bitmap_t bm;
    int offset = EOF;
    buffer_t *buf = NULL;
    for(size_t i = 0; i < sb->desc->imap_blocks; ++i) {
        buf = sb->imaps[i];
        bitmap_make(&bm, buf->data, BLOCK_SIZE, i * BLOCK_SIZE * BITLEN);
        offset = bitmap_scan(&bm, 1);
        if (offset!=EOF) {
            buf->dirty = true;
            break;
        }
    }
    bwrite(buf);
    return offset;
}
void ifree(dev_t dev, int idx)
{
    assert(idx!=EOF);
    super_block_t *sb = read_super(dev);
    int imap_nr = idx / (BITLEN * BLOCK_SIZE);
    assert(imap_nr < IMAP_MAX_NR);
    buffer_t *buf = sb->imaps[imap_nr];
    assert(buf);
    bitmap_t bm;
    bitmap_make(&bm, buf->data, BLOCK_SIZE, imap_nr *(BITLEN * BLOCK_SIZE));
    assert(bitmap_test(&bm, idx));
    {
        bitmap_set(&bm, idx, false);
        buf->dirty = true;
    }
    bwrite(buf);
}

// 获取 inode 第 block 块的索引值
// 如果不存在 且 create 为 true，则创建
int bmap(inode_t *inode, int block, bool create)
{
    // 确保 block 合法
    assert(block >= 0 && block < TOTAL_BLOCK);

    // 数组索引
    u16 index = block;

    // 数组
    u16 *array = inode->desc->zones;

    // 缓冲区
    buffer_t *buf = inode->buf;

    // 用于下面的 brelease，传入参数 inode 的 buf 不应该释放
    buf->count += 1;

    // 当前处理级别
    int level = 0;

    // 当前子级别块数量
    int divider = 1;

    // 直接块
    if (block < DIRECT_BLOCK)
    {
        goto reckon;
    }

    block -= DIRECT_BLOCK;

    if (block < INDIRECT1_BLOCK)
    {
        index = DIRECT_BLOCK;
        level = 1;
        divider = 1;
        goto reckon;
    }

    block -= INDIRECT1_BLOCK;
    assert(block < INDIRECT2_BLOCK);
    index = DIRECT_BLOCK + 1;
    level = 2;
    divider = BLOCK_INDEXES;

reckon:
    for (; level >= 0; level--)
    {
        // 如果不存在 且 create 则申请一块文件块
        if (!array[index] && create)
        {
            array[index] = balloc(inode->dev);
            buf->dirty = true;
        }
        
        // buffer 被写回磁盘，但是可用的 bnode 没有被释放，也就意味着这个信息已经被持久化了。 
        //只是快速内存不用而已。
        brelease(buf);

        // 如果 level == 0 或者 索引不存在，直接返回
        if (level == 0 || !array[index])
        {
            return array[index];
        }

        // level 不为 0，处理下一级索引
        buf = bread(inode->dev, array[index]);
        index = block / divider;
        block = block % divider;
        divider /= BLOCK_INDEXES;
        array = (u16 *)buf->data;
    }
}
