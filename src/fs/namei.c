#include <jp/debug.h>
#include <jp/types.h>
#include <jp/fs.h>

// 文件名是否相等
// name = "hello/world"
// entry_name = "hello"
// 这个函数之后 next 指向 "world" 开始的地方， 返回match=true
static bool match_name(const char *name, const char *entry_name, char **next)
{

}

// 从目录inode中找是否有为 name 的项目
// dir == &rootInode("/"), fullpath "/hello/world, /test, /usr/bin"
// name == "/hello/world",
// *next = "world"
// *result = dentry_of("/hello")
// ret = buffer of dentry
static buffer_t *find_entry(inode_t **dir, const char *name, char **next, dentry_t **result)
{

}

// 在dir目录中添加 name 目录项
// 1. 如果存在，那么直接返回
// 2. name中不能有分隔符
// 3.  ret = buffer of dentry
// 4. dentry 仅仅设置了name， inode nr 没有分配
static buffer_t *add_entry(inode_t *dir, const char *name, dentry_t  **result)
{

}