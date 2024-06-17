/*
 * Copyright (c) 2024, Mario Bălănică <mariobalanica02@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <common/runtime_svc.h>
#include <drivers/scmi-msg.h>
#include <lib/mmio.h>

#include <platform_def.h>
#include <rockchip_sip_svc.h>
#include <soc.h>

#include <scmi_clock.h>
#include <rk3588_clk.h>

#include "plat_sip_sdmmc.h"

/* This is board-specific (see rk3588_reference_pmic) */
#pragma weak plat_rk3588_sdmmc_set_signal_voltage

#define DIV_ROUND_CLOSEST(x, divisor) (((x) + ((divisor) / 2)) / (divisor))

#define PICOSECONDS_PER_SECOND	1000000000000ULL

#define CRU_SD_CON_SEL			BIT(11)
#define CRU_SD_CON_DELAYNUM_SHIFT	3
#define CRU_SD_CON_DELAYNUM_MASK 	GENMASK_32(10, 3)
#define CRU_SD_CON_DEGREE_SHIFT		1
#define CRU_SD_CON_DEGREE_MASK		GENMASK_32(2, 1)
#define CRU_SD_CON_INIT_STATE		BIT(0)
#define CRU_SD_CON_MASK			GENMASK_32(11, 0)

#define CRU_SD_CON_DELAYNUM_MAX      255
#define CRU_SD_CON_DEGREE_STEP       90

/*
 * RK3588 TRM-Part2 3.6.7:
 * The delay time of every element is in the range of 36ps~68ps,
 * varying with different voltage and temperature.
 */
#define CRU_SD_DELAY_ELEMENT_PS_MIN	36
#define CRU_SD_DELAY_ELEMENT_PS_MAX	68
#define CRU_SD_DELAY_ELEMENT_PS  \
	((CRU_SD_DELAY_ELEMENT_PS_MIN + CRU_SD_DELAY_ELEMENT_PS_MAX) / 2)

#define CRU_SD_CLKGEN_DIV	2

static unsigned int cru_sd_get_phase(unsigned int con_reg,
				     unsigned int rate_hz)
{
	unsigned int val;
	unsigned int phase_degrees;
	unsigned long long delaynum;

	if (rate_hz == 0) {
		return 0;
	}

	val = mmio_read_32(con_reg);

	phase_degrees = (val & CRU_SD_CON_DEGREE_MASK) >> CRU_SD_CON_DEGREE_SHIFT;
	phase_degrees *= CRU_SD_CON_DEGREE_STEP;

	if ((val & CRU_SD_CON_SEL) != 0) {
		delaynum = (val & CRU_SD_CON_DELAYNUM_MASK) >> CRU_SD_CON_DELAYNUM_SHIFT;
		delaynum *= CRU_SD_DELAY_ELEMENT_PS * rate_hz * 360ULL;

		phase_degrees += DIV_ROUND_CLOSEST(delaynum, PICOSECONDS_PER_SECOND);
	}

	return phase_degrees % 360;
}

static void cru_sd_set_phase(unsigned int con_reg,
			     unsigned int rate_hz,
			     unsigned int phase_degrees)
{
	unsigned int degree_sel, remaining_degrees;
	unsigned long long delaynum;
	unsigned int val = 0;

	if (rate_hz == 0) {
		return;
	}

	degree_sel = phase_degrees / CRU_SD_CON_DEGREE_STEP;
	remaining_degrees = (phase_degrees % CRU_SD_CON_DEGREE_STEP);

	delaynum = DIV_ROUND_CLOSEST(PICOSECONDS_PER_SECOND * remaining_degrees,
			CRU_SD_DELAY_ELEMENT_PS * rate_hz * 360ULL);

	delaynum = MIN(delaynum, (unsigned long long)CRU_SD_CON_DELAYNUM_MAX);

	if (delaynum) {
		val |= CRU_SD_CON_SEL;
	}
	val |= (delaynum << CRU_SD_CON_DELAYNUM_SHIFT) & CRU_SD_CON_DELAYNUM_MASK;
	val |= (degree_sel << CRU_SD_CON_DEGREE_SHIFT) & CRU_SD_CON_DEGREE_MASK;

	mmio_write_32(con_reg, (CRU_SD_CON_MASK << 16) | val);
}

static int get_mshc_phase_shift_cru_reg(uintptr_t controller_address,
					unsigned int id,
					unsigned int *cru_reg)
{
	switch (controller_address) {
	case SDMMC_BASE:
		switch (id) {
		case RK_SIP_SDMMC_CLOCK_ID_MSHC_CIU_DRIVE:
			*cru_reg = CRU_BASE + CRU_SDMMC_CON0;
			return RK_SIP_E_SUCCESS;
		case RK_SIP_SDMMC_CLOCK_ID_MSHC_CIU_SAMPLE:
			*cru_reg = CRU_BASE + CRU_SDMMC_CON1;
			return RK_SIP_E_SUCCESS;
		}
		break;
	case SDIO_BASE:
		switch (id) {
		case RK_SIP_SDMMC_CLOCK_ID_MSHC_CIU_DRIVE:
			*cru_reg = CRU_BASE + CRU_SDIO_CON0;
			return RK_SIP_E_SUCCESS;
		case RK_SIP_SDMMC_CLOCK_ID_MSHC_CIU_SAMPLE:
			*cru_reg = CRU_BASE + CRU_SDIO_CON1;
			return RK_SIP_E_SUCCESS;
		}
		break;
	}

	return RK_SIP_E_NOT_IMPLEMENTED;
}

static int get_sdmmc_card_clock_scmi_id(uintptr_t controller_address,
					unsigned int id,
					unsigned int *scmi_id)
{
	switch (controller_address) {
	case SDMMC_BASE:
		switch (id) {
		case RK_SIP_SDMMC_CLOCK_ID_MSHC_CIU:
			*scmi_id = SCMI_CCLK_SD;
			return RK_SIP_E_SUCCESS;
		}
		break;
	case EMMC_BASE:
		switch (id) {
		case RK_SIP_SDMMC_CLOCK_ID_EMMC_CCLK:
			*scmi_id = SCMI_CCLK_EMMC;
			return RK_SIP_E_SUCCESS;
		}
		break;
	}

	return RK_SIP_E_NOT_IMPLEMENTED;
}

static int rk_sip_sdmmc_clock_rate_get(uintptr_t controller_address,
				       unsigned int id,
				       unsigned int *rate_hz)
{
	int ret;
	unsigned int scmi_id;

	ret = get_sdmmc_card_clock_scmi_id(controller_address, id, &scmi_id);
	if (ret) {
		return ret;
	}

	*rate_hz = plat_scmi_clock_get_rate(0, scmi_id);
	if (*rate_hz == 0) {
		return RK_SIP_E_DEVICE_ERROR;
	}

	return RK_SIP_E_SUCCESS;
}

static int rk_sip_sdmmc_clock_rate_set(uintptr_t controller_address,
				       unsigned int id,
				       unsigned int rate_hz)
{
	int ret;
	unsigned int scmi_id;

	ret = get_sdmmc_card_clock_scmi_id(controller_address, id, &scmi_id);
	if (ret) {
		return ret;
	}

	ret = plat_scmi_clock_set_rate(0, scmi_id, rate_hz);
	if (ret) {
		return RK_SIP_E_DEVICE_ERROR;
	}

	return RK_SIP_E_SUCCESS;
}

static int rk_sip_sdmmc_clock_phase_get(uintptr_t controller_address,
					unsigned int id,
					unsigned int *phase_degrees)
{
	int ret;
	unsigned int cru_reg;
	unsigned int rate_hz;

	ret = get_mshc_phase_shift_cru_reg(controller_address, id, &cru_reg);
	if (ret) {
		return ret;
	}

	ret = rk_sip_sdmmc_clock_rate_get(controller_address,
					  RK_SIP_SDMMC_CLOCK_ID_MSHC_CIU,
					  &rate_hz);
	if (ret) {
		return ret;
	}

	rate_hz /= CRU_SD_CLKGEN_DIV;

	*phase_degrees = cru_sd_get_phase(cru_reg, rate_hz);

	return RK_SIP_E_SUCCESS;
}

static int rk_sip_sdmmc_clock_phase_set(uintptr_t controller_address,
					unsigned int id,
					unsigned int phase_degrees)
{
	int ret;
	unsigned int cru_reg;
	unsigned int rate_hz;

	ret = get_mshc_phase_shift_cru_reg(controller_address, id, &cru_reg);
	if (ret) {
		return ret;
	}

	ret = rk_sip_sdmmc_clock_rate_get(controller_address,
					  RK_SIP_SDMMC_CLOCK_ID_MSHC_CIU,
					  &rate_hz);
	if (ret) {
		return ret;
	}

	rate_hz /= CRU_SD_CLKGEN_DIV;

	cru_sd_set_phase(cru_reg, rate_hz, phase_degrees);

	return RK_SIP_E_SUCCESS;
}

static int rk_sip_sdmmc_regulator_voltage_get(uintptr_t controller_address,
					      unsigned int id,
					      unsigned int *microvolts)
{
	return RK_SIP_E_NOT_IMPLEMENTED;
}

static int rk_sip_sdmmc_regulator_voltage_set(uintptr_t controller_address,
					      unsigned int id,
					      unsigned int microvolts)
{
	int ret;

	switch (controller_address) {
	case SDMMC_BASE:
		switch (id) {
		case RK_SIP_SDMMC_REGULATOR_ID_SIGNAL:
			ret = plat_rk3588_sdmmc_set_signal_voltage(microvolts);
			if (ret) {
				return RK_SIP_E_DEVICE_ERROR;
			}
			return RK_SIP_E_SUCCESS;
		}
		break;
	}

	return RK_SIP_E_NOT_IMPLEMENTED;
}

static int rk_sip_sdmmc_regulator_enable_get(uintptr_t controller_address,
					     unsigned int id,
					     bool *enable)
{
	return RK_SIP_E_NOT_IMPLEMENTED;
}

static int rk_sip_sdmmc_regulator_enable_set(uintptr_t controller_address,
					     unsigned int id,
					     bool enable)
{
	return RK_SIP_E_NOT_IMPLEMENTED;
}

int plat_rk3588_sdmmc_set_signal_voltage(unsigned int microvolts)
{
	return RK_SIP_E_NOT_IMPLEMENTED;
}

uintptr_t rk_sip_sdmmc_handler(uint32_t smc_fid,
			       u_register_t x1,
			       u_register_t x2,
			       u_register_t x3,
			       u_register_t x4,
			       void *cookie,
			       void *handle,
			       u_register_t flags)
{
	int ret;

	switch (smc_fid) {
	case RK_SIP_SDMMC_CLOCK_RATE_GET: {
		unsigned int rate_hz = 0;

		ret = rk_sip_sdmmc_clock_rate_get(x1, x2, &rate_hz);
		if (!ret) {
			SMC_RET2(handle, RK_SIP_E_SUCCESS, rate_hz);
		} else {
			SMC_RET1(handle, ret);
		}
		break;
	}
	case RK_SIP_SDMMC_CLOCK_RATE_SET: {
		ret = rk_sip_sdmmc_clock_rate_set(x1, x2, x3);
		SMC_RET1(handle, ret);
		break;
	}
	case RK_SIP_SDMMC_CLOCK_PHASE_GET: {
		unsigned int phase_degrees = 0;

		ret = rk_sip_sdmmc_clock_phase_get(x1, x2, &phase_degrees);
		if (!ret) {
			SMC_RET2(handle, RK_SIP_E_SUCCESS, phase_degrees);
		} else {
			SMC_RET1(handle, ret);
		}
		break;
	}
	case RK_SIP_SDMMC_CLOCK_PHASE_SET: {
		ret = rk_sip_sdmmc_clock_phase_set(x1, x2, x3);
		SMC_RET1(handle, ret);
		break;
	}
	case RK_SIP_SDMMC_REGULATOR_VOLTAGE_GET: {
		unsigned int microvolts = 0;

		ret = rk_sip_sdmmc_regulator_voltage_get(x1, x2, &microvolts);
		if (!ret) {
			SMC_RET2(handle, RK_SIP_E_SUCCESS, microvolts);
		} else {
			SMC_RET1(handle, ret);
		}
		break;
	}
	case RK_SIP_SDMMC_REGULATOR_VOLTAGE_SET: {
		ret = rk_sip_sdmmc_regulator_voltage_set(x1, x2, x3);
		SMC_RET1(handle, ret);
		break;
	}
	case RK_SIP_SDMMC_REGULATOR_ENABLE_GET: {
		bool enable = false;

		ret = rk_sip_sdmmc_regulator_enable_get(x1, x2, &enable);
		if (!ret) {
			SMC_RET2(handle, RK_SIP_E_SUCCESS, enable);
		} else {
			SMC_RET1(handle, ret);
		}
		break;
	}
	case RK_SIP_SDMMC_REGULATOR_ENABLE_SET: {
		ret = rk_sip_sdmmc_regulator_enable_set(x1, x2, x3);
		SMC_RET1(handle, ret);
		break;
	}
	default:
		ERROR("%s: unhandled SMC (0x%x)\n", __func__, smc_fid);
		SMC_RET1(handle, SMC_UNK);
	}

	SMC_RET1(handle, SMC_UNK);
}
