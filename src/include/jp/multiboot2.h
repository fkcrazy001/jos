#pragma once

#include <jp/types.h>
// PART FROM MULTIBOOT.H

#define MULTIBOOT2_MAGIC 0x36d76289

#define MULTIBOOT_TAG_TYPE_END 0
#define MULTIBOOT_TAG_TYPE_MMAP 6

#define MULTIBOOT_MEMORY_AVAILABLE 1
#define MULTIBOOT_MEMORY_RESERVED 2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE 3
#define MULTIBOOT_MEMORY_NVS 4
#define MULTIBOOT_MEMORY_BADRAM 5

// memory and other infos provided by multiboot
typedef struct multi_tag
{
    u32 type;
    u32 size;
} multi_tag_t;

// memory info 
typedef struct multi_mmap_entry
{
    u64 addr;
    u64 len;
    u32 type;
    u32 zero;
} multi_mmap_entry_t;

// one realize of multi_tag
typedef struct multi_tag_mmap
{
    u32 type;
    u32 size;
    u32 entry_size;
    u32 entry_version;
    multi_mmap_entry_t entries[0];
}multi_tag_mmap_t;