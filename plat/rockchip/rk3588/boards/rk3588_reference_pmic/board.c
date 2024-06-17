/*
 * Copyright (c) 2024, Mario Bălănică <mariobalanica02@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <scmi/rk3588_voltd.h>

#include <plat_private.h>

void plat_rockchip_board_init(void)
{
	rk3588_reference_voltd_init();
}
