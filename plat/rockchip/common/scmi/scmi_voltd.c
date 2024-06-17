/*
 * Copyright (c) 2024, Rockchip, Inc. All rights reserved.
 * Copyright (c) 2024, Mario Bălănică <mariobalanica02@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <drivers/scmi-msg.h>
#include <drivers/scmi.h>

#include "scmi_voltd.h"

#pragma weak rockchip_scmi_voltd_count
#pragma weak rockchip_scmi_get_voltd

size_t rockchip_scmi_voltd_count(unsigned int agent_id __unused)
{
	return 0;
}

rk_scmi_voltd_t *rockchip_scmi_get_voltd(uint32_t agent_id __unused,
					 uint32_t scmi_id __unused)
{
	return NULL;
}

size_t plat_scmi_voltd_count(unsigned int agent_id)
{
	return rockchip_scmi_voltd_count(agent_id);
}

const char *plat_scmi_voltd_get_name(unsigned int agent_id,
				     unsigned int scmi_id)
{
	rk_scmi_voltd_t *voltd;

	voltd = rockchip_scmi_get_voltd(agent_id, scmi_id);
	if (voltd == NULL)
		return NULL;

	return voltd->name;
}

int32_t plat_scmi_voltd_levels_array(unsigned int agent_id,
				     unsigned int scmi_id,
				     int *levels,
				     size_t *nb_elts,
				     uint32_t start_idx)
{
	return SCMI_NOT_SUPPORTED;
}

int32_t plat_scmi_voltd_levels_by_step(unsigned int agent_id __unused,
				       unsigned int scmi_id __unused,
				       int *steps __unused)
{
	rk_scmi_voltd_t *voltd;

	voltd = rockchip_scmi_get_voltd(agent_id, scmi_id);
	if (voltd == NULL)
		return SCMI_NOT_FOUND;

	steps[0] = voltd->min_level;
	steps[1] = voltd->max_level;
	steps[2] = voltd->step_level;

	return SCMI_SUCCESS;
}

int32_t plat_scmi_voltd_get_level(unsigned int agent_id,
				  unsigned int scmi_id,
				  int *level)
{
	rk_scmi_voltd_t *voltd;

	voltd = rockchip_scmi_get_voltd(agent_id, scmi_id);
	if (voltd == NULL)
		return SCMI_NOT_FOUND;

	if (voltd->voltd_ops && voltd->voltd_ops->get_level)
		*level = voltd->voltd_ops->get_level(voltd);

	/* return cur_level if no get_level ops or get_level return 0 */
	if (*level == 0)
		*level = voltd->cur_level;

	return SCMI_SUCCESS;
}

int32_t plat_scmi_voltd_set_level(unsigned int agent_id,
				  unsigned int scmi_id,
				  int level)
{
	rk_scmi_voltd_t *voltd;
	int32_t status = 0;

	voltd = rockchip_scmi_get_voltd(agent_id, scmi_id);
	if (voltd == NULL)
		return SCMI_NOT_FOUND;

	if (voltd->voltd_ops && voltd->voltd_ops->set_level) {
		if ((level >= voltd->min_level) &&
		    (level <= voltd->max_level)) {
			status = voltd->voltd_ops->set_level(voltd, level);
		} else {
			status = SCMI_INVALID_PARAMETERS;
		}

		if (status == SCMI_SUCCESS)
			voltd->cur_level = level;
	} else {
		status = SCMI_NOT_SUPPORTED;
	}

	return status;
}
