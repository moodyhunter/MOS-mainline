// SPDX-License-Identifier: GPL-3.0-or-later

#include "mos/printk.h"

u64 __stack_chk_guard = 0;

noreturn void __stack_chk_fail(void)
{
    mos_panic("Stack smashing detected!");
}

void __stack_chk_fail_local(void)
{
    __stack_chk_fail();
}
