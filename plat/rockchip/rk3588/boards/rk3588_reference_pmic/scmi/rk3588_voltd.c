/*
 * Copyright (c) 2024, Mario Bălănică <mariobalanica02@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>

#include <drivers/rk806/rk806.h>
#include <drivers/scmi.h>
#include <platform_def.h>
#include <rockchip_spi.h>
#include <scmi_voltd.h>

#include "rk3588_voltd.h"

static struct rk_spi_controller spi_instance;
static struct rk806_regulator rk806_instance;

#define RK3588_SCMI_VOLTD(_id, _name, _min, _max, _step, _ops)	\
	[_id] = {						\
		.id = _id,					\
		.name = _name,					\
		.min_level = _min,				\
		.max_level = _max,				\
		.step_level = _step,				\
		.voltd_ops = _ops,				\
	}

#define RK806_SCMI_REG(_scmi_id, _reg_id)	        	\
	[_scmi_id] = {						\
		.scmi_id = _scmi_id,				\
		.reg_id = _reg_id,				\
	}

typedef struct rk806_scmi_reg {
	uint32_t scmi_id;
	uint32_t reg_id;
} rk806_scmi_reg_t;

rk806_scmi_reg_t rk806_ref_table[] = {
	RK806_SCMI_REG(SCMI_VCCIO_SD_S0, MASTER_PLDO5),
};

static int32_t rk806_voltd_set_level(rk_scmi_voltd_t *voltd, int level)
{
	int ret;

	if (voltd->id >= ARRAY_SIZE(rk806_ref_table)) {
		assert(false);
		return SCMI_NOT_FOUND;
	}

	ret = rk806_regulator_set_voltage(&rk806_instance,
					  rk806_ref_table[voltd->id].reg_id,
					  level);
	if (ret) {
		return SCMI_HARDWARE_ERROR;
	}

	return SCMI_SUCCESS;
}

static struct rk_scmi_voltd_ops rk806_voltd_ops = {
	.set_level = rk806_voltd_set_level
};

static rk_scmi_voltd_t rk3588_voltd_table[] = {
	RK3588_SCMI_VOLTD(SCMI_VCCIO_SD_S0, "scmi_vccio_sd_s0", 1800000, 3300000, 12500, &rk806_voltd_ops),
};

rk_scmi_voltd_t *rockchip_scmi_get_voltd(unsigned int agent_id,
				       unsigned int scmi_id)
{
	return &rk3588_voltd_table[scmi_id];
}

size_t rockchip_scmi_voltd_count(unsigned int agent_id)
{
	return ARRAY_SIZE(rk3588_voltd_table);
}

void rk3588_reference_voltd_init(void)
{
	rk_spi_init(&spi_instance, SPI2_BASE, 24000000);
	rk806_init(&rk806_instance, &spi_instance);
}
