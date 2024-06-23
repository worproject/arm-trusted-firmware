/*
 * Copyright (c) 2024, Mario Bălănică <mariobalanica02@gmail.com>
 * Copyright (c) 2024, Rockchip, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>
#include <stdint.h>

#include <drivers/delay_timer.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>
#include <platform_def.h>

#include <lib/smccc.h>
#include <services/trng_svc.h>
#include <smccc_helpers.h>

#include <plat/common/platform.h>

#define TRNG_CTRL			0x0000
#define TRNG_CTRL_CMD_NOP		(0 << 0)
#define TRNG_CTRL_CMD_RAND		(1 << 0)
#define TRNG_CTRL_CMD_SEED		(2 << 0)

#define TRNG_STAT			0x0004
#define TRNG_STAT_SEEDED		BIT(9)
#define TRNG_STAT_GENERATING		BIT(30)
#define TRNG_STAT_RESEEDING		BIT(31)

#define TRNG_MODE			0x0008
#define TRNG_MODE_128_BIT		(0 << 3)
#define TRNG_MODE_256_BIT		(1 << 3)

#define TRNG_IE				0x0010
#define TRNG_IE_GLBL_EN			BIT(31)
#define TRNG_IE_SEED_DONE_EN		BIT(1)
#define TRNG_IE_RAND_RDY_EN		BIT(0)

#define TRNG_ISTAT			0x0014
#define TRNG_ISTAT_RAND_RDY		BIT(0)

#define TRNG_RAND(x)			(0x0020 + (x) * 4)
#define TRNG_RAND_CNT			8

#define TRNG_AUTO_RQSTS			0x0060

#define ROCKCHIP_POLL_TIMEOUT_US	50000

DEFINE_SVC_UUID2(_plat_trng_uuid,
	0x23523c58, 0x7448, 0x4083, 0x9d, 0x16,
	0xe3, 0xfa, 0xb9, 0xf1, 0x73, 0xbc
);
uuid_t plat_trng_uuid;

bool plat_get_entropy(uint64_t *out)
{
	bool success;
	uint32_t reg;
	unsigned int retry;

	success = false;

	/* Clear interrupt status */
	reg = mmio_read_32(TRNG_BASE + TRNG_ISTAT);
	mmio_write_32(TRNG_BASE + TRNG_ISTAT, reg);

	/* Generate 128-bit random data */
	mmio_write_32(TRNG_BASE + TRNG_MODE, TRNG_MODE_128_BIT);
	mmio_write_32(TRNG_BASE + TRNG_CTRL, TRNG_CTRL_CMD_RAND);

	/* Wait for data ready */
	for (retry = ROCKCHIP_POLL_TIMEOUT_US; retry > 0; retry--) {
		reg = mmio_read_32(TRNG_BASE + TRNG_ISTAT);
		if (reg & TRNG_ISTAT_RAND_RDY) {
			success = true;
			break;
		}
		udelay(1);
	}

	if (retry == 0) {
		goto done;
	}

	/* Grab 64-bit random data */
	*out = mmio_read_64(TRNG_BASE + TRNG_RAND(0));

	/* Clear interrupt status */
	mmio_write_32(TRNG_BASE + TRNG_ISTAT, reg);

done:
	/* Close TRNG */
	mmio_write_32(TRNG_BASE + TRNG_CTRL, TRNG_CTRL_CMD_NOP);

	return success;
}

void plat_entropy_setup(void)
{
	uint32_t reg;
	unsigned int retry;

	plat_trng_uuid = _plat_trng_uuid;

	/* Ensure TRNG is ready */
	const uint32_t mask = TRNG_STAT_SEEDED |
			      TRNG_STAT_GENERATING |
			      TRNG_STAT_RESEEDING;

	for (retry = ROCKCHIP_POLL_TIMEOUT_US; retry > 0; retry--) {
		reg = mmio_read_32(TRNG_BASE + TRNG_STAT);
		if ((reg & mask) == TRNG_STAT_SEEDED) {
			break;
		}
		udelay(1);
	}

	/* Clear interrupt status */
	reg = mmio_read_32(TRNG_BASE + TRNG_ISTAT);
	mmio_write_32(TRNG_BASE + TRNG_ISTAT, reg);

	/* Auto reseed after (1000 * 16) bytes of random data generated */
	mmio_write_32(TRNG_BASE + TRNG_AUTO_RQSTS, 1000);
}
