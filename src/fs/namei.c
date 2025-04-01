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
            if(entry->nr){
                if (match_name(name, entry->name, &next)) {
                    *result = entry;
                    return buf;
                }
                continue;
            }
            // rmdir let entry->nr == 0;
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

int sys_mkdir(u32 path, u32 mode)
{
    int ret = EOF;
    const char *path_s = (const char *)path;
    const char *next = NULL;
    dentry_t *entry = NULL;
    buffer_t *entry_buf = NULL;
    inode_t *dir = NULL;
    const char *name = NULL;
    dir = named(path_s, &next);
    
    if (!dir || !*next) {
        DEBUGK("can't find parent dir, dir %p, *next = %c", dir, *next);
        goto out;
    }
    
    if (!permission(dir, P_WRITE)) {
        WARNK("not write permission on parent dir");
        goto out;   
    }

    name = next;
    entry_buf = find_entry(&dir, name, &next, &entry);
    if (entry_buf) {
        DEBUGK("find existing dir: %s", path_s);
        ret = 0;
        goto out;
    }

    entry_buf = add_entry(dir, name, &entry);
    if (!entry_buf) {
        WARNK("can't create entry %s", name);
        ret = EOF;
        goto out;
    }
    entry->nr = ialloc(dir->dev);
    assert(entry->nr);
    entry_buf->dirty = true;

    task_t *task = current;
    inode_t *inode = iget(dir->dev, entry->nr);
    assert(inode);
    inode->buf->dirty = true;
    
    inode->desc->gid = task->gid;
    inode->desc->mode = (u16)((mode&0777) & (~task->umask) | IFDIR);
    inode->desc->mtime = time();
    inode->desc->nlinks = 2; // 一个是 当前目录下的. ，另外一个是父目录下的nr
    inode->desc->size = sizeof(dentry_t) * 2;
    inode->desc->uid = task->uid;
    // write real data
    buffer_t *zbuf = bread(dir->dev, bmap(inode, 0, true));
    dentry_t *tmp_entry = zbuf->data;
    strncpy(tmp_entry->name, ".", NAME_LEN);
    tmp_entry->nr = entry->nr;
    tmp_entry++;
    strncpy(tmp_entry->name, "..", NAME_LEN);
    tmp_entry->nr = dir->nr;
    zbuf->dirty = true;
    brelease(zbuf);
    iput(inode);
    // parent dir process
    dir->desc->nlinks += 1;
    dir->buf->dirty = true;
    ret = 0;
out:
    if (dir)
        iput(dir);
    if (entry_buf)
        brelease(entry_buf);
    
    return ret;
}

static bool dir_inode_is_empty(inode_t *dir)
{
    assert(ISDIR(dir->desc->mode));
    int entries = dir->desc->size / (sizeof(dentry_t));
    if (entries < 2 || !dir->desc->zones[0]) {
        panic("bad fs!!!");
    }
    int i = 0, cnt = 0;
    buffer_t *buf = NULL;
    dentry_t *entry = NULL;
    for (i = 0; i < entries;entry++, ++i) {
        if (!buf || (char*)entry >= ((char*)buf->data + BLOCK_SIZE)) {
            if (buf) {
                brelease(buf);
            }
            int block = bmap(dir, i/ BLOCK_DENTRIES, false);
            assert(block);
            buf = bread(dir->dev, block);
            assert(buf);
            entry = (dentry_t*)buf->data;
        }
        if (entry->nr)
            cnt++;
    }
    if (buf) {
        brelease(buf);
    }
    if (cnt < 2) {
        panic("bad fs!!!");
    }
    return cnt == 2;
}

int sys_rmdir(u32 path)
{
    int ret = EOF;
    const char *path_s = (const char *)path;
    const char *next = NULL;
    dentry_t *entry = NULL;
    buffer_t *entry_buf = NULL;
    inode_t *dir = NULL;
    const char *name = NULL;
    inode_t *inode = NULL;
    dir = named(path_s, &next);
    
    if (!dir || !*next) {
        DEBUGK("can't find parent dir, dir %p, *next = %c", dir, *next);
        goto out;
    }
    
    if (!permission(dir, P_WRITE)) {
        WARNK("not write permission on parent dir");
        goto out;   
    }

    name = next;
    entry_buf = find_entry(&dir, name, &next, &entry);
    if (!entry_buf) {
        WARNK("can't find existing dir: %s", path_s);
        goto out;
    }
    inode = iget(dir->dev, entry->nr);
    if (!inode || inode == dir || !ISDIR(inode->desc->mode)) {
        WARNK("something unexpected happened!");
        goto out;
    }
    task_t *task = current;
    if((dir->desc->mode & ISVTX) && task->uid != inode->desc->uid) {
        DEBUGK("user %d has no permission to write dir %s", task->uid, name);
        goto out;
    }
    if (dir->dev != inode->dev || inode->count > 1) {
        DEBUGK("check failed");
        goto out;
    }

    if (!dir_inode_is_empty(inode)) {
        WARNK("can't delete dir which is not empty!!!");
        goto out;
    }

    inode_truncate(inode);
    ifree(inode->dev, inode->nr);
    inode->desc->nlinks = 0;
    inode->buf->dirty = true;
    inode->nr = 0;

    entry->nr = 0;
    memset(entry->name, 0, sizeof(entry->name));
    entry_buf->dirty = true;
    dir->desc->nlinks -= 1;
    assert(dir->desc->nlinks>0);
    dir->atime = dir->ctime = dir->desc->mtime = time();
    dir->buf->dirty = true;
    ret = 0;
out:
    if (inode) {
        iput(inode);
    }
    if (dir)
        iput(dir);
    if (entry_buf)
        brelease(entry_buf);
    
    return ret;
}

int sys_link(u32 old_path, u32 new_path)
{
    int ret = EOF;
    const char *old_path_name = (const char*)old_path;
    const char *new_path_name = (const char*)new_path;
    inode_t *old_file_inode = namei(old_path_name);
    inode_t *new_file_inode = NULL;
    inode_t *new_file_parent = NULL;
    buffer_t *new_file_dir_entry_buffer = NULL;
    dentry_t *new_file_dir_entry = NULL;
    if (!old_file_inode || !ISFILE(old_file_inode->desc->mode)) {
        WARNK("invalid file %s", old_path_name);
        goto out;
    }
    const char *next;
    new_file_parent = named(new_path_name, &next);
    if (!new_file_parent  || !permission(new_file_parent, P_WRITE)) {
        WARNK("can't find %s parent dir or no permission to write", new_file_parent);
        goto out;
    }
    if (!*next) {
        WARNK("file name invalid!");
        goto out;
    }
    const char *name = next;
    new_file_dir_entry_buffer = find_entry(&new_file_parent, name, &next, &new_file_dir_entry);
    if (new_file_dir_entry_buffer) {
        DEBUGK("file  %s existing", new_path_name);
        ret = 0;
        goto out;
    }
    if (new_file_parent->dev != old_file_inode->dev) {
        WARNK("can't create hardlink between 2 device");
        goto out;
    }
    new_file_dir_entry_buffer = add_entry(new_file_parent, name, &new_file_dir_entry);
    if (!new_file_dir_entry_buffer) {
        WARNK("can't create entry for %s on parent dir", new_path_name);
        goto out;
    }
    new_file_dir_entry->nr = old_file_inode->nr;
    new_file_dir_entry_buffer->dirty = true;
    
    old_file_inode->desc->nlinks++;
    old_file_inode->buf->dirty = true;
    old_file_inode->atime = time();
    ret = 0;
out:
    if (new_file_dir_entry_buffer) {
        brelease(new_file_dir_entry_buffer);
    }
    if (new_file_parent) {
        iput(new_file_parent);
    }
    if (new_file_inode) {
        iput(new_file_inode);
    }
    if (old_file_inode)
        iput(old_file_inode);
    return ret;
}

int sys_unlink(u32 file_path)
{
    int ret = EOF;
    const char *file_path_name = (const char *)file_path;
    const char *next = NULL;
    inode_t *file_dir_inode = named(file_path_name, &next);
    buffer_t *file_entry_buf = NULL;
    inode_t *file_inode = NULL;
    if (!file_dir_inode) {
        WARNK("can't find file %s parent dir", file_path_name);
        goto out;
    }
    if (!*next) {
        WARNK("name %s invalid ", file_path_name);
        goto out;
    }
    if (!permission(file_dir_inode, P_WRITE)) {
        WARNK("no write permission on parent dir");
        goto out;
    }
    const char *name = next;
    dentry_t *file_entry = NULL;
    file_entry_buf = find_entry(&file_dir_inode, name, &next, &file_entry);
    if (!file_entry_buf) {
        WARNK("file not exist !!!");
        goto out;
    }
    file_inode = iget(file_dir_inode->dev, file_entry->nr);
    if (!file_inode || !ISFILE(file_inode->desc->mode)) {
        WARNK("can't get file inode or not file");
        goto out;
    }
    task_t *task = current;
    if ((file_inode->desc->mode & ISVTX) && task->uid != file_inode->desc->uid) {
        WARNK("can't delete VTX file, file->uid %d, current uid %d", file_inode->desc->uid, task->uid);
        goto out;
    }
    assert(file_inode->desc->nlinks);
    file_entry->nr = 0;
    file_entry_buf->dirty = true;

    // file_dir_inode->desc->size -= sizeof(*file_entry); // don't reduce size, nr = 0 says this is invalid
    file_dir_inode->buf->dirty = true;

    file_inode->desc->nlinks--;
    file_inode->buf->dirty = true;
    if (!file_inode->desc->nlinks) {
        inode_truncate(file_inode);
        ifree(file_inode->dev, file_inode->nr);
    }
    ret = 0;
out:
    if (file_inode) {
        iput(file_inode);
    }
    if (file_entry_buf) {
        brelease(file_entry_buf);
    }
    if (file_dir_inode)
        iput(file_dir_inode);
    return ret;
}

inode_t *inode_open(const char *pathname, int flags, int mode)
{
    inode_t *dir = NULL;
    inode_t *file = NULL;
    buffer_t *entry_buf = NULL;
    dentry_t *entry = NULL;
    const char *next = NULL;

    dir = named(pathname, &next);
    if (!dir || !*next) {
        DEBUGK("input file name invalid: %s", pathname);
        goto out;
    }
    if ((flags & O_TRUNC) && (flags & O_ACCMODE) == O_RDONLY) {
        DEBUGK("trunc but with only read flag, we give it a write permit");
        flags |= O_WRONLY;
    }
    const char *name = next;
    entry_buf = find_entry(&dir, name, &next, &entry);
    if (entry_buf) {
        DEBUGK("find file: req %s, disk %s", name, entry->name);
        file = iget(dir->dev, entry->nr);
        goto makeup;
    }
    if (!(flags & O_CREATE)) {
        DEBUGK("no create mode on not exist file");
        goto out;
    }
    if (!permission(dir, P_WRITE)) {
        DEBUGK("no write permission on dir");
        goto out;
    }
    entry_buf = add_entry(dir, name, &entry);
    assert(entry_buf);
    entry->nr = ialloc(dir->dev);
    assert(entry->nr);
    entry_buf->dirty = true;
    
    task_t *task = current;
    file = iget(dir->dev, entry->nr);
    assert(file);
    file->buf->dirty = true;
    
    file->desc->gid = task->gid;
    file->desc->mode = (u16)((mode&0777) & (~task->umask) | IFREG);
    file->desc->mtime = time();
    file->desc->nlinks = 1; // self 
    file->desc->size = 0;
    file->desc->uid = task->uid;
makeup:
    assert(file);
    if (ISDIR(file->desc->mode) || !permission(file, flags&O_ACCMODE)) {
        DEBUGK("permission check failed");
        iput(file);
        file = NULL;
        goto out;
    }
    file->atime = time();
    if (flags & O_TRUNC)
        inode_truncate(file);
out:
    if (entry_buf)
        brelease(entry_buf);
    if (dir)
        iput(dir);
    return file;
}

#include <jp/memory.h>

void dir_test()
{
    char* buf = (char*)alloc_kpage(1);
    inode_t *inode = namei("/home");
    
    int i = inode_read(inode, buf, PAGE_SIZE, 0);
    DEBUGK("read %d bytes, content: %s", i, buf);

    // inode_truncate(inode);
    // i = inode_read(inode, buf, PAGE_SIZE, 0);
    // DEBUGK("read %d bytes, content: %s", i, buf);

    iput(inode);
    free_kpage((u32)buf, 1);
}