// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "lib/liballoc.h"
#include "lib/mos_lib.h"
#include "libuserspace.h"
#include "mos/mos_global.h"

MOSAPI __malloc void *malloc(size_t size);
MOSAPI void free(void *ptr);
MOSAPI void *calloc(size_t nmemb, size_t size);
MOSAPI void *realloc(void *ptr, size_t size);
