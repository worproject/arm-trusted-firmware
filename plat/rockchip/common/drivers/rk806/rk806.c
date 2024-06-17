/*
 * Copyright (c) 2020-2021, Rockchip, Inc. All rights reserved.
 * Copyright (c) 2024, Mario Bălănică <mariobalanica02@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>

#include <common/debug.h>
#include <drivers/delay_timer.h>

#include <rockchip_spi.h>

#include "rk806.h"

static const struct rk8xx_reg_info rk806_buck[] = {
	/* buck 1 */
	{  500000,   6250, RK806_BUCK_ON_VSEL(1), RK806_BUCK_SLP_VSEL(1), RK806_BUCK_CONFIG(1), RK806_BUCK_VSEL_MASK, 0x00, 0xa0, 3},
	{  1500000, 25000, RK806_BUCK_ON_VSEL(1), RK806_BUCK_SLP_VSEL(1), RK806_BUCK_CONFIG(1), RK806_BUCK_VSEL_MASK, 0xa1, 0xed, 3},
	{  3400000,     0, RK806_BUCK_ON_VSEL(1), RK806_BUCK_SLP_VSEL(1), RK806_BUCK_CONFIG(1), RK806_BUCK_VSEL_MASK, 0xee, 0xff, 3},
	/* buck 2 */
	{  500000,   6250, RK806_BUCK_ON_VSEL(2), RK806_BUCK_SLP_VSEL(2), RK806_BUCK_CONFIG(2), RK806_BUCK_VSEL_MASK, 0x00, 0xa0, 3},
	{  1500000, 25000, RK806_BUCK_ON_VSEL(2), RK806_BUCK_SLP_VSEL(2), RK806_BUCK_CONFIG(2), RK806_BUCK_VSEL_MASK, 0xa1, 0xed, 3},
	{  3400000,     0, RK806_BUCK_ON_VSEL(2), RK806_BUCK_SLP_VSEL(2), RK806_BUCK_CONFIG(2), RK806_BUCK_VSEL_MASK, 0xee, 0xff, 3},
	/* buck 3 */
	{  500000,   6250, RK806_BUCK_ON_VSEL(3), RK806_BUCK_SLP_VSEL(3), RK806_BUCK_CONFIG(3), RK806_BUCK_VSEL_MASK, 0x00, 0xa0, 3},
	{  1500000, 25000, RK806_BUCK_ON_VSEL(3), RK806_BUCK_SLP_VSEL(3), RK806_BUCK_CONFIG(3), RK806_BUCK_VSEL_MASK, 0xa1, 0xed, 3},
	{  3400000,     0, RK806_BUCK_ON_VSEL(3), RK806_BUCK_SLP_VSEL(3), RK806_BUCK_CONFIG(3), RK806_BUCK_VSEL_MASK, 0xee, 0xff, 3},
	/* buck 4 */
	{  500000,   6250, RK806_BUCK_ON_VSEL(4), RK806_BUCK_SLP_VSEL(4), RK806_BUCK_CONFIG(4), RK806_BUCK_VSEL_MASK, 0x00, 0xa0, 3},
	{  1500000, 25000, RK806_BUCK_ON_VSEL(4), RK806_BUCK_SLP_VSEL(4), RK806_BUCK_CONFIG(4), RK806_BUCK_VSEL_MASK, 0xa1, 0xed, 3},
	{  3400000,     0, RK806_BUCK_ON_VSEL(4), RK806_BUCK_SLP_VSEL(4), RK806_BUCK_CONFIG(4), RK806_BUCK_VSEL_MASK, 0xee, 0xff, 3},
	/* buck 5 */
	{  500000,   6250, RK806_BUCK_ON_VSEL(5), RK806_BUCK_SLP_VSEL(5), RK806_BUCK_CONFIG(5), RK806_BUCK_VSEL_MASK, 0x00, 0xa0, 3},
	{  1500000, 25000, RK806_BUCK_ON_VSEL(5), RK806_BUCK_SLP_VSEL(5), RK806_BUCK_CONFIG(5), RK806_BUCK_VSEL_MASK, 0xa1, 0xed, 3},
	{  3400000,     0, RK806_BUCK_ON_VSEL(5), RK806_BUCK_SLP_VSEL(5), RK806_BUCK_CONFIG(5), RK806_BUCK_VSEL_MASK, 0xee, 0xff, 3},
	/* buck 6 */
	{  500000,   6250, RK806_BUCK_ON_VSEL(6), RK806_BUCK_SLP_VSEL(6), RK806_BUCK_CONFIG(6), RK806_BUCK_VSEL_MASK, 0x00, 0xa0, 3},
	{  1500000, 25000, RK806_BUCK_ON_VSEL(6), RK806_BUCK_SLP_VSEL(6), RK806_BUCK_CONFIG(6), RK806_BUCK_VSEL_MASK, 0xa1, 0xed, 3},
	{  3400000,     0, RK806_BUCK_ON_VSEL(6), RK806_BUCK_SLP_VSEL(6), RK806_BUCK_CONFIG(6), RK806_BUCK_VSEL_MASK, 0xee, 0xff, 3},
	/* buck 7 */
	{  500000,   6250, RK806_BUCK_ON_VSEL(7), RK806_BUCK_SLP_VSEL(7), RK806_BUCK_CONFIG(7), RK806_BUCK_VSEL_MASK, 0x00, 0xa0, 3},
	{  1500000, 25000, RK806_BUCK_ON_VSEL(7), RK806_BUCK_SLP_VSEL(7), RK806_BUCK_CONFIG(7), RK806_BUCK_VSEL_MASK, 0xa1, 0xed, 3},
	{  3400000,     0, RK806_BUCK_ON_VSEL(7), RK806_BUCK_SLP_VSEL(7), RK806_BUCK_CONFIG(7), RK806_BUCK_VSEL_MASK, 0xee, 0xff, 3},
	/* buck 8 */
	{  500000,   6250, RK806_BUCK_ON_VSEL(8), RK806_BUCK_SLP_VSEL(8), RK806_BUCK_CONFIG(8), RK806_BUCK_VSEL_MASK, 0x00, 0xa0, 3},
	{  1500000, 25000, RK806_BUCK_ON_VSEL(8), RK806_BUCK_SLP_VSEL(8), RK806_BUCK_CONFIG(8), RK806_BUCK_VSEL_MASK, 0xa1, 0xed, 3},
	{  3400000,     0, RK806_BUCK_ON_VSEL(8), RK806_BUCK_SLP_VSEL(8), RK806_BUCK_CONFIG(8), RK806_BUCK_VSEL_MASK, 0xee, 0xff, 3},
	/* buck 9 */
	{  500000,   6250, RK806_BUCK_ON_VSEL(9), RK806_BUCK_SLP_VSEL(9), RK806_BUCK_CONFIG(9), RK806_BUCK_VSEL_MASK, 0x00, 0xa0, 3},
	{  1500000, 25000, RK806_BUCK_ON_VSEL(9), RK806_BUCK_SLP_VSEL(9), RK806_BUCK_CONFIG(9), RK806_BUCK_VSEL_MASK, 0xa1, 0xed, 3},
	{  3400000,     0, RK806_BUCK_ON_VSEL(9), RK806_BUCK_SLP_VSEL(9), RK806_BUCK_CONFIG(9), RK806_BUCK_VSEL_MASK, 0xee, 0xff, 3},
	/* buck 10 */
	{  500000,   6250, RK806_BUCK_ON_VSEL(10), RK806_BUCK_SLP_VSEL(10), RK806_BUCK_CONFIG(10), RK806_BUCK_VSEL_MASK, 0x00, 0xa0, 3},
	{  1500000, 25000, RK806_BUCK_ON_VSEL(10), RK806_BUCK_SLP_VSEL(10), RK806_BUCK_CONFIG(10), RK806_BUCK_VSEL_MASK, 0xa1, 0xed, 3},
	{  3400000,     0, RK806_BUCK_ON_VSEL(10), RK806_BUCK_SLP_VSEL(10), RK806_BUCK_CONFIG(10), RK806_BUCK_VSEL_MASK, 0xee, 0xff, 3},
};

static const struct rk8xx_reg_info rk806_nldo[] = {
	/* nldo1 */
	{  500000, 12500, RK806_NLDO_ON_VSEL(1), RK806_NLDO_SLP_VSEL(1), NA, RK806_NLDO_VSEL_MASK, 0x00, },
	{  3400000,    0, RK806_NLDO_ON_VSEL(1), RK806_NLDO_SLP_VSEL(1), NA, RK806_NLDO_VSEL_MASK, 0xE8, },
	/* nldo2 */
	{  500000, 12500, RK806_NLDO_ON_VSEL(2), RK806_NLDO_SLP_VSEL(2), NA, RK806_NLDO_VSEL_MASK, 0x00, },
	{  3400000,    0, RK806_NLDO_ON_VSEL(2), RK806_NLDO_SLP_VSEL(2), NA, RK806_NLDO_VSEL_MASK, 0xE8, },
	/* nldo3 */
	{  500000, 12500, RK806_NLDO_ON_VSEL(3), RK806_NLDO_SLP_VSEL(3), NA, RK806_NLDO_VSEL_MASK, 0x00, },
	{  3400000,    0, RK806_NLDO_ON_VSEL(3), RK806_NLDO_SLP_VSEL(3), NA, RK806_NLDO_VSEL_MASK, 0xE8, },
	/* nldo4 */
	{  500000, 12500, RK806_NLDO_ON_VSEL(4), RK806_NLDO_SLP_VSEL(4), NA, RK806_NLDO_VSEL_MASK, 0x00, },
	{  3400000,    0, RK806_NLDO_ON_VSEL(4), RK806_NLDO_SLP_VSEL(4), NA, RK806_NLDO_VSEL_MASK, 0xE8, },
	/* nldo5 */
	{  500000, 12500, RK806_NLDO_ON_VSEL(5), RK806_NLDO_SLP_VSEL(5), NA, RK806_NLDO_VSEL_MASK, 0x00, },
	{  3400000,    0, RK806_NLDO_ON_VSEL(5), RK806_NLDO_SLP_VSEL(5), NA, RK806_NLDO_VSEL_MASK, 0xE8, },
};

static const struct rk8xx_reg_info rk806_pldo[] = {
	/* pldo1 */
	{  500000, 12500, RK806_PLDO_ON_VSEL(1), RK806_PLDO_SLP_VSEL(1), NA, RK806_PLDO_VSEL_MASK, 0x00, },
	{  3400000,    0, RK806_PLDO_ON_VSEL(1), RK806_PLDO_SLP_VSEL(1), NA, RK806_PLDO_VSEL_MASK, 0xE8, },
	/* pldo2 */
	{  500000, 12500, RK806_PLDO_ON_VSEL(2), RK806_PLDO_SLP_VSEL(2), NA, RK806_PLDO_VSEL_MASK, 0x00, },
	{  3400000,    0, RK806_PLDO_ON_VSEL(2), RK806_PLDO_SLP_VSEL(2), NA, RK806_PLDO_VSEL_MASK, 0xE8, },
	/* pldo3 */
	{  500000, 12500, RK806_PLDO_ON_VSEL(3), RK806_PLDO_SLP_VSEL(3), NA, RK806_PLDO_VSEL_MASK, 0x00, },
	{  3400000,    0, RK806_PLDO_ON_VSEL(3), RK806_PLDO_SLP_VSEL(3), NA, RK806_PLDO_VSEL_MASK, 0xE8, },
	/* pldo4 */
	{  500000, 12500, RK806_PLDO_ON_VSEL(4), RK806_PLDO_SLP_VSEL(4), NA, RK806_PLDO_VSEL_MASK, 0x00, },
	{  3400000,    0, RK806_PLDO_ON_VSEL(4), RK806_PLDO_SLP_VSEL(4), NA, RK806_PLDO_VSEL_MASK, 0xE8, },
	/* pldo5 */
	{  500000, 12500, RK806_PLDO_ON_VSEL(5), RK806_PLDO_SLP_VSEL(5), NA, RK806_PLDO_VSEL_MASK, 0x00, },
	{  3400000,    0, RK806_PLDO_ON_VSEL(5), RK806_PLDO_SLP_VSEL(5), NA, RK806_PLDO_VSEL_MASK, 0xE8, },
	/* pldo6 */
	{  500000, 12500, RK806_PLDO_ON_VSEL(6), RK806_PLDO_SLP_VSEL(6), NA, RK806_PLDO_VSEL_MASK, 0x00, },
	{  3400000,    0, RK806_PLDO_ON_VSEL(6), RK806_PLDO_SLP_VSEL(6), NA, RK806_PLDO_VSEL_MASK, 0xE8, },
};

static int pmic_spi_read(struct rk806_regulator *dev, uint32_t cs_id,
			 uint32_t reg, uint8_t *val)
{
	uint8_t txbuf[8];
	int ret;

	txbuf[0] = RK806_CMD_READ;
	txbuf[1] = reg;
	txbuf[2] = RK806_REG_H;

	rk_spi_set_cs(dev->spi, cs_id, true);

	rk_spi_configure(dev->spi, txbuf, NULL, 3);
	ret = rk_spi_transfer(dev->spi);
	if (ret != 0) {
		goto exit;
	}
	rk_spi_stop(dev->spi);

	rk_spi_configure(dev->spi, NULL, val, 1);
	ret = rk_spi_transfer(dev->spi);

exit:
	rk_spi_stop(dev->spi);
	rk_spi_set_cs(dev->spi, cs_id, false);

	return ret;
}

static int pmic_spi_write(struct rk806_regulator *dev, uint32_t cs_id,
			  uint32_t reg, const uint8_t *val)
{
	uint8_t txbuf[4];
	int ret;

	txbuf[0] = RK806_CMD_WRITE;
	txbuf[1] = reg;
	txbuf[2] = RK806_REG_H;
	txbuf[3] = *val;

	rk_spi_set_cs(dev->spi, cs_id, true);

	rk_spi_configure(dev->spi, txbuf, NULL, 4);
	ret = rk_spi_transfer(dev->spi);

	rk_spi_stop(dev->spi);
	rk_spi_set_cs(dev->spi, cs_id, false);

	return ret;
}

static int pmic_reg_read(struct rk806_regulator *dev, uint32_t cs_id,
			 uint32_t reg, uint8_t *buffer)
{
	int ret;

	ret = pmic_spi_read(dev, cs_id, reg, buffer);
	if (ret)
		ERROR("%s: cs_id=%d reg(0x%x) buffer=%x ret=%d\n",
		      __func__, cs_id, reg, buffer[0], ret);

	return ret;
}

static int pmic_reg_write(struct rk806_regulator *dev, uint32_t cs_id,
			      uint32_t reg, const uint8_t *buffer)
{
	int ret;

	ret = pmic_spi_write(dev, cs_id, reg, buffer);
	if (ret)
		ERROR("%s: cs_id=%d reg(0x%x) buffer=%x ret=%d\n",
		      __func__, cs_id, reg, buffer[0], ret);

	return ret;
}

static int pmic_clrsetbits(struct rk806_regulator *dev, uint32_t cs_id,
			   uint32_t reg, uint32_t clr, uint32_t set)
{
	uint8_t byte;
	int ret;

	ret = pmic_reg_read(dev, cs_id, reg, &byte);
	if (ret < 0)
		return ret;
	byte = (ret & ~clr) | set;

	pmic_reg_write(dev, cs_id, reg, &byte);
	ret = pmic_reg_read(dev, cs_id, reg, &byte);
	return ret;
}

static const struct rk8xx_reg_info *get_buck_reg(uint32_t num, uint32_t uvolt)
{
	if (uvolt < 1500000)
		return &rk806_buck[num * 3 + 0];
	else if (uvolt < 3400000)
		return &rk806_buck[num * 3 + 1];
	else
		return &rk806_buck[num * 3 + 2];
}

static const struct rk8xx_reg_info *get_nldo_reg(uint32_t num, uint32_t uvolt)
{
	if (uvolt < 3400000)
		return &rk806_nldo[num * 2];
	else
		return &rk806_nldo[num * 2 + 1];
}

static const struct rk8xx_reg_info *get_pldo_reg(uint32_t num, uint32_t uvolt)
{
	if (uvolt < 3400000)
		return &rk806_pldo[num * 2];
	else
		return &rk806_pldo[num * 2 + 1];
}

static inline const char *reg_type_to_string(uint32_t reg_id)
{
	char *str;

	switch (reg_id & REGULATOR_TYPE_MASK) {
	case BUCK:
		str = "buck";
		break;
	case NLDO:
		str = "nldo";
		break;
	case PLDO:
		str = "pldo";
		break;
	default:
		assert(false);
		str = "unknown";
		break;
	}
	return str;
}

static int common_set_voltage(struct rk806_regulator *dev, uint32_t reg_id, uint32_t uvolt,
			      const struct rk8xx_reg_info *info)
{
	uint32_t cs_id = (reg_id & CHIP_SELECT_MASK) >> CHIP_SELECT_SHIFT;
	uint32_t num = (reg_id & REGULATOR_ID_MASK) >> REGULATOR_ID_SHIFT;
	uint32_t value;

	if (info->vsel_reg == NA)
		return -EINVAL;

	if (info->step_uv == 0)	/* Fixed voltage */
		value = info->min_sel;
	else
		value = ((uvolt - info->min_uv) / info->step_uv) + info->min_sel;

	INFO("%s: cs_id=%d, %s=%d, uvolt=%d, reg=0x%x, mask=0x%x, val=0x%x\n",
	      __func__, cs_id, reg_type_to_string(reg_id), num + 1, uvolt,
	      info->vsel_reg, info->vsel_mask, value);

	return pmic_clrsetbits(dev, cs_id, info->vsel_reg, info->vsel_mask, value);
}

static int buck_set_enable(struct rk806_regulator *dev, uint32_t reg_id, bool enable)
{
	uint32_t cs_id = (reg_id & CHIP_SELECT_MASK) >> CHIP_SELECT_SHIFT;
	uint32_t buck = (reg_id & REGULATOR_ID_MASK) >> REGULATOR_ID_SHIFT;
	uint8_t value, en_reg;
	int ret = 0;

	en_reg = RK806_POWER_EN(buck / 4);
	if (enable)
		value = ((1 << buck % 4) | (1 << (buck % 4 + 4)));
	else
		value = ((0 << buck % 4) | (1 << (buck % 4 + 4)));

	ret = pmic_reg_write(dev, cs_id, en_reg, &value);

	return ret;
}

static int nldo_set_enable(struct rk806_regulator *dev, uint32_t reg_id, bool enable)
{
	uint32_t cs_id = (reg_id & CHIP_SELECT_MASK) >> CHIP_SELECT_SHIFT;
	uint32_t ldo = (reg_id & REGULATOR_ID_MASK) >> REGULATOR_ID_SHIFT;
	uint8_t value, en_reg;
	int ret = 0;

	if (ldo < 4) {
		en_reg = RK806_NLDO_EN(0);
		if (enable)
			value = ((1 << ldo % 4) | (1 << (ldo % 4 + 4)));
		else
			value = ((0 << ldo % 4) | (1 << (ldo % 4 + 4)));
		ret = pmic_reg_write(dev, cs_id, en_reg, &value);
	} else {
		en_reg = RK806_NLDO_EN(2);
		if (enable)
			value = 0x44;
		else
			value = 0x40;
		ret = pmic_reg_write(dev, cs_id, en_reg, &value);
	}

	return ret;
}

static int pldo_set_enable(struct rk806_regulator *dev, uint32_t reg_id, bool enable)
{
	uint32_t cs_id = (reg_id & CHIP_SELECT_MASK) >> CHIP_SELECT_SHIFT;
	uint32_t pldo = (reg_id & REGULATOR_ID_MASK) >> REGULATOR_ID_SHIFT;
	uint8_t value, en_reg;
	int ret = 0;

	if (pldo < 3) {
		en_reg = RK806_PLDO_EN(0);
		if (enable)
			value = RK806_PLDO0_2_SET(pldo);
		else
			value = RK806_PLDO0_2_CLR(pldo);
		ret = pmic_reg_write(dev, cs_id, en_reg, &value);
	} else if (pldo == 3) {
		en_reg = RK806_PLDO_EN(1);
		if (enable) {
			value = ((1 << 0) | (1 << 4));
		} else {
			value = (1 << 4);
		}
		ret = pmic_reg_write(dev, cs_id, en_reg, &value);
	} else if (pldo == 4) {
		en_reg = RK806_PLDO_EN(1);
		if (enable)
			value = ((1 << 1) | (1 << 5));
		else
			value = ((0 << 1) | (1 << 5));
		ret = pmic_reg_write(dev, cs_id, en_reg, &value);
	} else if (pldo == 5) {
		en_reg = RK806_PLDO_EN(0);
		if (enable)
			value = ((1 << 0) | (1 << 4));
		else
			value = ((0 << 0) | (1 << 4));
		ret = pmic_reg_write(dev, cs_id, en_reg, &value);
	} else {
		ret = -EINVAL;
	}

	return ret;
}

int rk806_regulator_set_voltage(struct rk806_regulator *dev, uint32_t reg_id, uint32_t uvolt)
{
	uint32_t num = (reg_id & REGULATOR_ID_MASK) >> REGULATOR_ID_SHIFT;
	int ret = 0;

	switch (reg_id & REGULATOR_TYPE_MASK) {
	case BUCK:
		ret = common_set_voltage(dev, reg_id, uvolt, get_buck_reg(num, uvolt));
		break;
	case NLDO:
		ret = common_set_voltage(dev, reg_id, uvolt, get_nldo_reg(num, uvolt));
		break;
	case PLDO:
		ret = common_set_voltage(dev, reg_id, uvolt, get_pldo_reg(num, uvolt));
		break;
	default:
		assert(false);
		break;
	}

	return ret;
}

int rk806_regulator_set_state(struct rk806_regulator *dev, uint32_t reg_id, bool enable)
{
	int ret = 0;

	switch (reg_id & REGULATOR_TYPE_MASK) {
	case BUCK:
		ret = buck_set_enable(dev, reg_id, enable);
		break;
	case NLDO:
		ret = nldo_set_enable(dev, reg_id, enable);
		break;
	case PLDO:
		ret = pldo_set_enable(dev, reg_id, enable);
		break;
	default:
		assert(false);
		break;
	}

	return ret;
}

static void rk806_spi_config(struct rk806_regulator *dev)
{
	struct rk_spi_controller *spi = dev->spi;

	/* Data width */
	spi->config.num_bytes = RK_SPI_CFG_DATA_FRAME_SIZE_8BIT;

	/* CPOL */
	spi->config.clk_polarity = RK_SPI_CFG_POLARITY_LOW;

	/* CPHA */
	spi->config.clk_phase = RK_SPI_CFG_PHASE_1EDGE;

	/* MSB or LSB */
	spi->config.first_bit = RK_SPI_CFG_FIRSTBIT_MSB;

	/* Master or Slave */
	spi->config.op_mode = RK_SPI_CFG_OPM_MASTER;

	/* CSM cycles */
	spi->config.csm = RK_SPI_CFG_CSM_0CYCLE;

	spi->config.speed = 2000000;

	if (spi->config.speed > SPI_MASTER_MAX_SCLK_OUT) {
		spi->config.speed = SPI_MASTER_MAX_SCLK_OUT;
	}
}

int rk806_init(struct rk806_regulator *dev, struct rk_spi_controller *spi)
{
	dev->spi = spi;

	rk806_spi_config(dev);

	return 0;
}
