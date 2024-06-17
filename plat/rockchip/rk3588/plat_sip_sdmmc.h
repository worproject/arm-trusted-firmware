/*
 * Copyright (c) 2024, Mario Bălănică <mariobalanica02@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_SIP_SDMMC_H
#define PLAT_SIP_SDMMC_H

int plat_rk3588_sdmmc_set_signal_voltage(unsigned int microvolts);

uintptr_t rk_sip_sdmmc_handler(uint32_t smc_fid,
			       u_register_t x1,
			       u_register_t x2,
			       u_register_t x3,
			       u_register_t x4,
			       void *cookie,
			       void *handle,
			       u_register_t flags);

#endif /* PLAT_SIP_SDMMC_H */
