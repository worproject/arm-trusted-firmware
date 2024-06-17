// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 * Copyright (c) 2024, Mario Bălănică <mariobalanica02@gmail.com>
 */

#ifndef SCMI_MSG_VOLTAGE_DOMAIN_H
#define SCMI_MSG_VOLTAGE_DOMAIN_H

#include <stdint.h>

#include <lib/utils_def.h>

#define SCMI_PROTOCOL_VERSION_VOLTAGE_DOMAIN	0x10000U

/*
 * Identifiers of the SCMI Voltage domain management protocol commands
 */
enum scmi_voltd_command_id {
	SCMI_VOLTAGE_DOMAIN_ATTRIBUTES 		= 0x003,
	SCMI_VOLTAGE_DOMAIN_DESCRIBE_LEVELS 	= 0x004,
	SCMI_VOLTAGE_DOMAIN_CONFIG_SET 		= 0x005,
	SCMI_VOLTAGE_DOMAIN_CONFIG_GET 		= 0x006,
	SCMI_VOLTAGE_DOMAIN_LEVEL_SET 		= 0x007,
	SCMI_VOLTAGE_DOMAIN_LEVEL_GET 		= 0x008,
};

/*
 * VOLTAGE_DOMAIN_ATTRIBUTES
 */

struct scmi_voltd_attributes_a2p {
	uint32_t domain_id;
};

#define SCMI_VOLTAGE_DOMAIN_NAME_LENGTH_MAX	16U

struct scmi_voltd_attributes_p2a {
	int32_t status;
	uint32_t attributes;
	char name[SCMI_VOLTAGE_DOMAIN_NAME_LENGTH_MAX];
};


/*
 * VOLTAGE_DESCRIBE_LEVELS
 */

#define SCMI_VOLTAGE_LEVEL_FORMAT_RANGE			1U
#define SCMI_VOLTAGE_LEVEL_FORMAT_LIST			0U

#define SCMI_VOLTD_DESCRIBE_LEVELS_REMAINING_MASK	GENMASK_32(31, 16)
#define SCMI_VOLTD_DESCRIBE_LEVELS_REMAINING_POS	16

#define SCMI_VOLTD_DESCRIBE_LEVELS_FORMAT_MASK		BIT(12)
#define SCMI_VOLTD_DESCRIBE_LEVELS_FORMAT_POS		12

#define SCMI_VOLTD_DESCRIBE_LEVELS_COUNT_MASK		GENMASK_32(11, 0)

#define SCMI_VOLTD_DESCRIBE_LEVELS_FLAGS(_count, _fmt, _rem_levels) \
	( \
		((_count) & SCMI_VOLTD_DESCRIBE_LEVELS_COUNT_MASK) | \
		(((_rem_levels) << SCMI_VOLTD_DESCRIBE_LEVELS_REMAINING_POS) & \
		 SCMI_VOLTD_DESCRIBE_LEVELS_REMAINING_MASK) | \
		(((_fmt) << SCMI_VOLTD_DESCRIBE_LEVELS_FORMAT_POS) & \
		 SCMI_VOLTD_DESCRIBE_LEVELS_FORMAT_MASK) \
	)

struct scmi_voltd_describe_levels_a2p {
	uint32_t domain_id;
	uint32_t level_index;
};

struct scmi_voltd_describe_levels_p2a {
	int32_t status;
	uint32_t flags;
	int32_t voltage[];
};

/*
 * VOLTAGE_CONFIG
 */

#define SCMI_VOLTD_CONFIG_MODE_TYPE_IMPL	BIT(3)
#define SCMI_VOLTD_CONFIG_MODE_ID_MASK		GENMASK_32(2, 0)
#define SCMI_VOLTD_CONFIG_MODE_ID_ON 		0x7
#define SCMI_VOLTD_CONFIG_MODE_ID_OFF 		0x0

/*
 * VOLTAGE_CONFIG_SET
 */

struct scmi_voltd_config_set_a2p {
	uint32_t domain_id;
	uint32_t config;
};

struct scmi_voltd_config_set_p2a {
	int32_t status;
};

/*
 * VOLTAGE_CONFIG_GET
 */

struct scmi_voltd_config_get_a2p {
	uint32_t domain_id;
};

struct scmi_voltd_config_get_p2a {
	int32_t status;
	uint32_t config;
};

/*
 * VOLTAGE_LEVEL_SET
 */

struct scmi_voltd_level_set_a2p {
	uint32_t domain_id;
	uint32_t flags;
	int32_t voltage_level;
};

struct scmi_voltd_level_set_p2a {
	int32_t status;
};

/*
 * VOLTAGE_LEVEL_GET
 */

struct scmi_voltd_level_get_a2p {
	uint32_t domain_id;
};

struct scmi_voltd_level_get_p2a {
	int32_t status;
	int32_t voltage_level;
};

#endif /* SCMI_MSG_VOLTAGE_DOMAIN_H */
