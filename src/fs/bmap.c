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