/*
 * Copyright (c) 2024, Rockchip, Inc. All rights reserved.
 * Copyright (c) 2024, Mario Bălănică <mariobalanica02@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef RK_SCMI_VOLTD_H
#define RK_SCMI_VOLTD_H

#include <stdint.h>

#include <common.h>

struct rk_scmi_voltd;

struct rk_scmi_voltd_ops {
	int (*get_level)(struct rk_scmi_voltd *voltd);
	int32_t (*set_level)(struct rk_scmi_voltd *voltd, int level);
};

typedef struct rk_scmi_voltd {
	char name[SCMI_VOLTAGE_DOMAIN_NAME_LENGTH_MAX];
	uint32_t id;
	int32_t min_level;
	int32_t max_level;
	int32_t step_level;
	int32_t cur_level;
	const struct rk_scmi_voltd_ops *voltd_ops;
} rk_scmi_voltd_t;

/*
 * Return number of voltage domains for an agent
 * @agent_id: SCMI agent ID
 * Return number of voltage domains
 */
size_t rockchip_scmi_voltd_count(unsigned int agent_id);

/*
 * Get rk_scmi_voltd_t point
 * @agent_id: SCMI agent ID
 * @scmi_id: SCMI voltd ID
 * Return a rk_scmi_voltd_t point
 */
rk_scmi_voltd_t *rockchip_scmi_get_voltd(uint32_t agent_id,
					 uint32_t scmi_id);

#endif /* RK_SCMI_VOLTD_H */
