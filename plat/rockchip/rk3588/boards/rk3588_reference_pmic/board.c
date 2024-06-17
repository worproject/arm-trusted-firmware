/*
 * Copyright (c) 2024, Mario Bălănică <mariobalanica02@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <drivers/scmi-msg.h>
#include <scmi/rk3588_voltd.h>

#include <plat_private.h>

#include <plat_sip_sdmmc.h>

int plat_rk3588_sdmmc_set_signal_voltage(unsigned int microvolts)
{
	return plat_scmi_voltd_set_level(0, SCMI_VCCIO_SD_S0, microvolts);
}

void plat_rockchip_board_init(void)
{
	rk3588_reference_voltd_init();
}
