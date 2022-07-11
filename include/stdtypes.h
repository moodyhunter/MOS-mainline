// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if __WORDSIZE == 64
#error "MOS is not supported on 64-bit systems"
#endif

#define NULL ((void *) 0)

typedef char s8;
typedef unsigned char u8;
typedef short s16;
typedef unsigned short u16;
typedef int s32;
typedef unsigned int u32;
typedef long long s64;
typedef unsigned long long u64;
typedef float f32;
typedef double f64;
typedef long double f80;

typedef u32 size_t;
typedef s32 ssize_t;
typedef u32 pid_t;
typedef u32 uid_t;
typedef u32 gid_t;
typedef u32 mode_t;
typedef u32 dev_t;
typedef u32 ino_t;

typedef s8 int8_t;
typedef u8 uint8_t;
typedef s16 int16_t;
typedef u16 uint16_t;
typedef s32 int32_t;
typedef u32 uint32_t;
typedef s64 int64_t;
typedef u64 uint64_t;

typedef u32 uintptr_t;
typedef u32 uintmax_t;