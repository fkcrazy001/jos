#include <jp/fs.h>
#include <jp/debug.h>
#include <jp/string.h>
#include <jp/task.h>

#define KERNEL_MAX_OPEN_FILE 128

static file_t file_table[KERNEL_MAX_OPEN_FILE];

file_t *get_file(void)
{
    size_t i = 0;
    for (i = 0; i < KERNEL_MAX_OPEN_FILE; ++i) {
        file_t *f = &file_table[i];
        if (!f->count) {
            f->count ++;
            return f;
        }
    }
    panic("system running out of files");    
}

void put_file(file_t *f)
{
    assert(f->count > 0);
    f->count--;
    if (!f->count) {
        iput(f->inode);
    }
}

void file_init(void)
{
    // a trick to set all data to zero
    memset(file_table, 0, sizeof(file_table));
}

fd_t sys_open(u32 path, int flags, int mode)
{
    const char *path_name = (const char*)path;
    inode_t *file_inode = inode_open(path_name, flags, mode);
    if (!file_inode) {
        return EOF;
    }
    file_t *file = get_file();
    assert(file);

    task_t *task = current;
    fd_t fd = task_get_fd(task);
    assert(!task->files[fd]);
    
    task->files[fd] = file;
    file->inode = file_inode;
    file->flags = flags;
    file->mode = file_inode->desc->mode;
    file->offset = 0;

    if (flags & O_APPEND) {
        file->offset = file_inode->desc->size;
    }
    return fd;
}

int sys_create(u32 path, int mode)
{
    return sys_open(path, O_CREATE|O_TRUNC, mode);
}

int sys_close(fd_t fd)
{
    assert(fd < TASK_MAX_OPEN_FILE);
    task_t *task = current;
    file_t *f = task->files[fd];
    if (!f) {
        return EOF;
    }
    assert(f->inode);
    assert(f->count);
    put_file(f);
    task_put_fd(task, fd);
    return 0;
}

int sys_read(fd_t fd, u32 buf_addr, u32 size)
{
    assert(fd < TASK_MAX_OPEN_FILE);
    // @todo check buf_addr
    char *buf = (char *)buf_addr;

    if (fd == stdin) {
        device_t *dev = device_find(DEV_KEYBOARD, 0);
        assert(dev);
        return device_read(dev->dev, buf, size, 0, 0);
    }

    task_t *task = current;
    file_t *f = task->files[fd];
    if (!f || !f->inode || (f->flags & O_ACCMODE == O_WRONLY)) {
        DEBUGK("invalid fd %d get, file %p", fd, f);
        return EOF;
    }
    inode_t *inode = f->inode;
    int len = inode_read(inode, buf, size, f->offset);
    if (len != EOF) {
        f->offset += len;
    }
    return len;
}

int sys_write(fd_t fd, u32 buf_addr, u32 size)
{
    assert(fd < TASK_MAX_OPEN_FILE);
    // @todo check buf_addr
    char *buf = (char *)buf_addr;

    if (fd == stdout || fd == stderr) {
        device_t *dev = device_find(DEV_CONSOLE, 0);
        assert(dev);
        return device_write(dev->dev, buf, size, 0, 0);
    }

    task_t *task = current;
    file_t *f = task->files[fd];
    if (!f || !f->inode || (f->flags & O_ACCMODE == O_RDONLY)) {
        DEBUGK("invalid fd %d get, file %p", fd, f);
        return EOF;
    }
    inode_t *inode = f->inode;
    int len = inode_write(inode, buf, size, f->offset);
    if (len != EOF) {
        f->offset += len;
    }
    return len;
}