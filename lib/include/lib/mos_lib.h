// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#ifdef __MOS_KERNEL__
#include "mos/printk.h"
#define MOS_LIB_ASSERT(cond)             MOS_ASSERT(cond)
#define MOS_LIB_ASSERT_X(cond, msg, ...) MOS_ASSERT_X(cond, msg, ##__VA_ARGS__)
#define MOS_LIB_UNIMPLEMENTED(content)   MOS_UNIMPLEMENTED(content)
#define MOS_LIB_UNREACHABLE()            MOS_UNREACHABLE()
#else
// TODO: implement
#define MOS_LIB_ASSERT(cond)
#define MOS_LIB_ASSERT_X(cond, msg, ...)
#define MOS_LIB_UNIMPLEMENTED(content)
#define MOS_LIB_UNREACHABLE()
#define mos_warn(...)
inline void *liballoc_malloc(unsigned long size)
{
    (void) size;
    return (void *) 0;
}
inline void liballoc_free(const void *ptr)
{
    (void) ptr;
}
#define mos_panic(...)
#endif
