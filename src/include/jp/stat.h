#pragma once

#include <jp/types.h>
#include <jp/device.h>

// file type
#define IFMT 0xF000
#define IFREG 0x8000
#define IFBLK 0x6000
#define IFDIR 0x4000
#define IFCHR 0x2000
#define IFIFO 0x1000
#define IFSYM 0xA000

// 文件属性位
// ISUID 测试文件的 set-user-id 是否置位
// 如果置为，则执行当前文件时，进程的eid将被设置为该文件宿主的uid
// 例如sudo
// ISGID 是设置组的
#define ISUID 0x800
#define ISGID 0x400

// 如果置为，则非文件用户没有删除权限
#define ISVTX 0x200

#define ISREG(m) (((m)&IFMT) == IFREG)   // 是常规文件
#define ISDIR(m) (((m)&IFMT) == IFDIR)   // 是目录文件
#define ISCHR(m) (((m)&IFMT) == IFCHR)   // 是字符设备文件
#define ISBLK(m) (((m)&IFMT) == IFBLK)   // 是块设备文件
#define ISFIFO(m) (((m)&IFMT) == IFIFO)  // 是 FIFO 特殊文件
#define ISLNK(m) (((m)&IFMT) == IFLNK)   // 是符号连接文件
#define ISSOCK(m) (((m)&IFMT) == IFSOCK) // 是套接字文件
#define ISFILE(m) ISREG(m)               // 更直观的一个宏

// 文件访问权限
#define IRWXU 00700 // 宿主可以读、写、执行/搜索
#define IRUSR 00400 // 宿主读许可
#define IWUSR 00200 // 宿主写许可
#define IXUSR 00100 // 宿主执行/搜索许可

#define IRWXG 00070 // 组成员可以读、写、执行/搜索
#define IRGRP 00040 // 组成员读许可
#define IWGRP 00020 // 组成员写许可
#define IXGRP 00010 // 组成员执行/搜索许可

#define IRWXO 00007 // 其他人读、写、执行/搜索许可
#define IROTH 00004 // 其他人读许可
#define IWOTH 00002 // 其他人写许可
#define IXOTH 00001 // 其他人执行/搜索许可

typedef struct stat_t
{
    dev_t dev;    // 含有文件的设备号
    int nr;     // 文件 i 节点号
    u16 mode;     // 文件类型和属性
    u8 nlinks;    // 指定文件的连接数
    u16 uid;      // 文件的用户(标识)号
    u8 gid;       // 文件的组号
    dev_t rdev;   // 设备号(如果文件是特殊的字符文件或块文件)
    size_t size;  // 文件大小（字节数）（如果文件是常规文件）
    time_t atime; // 上次（最后）访问时间
    time_t mtime; // 最后修改时间
    time_t ctime; // 最后节点修改时间
} stat_t;