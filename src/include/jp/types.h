#ifndef _TYPES_H_
#define _TYPES_H_

#define EOF   -1
#define EOS   '\0'
#define NULL  (void*)0

#ifndef __cplusplus
#define  bool   _Bool
#define  true   1
#define  false  0
#endif

#define _ofp //__attribute__((optimize("omit-frame-pointer")))
#define  _packed  __attribute__((packed))

typedef  unsigned int  size_t;

typedef  char int8_t;
typedef  short  int16_t;
typedef  int    int32_t;
typedef  long long  int64_t;

typedef  unsigned char uint8_t;
typedef  unsigned short  uint16;
typedef  unsigned int    uint32_t;
typedef  unsigned long long  uint64_t;

typedef  unsigned char u8;
typedef  unsigned short  u16;
typedef  unsigned int    u32;
typedef  unsigned long long  u64;

typedef u32 time_t;

#endif