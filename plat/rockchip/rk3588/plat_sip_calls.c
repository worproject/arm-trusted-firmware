/*
 * Copyright (c) 2024, Rockchip, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <common/runtime_svc.h>
#include <drivers/scmi-msg.h>

#include <plat_sip_calls.h>
#include <rockchip_sip_svc.h>

#include "plat_sip_sdmmc.h"

uintptr_t rockchip_plat_sip_handler(uint32_t smc_fid,
				    u_register_t x1,
				    u_register_t x2,
				    u_register_t x3,
				    u_register_t x4,
				    void *cookie,
				    void *handle,
				    u_register_t flags)
{
	if (GET_SMC_CC(smc_fid) == SMC_32) {
		/* Clear top bits */
		x1 = (uint32_t)x1;
		x2 = (uint32_t)x2;
		x3 = (uint32_t)x3;
		x4 = (uint32_t)x4;
	} else {
		/*
		 * ARM DEN 0028F:
		 * SMC64/HVC64 calls are expected to be the 64-bit
		 * equivalent to the 32-bit call, where applicable.
		 */
		smc_fid &= ~(SMC_64 << FUNCID_CC_SHIFT);
	}

	switch (smc_fid) {
	case RK_SIP_SCMI_AGENT0:
		scmi_smt_fastcall_smc_entry(0);
		SMC_RET1(handle, 0);

	case RK_SIP_SDMMC_CLOCK_RATE_GET:
	case RK_SIP_SDMMC_CLOCK_RATE_SET:
	case RK_SIP_SDMMC_CLOCK_PHASE_GET:
	case RK_SIP_SDMMC_CLOCK_PHASE_SET:
	case RK_SIP_SDMMC_REGULATOR_VOLTAGE_GET:
	case RK_SIP_SDMMC_REGULATOR_VOLTAGE_SET:
	case RK_SIP_SDMMC_REGULATOR_ENABLE_GET:
	case RK_SIP_SDMMC_REGULATOR_ENABLE_SET:
		return rk_sip_sdmmc_handler(smc_fid, x1, x2, x3, x4, cookie, handle, flags);

	default:
		ERROR("%s: unhandled SMC (0x%x)\n", __func__, smc_fid);
		SMC_RET1(handle, SMC_UNK);
	}
}
