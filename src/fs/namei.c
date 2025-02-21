#include <jp/debug.h>
#include <jp/types.h>
#include <jp/fs.h>
#include <jp/stat.h>
#include <jp/string.h>
#include <jp/syscall.h>
#include <jp/task.h>

// 文件名是否相等
// name = "hello/world"
// entry_name = "hello"
// 这个函数之后 next 指向 "world" 开始的地方， 返回match=true
static bool match_name(const char *name, const char *entry_name, const char **next)
{
    const char* lhs = name, *rhs = entry_name;
    // @todo: check lhs and rhs addr valid
    while(*lhs && *rhs && *lhs == *rhs) {
        lhs++;
        rhs++;
    }
    if (*rhs) {
        return false;
    }
    if (*lhs && !IS_SEPARATOR(*lhs))
        return false;
    if (IS_SEPARATOR(*lhs))
        lhs++;
    *next = lhs;
    return true;
}

// 从目录inode中找是否有为 name 的项目
// dir == &rootInode("/"), fullpath "/hello/world, /test, /usr/bin"
// name == "/hello/world",
// *next = "world"
// *result = dentry_of("/hello")
// ret = buffer of dentry
static buffer_t *find_entry(inode_t **dir, const char *name,const char **next, dentry_t **result)
{
    assert(ISDIR((*dir)->desc->mode));
    inode_t *pdir = *dir;
    size_t i = 0, max_entry = pdir->desc->size / sizeof(dentry_t);
    buffer_t *buf = NULL;
    dentry_t *entry = NULL;
    int block = 0;
    
    for (i = 0; i < max_entry; ++i, entry++) {
        if (!buf || (u32)entry > (u32)buf->data + BLOCK_SIZE) {
            // switch to next buf
            if (buf)
                brelease(buf);
            block = bmap(pdir, i / BLOCK_DENTRIES, false);
            assert(block);
            buf = bread(pdir->dev, block);
            assert(buf);
            entry = (dentry_t*)buf->data;
        }
        if (match_name(name, entry->name, next)) {
            *result = entry;
            return buf;
        }
    }
    if (buf)
        brelease(buf);
    return NULL;
}

// 在dir目录中添加 name 目录项
// 1. 如果存在，那么直接返回
// 2. name中不能有分隔符
// 3.  ret = buffer of dentry
// 4. dentry 仅仅设置了name， inode nr 没有分配
static buffer_t *add_entry(inode_t *dir, const char *name, dentry_t  **result)
{
    const char *next = name;
    while(*next) {
        if (IS_SEPARATOR(*next)) {
            WARNK("can't create entry that has / or \\, name: %s",name);
            return NULL;
        }
        next++;
    }
    
    buffer_t *buf = NULL;
    dentry_t *entry = NULL;
    int block = 0;
    size_t i = 0;
    for(; true; i++,entry++) {
        if (!buf || (u32)entry >= (u32)buf->data+BLOCK_SIZE) {
            if (buf) {
                brelease(buf);
            }
            block = bmap(dir, i / BLOCK_DENTRIES, true);
            assert(block);
            buf = bread(dir->dev, block);
            assert(buf);
            entry = (dentry_t*)buf->data;
        }
        DEBUGK("get entry: "entry_fmt(entry));
        // 看这个entry是不是被新生成出来的
        if (i * sizeof(dentry_t) >=  dir->desc->size) {
            // memset new entry, do a create
            memset(entry, 0, sizeof(*entry));
            dir->desc->size += sizeof(*entry);
            dir->desc->mtime = time();
            dir->buf->dirty = true;
        } else{
            if(match_name(name, entry->name, &next)) {
                *result = entry;
                return buf;
            }
            continue;
        }
        if (entry->nr)
            panic("fs!!!");
        strncpy(entry->name, name, NAME_LEN);
        *result = entry;
        buf->dirty = true;
        return buf;
    }
    panic("can't run to here");
}

// this function is for debug, copy from onix@github
// static buffer_t *add_entry_v2(inode_t *dir, const char *name, dentry_t  **result)
// {
//     const char *next = NULL;
//     buffer_t *buf = find_entry(&dir, name, &next, result);
//     if (buf) {
//         return buf;
//     }
//     for (size_t i = 0; i < NAME_LEN && name[i]; ++i) {
//         assert(!IS_SEPARATOR(name[i]));
//     }

//     int i = 0, block = 0;
//     dentry_t *entry = NULL;
//     for (; true; i++, entry++) {
//         if (!buf || (u32)entry>=(u32)buf->data+BLOCK_SIZE) {
//             if (buf)
//                 brelease(buf);
//             block = bmap(dir, i/ BLOCK_DENTRIES, true);
//             assert(block);
//             buf = bread(dir->dev, block);
//             entry = (dentry_t*)buf->data;
//         }
//         if (i *sizeof(dentry_t) >= dir->desc->size) {
//             entry->nr = 0;
//             dir->desc->size = (i+1)*sizeof(dentry_t);
//             dir->buf->dirty = true;
//         }
//         if (entry->nr)
//             continue;
//         strncpy(entry->name, name, NAME_LEN);
//         buf->dirty = true;
//         dir->desc->mtime = time();
//         dir->buf->dirty = true;
//         *result = entry;
//         return buf;
//     }
//     return NULL;
// }

static bool permission(inode_t *inode, u16 mask)
{
    u16 mode = inode->desc->mode;
    if (!inode->desc->nlinks) {
        return false;
    }
    task_t *task = current;
    if (task->uid == KERNEL_USER) {
        return true;
    }

    if (task->uid == inode->desc->uid) {
        mode >>= 6;
    } else if (task->uid == inode->desc->gid) {
        mode >>= 3;
    }
    mode &= 0x3;
    return (mode & mask) == mask;
}

/// @brief named 返回 path 的父目录inode，例如 1. /root/.bashrc, 返回 root 的inode，next 指向 .bashrc
///                                          2. help/a.txt 返回 help 的inode，next指向 a.txt
///                                          3. a.txt 返回 help的inode， next为 a.txt
///                                          4. help/ 返回 help的inode， next为空
/// @param path 
/// @param next 
/// @return inode_t *
static inode_t *named(const char *path, const char **next)
{
    const char *left = path;
    inode_t *inode = NULL;
    task_t *task = current;
    if (IS_SEPARATOR(*left)) {
        inode = task->iroot;
        left++;
    } else if (left[0]) {
        inode = task->ipwd;
    } else {
        return NULL;
    }
    
    inode->count++;
    *next = left;
    if (!*left) {
        return inode;
    }
    const char *right = strrsep(left);
    // if left is "/", then left > right
    if (!right || right < left) {
        return inode;
    }
    right++;
    *next = left;
    buffer_t *buf = NULL;
    dentry_t *entry = NULL;
    while (true)
    {
        buf = find_entry(&inode, left, next, &entry);
        if (!buf) {
            inode = NULL;
            goto out;
        }
        dev_t dev = inode->dev;
        iput(inode);
        inode = iget(dev, entry->nr);
        assert(inode);
        if (!ISDIR(inode->desc->mode) || !permission(inode, P_EXEC)) {
            iput(inode);
            inode = NULL;
            goto out;
        }
        if (right == *next) {
            goto out;
        }
        left = *next;
        brelease(buf); // release this buf
    }
out:
    if (buf) {
        brelease(buf);
    }
    return inode;
}

static inode_t *namei(const char *path)
{
    const char *next = NULL;
    inode_t *dir = named(path, &next);
    if (!dir) {
        return NULL;
    } else if (!*next) {
        return dir;
    }
    dentry_t *entry = NULL;
    const char *name = next;
    inode_t *node = NULL;
    buffer_t *buf = find_entry(&dir, name, &next, &entry);
    if (!buf) {
        goto end;
    }
    node = iget(dir->dev, entry->nr);
end:
    iput(dir);
    if (buf) {
        brelease(buf);
    }
    return node;
}


#include <jp/task.h>

void dir_test()
{
    task_t *task = current;
    inode_t *root = task->iroot;
    root->count++;
    const char *next = NULL;
    dentry_t *entry = NULL;
    buffer_t *buf = NULL;
    dev_t dev = root->dev;

    // try to read /d1/d2/d3/d4
    char pathname[] = "d1/d2/d3/d4";
    const char *name = (const char *)pathname;
    
    // d1/d2/d3/d4
    DEBUGK("find in inode %d, name = %s", root->nr, name);
    buf = find_entry(&root, name, &next, &entry);
    DEBUGK("find entry: "entry_fmt(entry));
    iput(root);
    root = iget(dev, entry->nr);
    name = next;
    brelease(buf);

    // d2/d3/d4
    DEBUGK("find in inode %d, name = %s", root->nr, name);
    buf = find_entry(&root, name, &next, &entry);
    DEBUGK("find entry: "entry_fmt(entry));
    iput(root);
    root = iget(dev, entry->nr);
    name = next;
    brelease(buf);

    // d3/d4
    DEBUGK("find in inode %d, name = %s", root->nr, name);
    buf = find_entry(&root, name, &next, &entry);
    DEBUGK("find entry: "entry_fmt(entry));
    iput(root);
    root = iget(dev, entry->nr);
    name = next;
    brelease(buf);

    // d4
    DEBUGK("find in inode %d, name = %s", root->nr, name);
    buf = find_entry(&root, name, &next, &entry);
    DEBUGK("find entry: "entry_fmt(entry));
    iput(root);
    root = iget(dev, entry->nr);
    name = next;
    brelease(buf);

    assert(*next == 0);

    // // test add entry
    // root = task->iroot;
    // root->count++;
    // buf = find_entry(&root, "hello.txt", &next, &entry);
    // u16 nr = entry->nr;
    // buf = add_entry(root, "world.txt", &entry);
    // entry->nr = nr;
    

    // inode_t *hello = iget(dev, nr);
    // hello->desc->nlinks++;
    // hello->buf->dirty = true;

    // iput(hello);
    // iput(root);
    // brelease(buf);
    
    char rootname[] = "/";
    name = NULL;
    inode_t *inode = named(rootname, &name);
    iput(inode);

    inode = namei("/home/hello.txt");
    DEBUGK("get inode %d", inode->nr);
    iput(inode);
}