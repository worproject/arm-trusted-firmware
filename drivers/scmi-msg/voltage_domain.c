// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 * Copyright (c) 2024, Mario Bălănică <mariobalanica02@gmail.com>
 */

#include <cdefs.h>
#include <string.h>

#include <drivers/scmi-msg.h>
#include <drivers/scmi.h>
#include <lib/utils_def.h>

#include "common.h"

#pragma weak plat_scmi_voltd_count
#pragma weak plat_scmi_voltd_get_name
#pragma weak plat_scmi_voltd_get_attributes
#pragma weak plat_scmi_voltd_levels_array
#pragma weak plat_scmi_voltd_levels_by_step
#pragma weak plat_scmi_voltd_set_state
#pragma weak plat_scmi_voltd_get_state
#pragma weak plat_scmi_voltd_set_state_custom
#pragma weak plat_scmi_voltd_get_state_custom
#pragma weak plat_scmi_voltd_set_level
#pragma weak plat_scmi_voltd_get_level

static bool message_id_is_supported(unsigned int message_id);

size_t plat_scmi_voltd_count(unsigned int agent_id __unused)
{
	return 0U;
}

const char *plat_scmi_voltd_get_name(unsigned int agent_id __unused,
				     unsigned int domain_id __unused)
{
	return NULL;
}

unsigned int plat_scmi_voltd_get_attributes(unsigned int agent_id __unused,
					    unsigned int domain_id __unused)
{
	return 0U;
}

int32_t plat_scmi_voltd_levels_array(unsigned int agent_id __unused,
				     unsigned int scmi_id __unused,
				     int *levels __unused,
				     size_t *nb_elts __unused,
				     uint32_t start_idx __unused)
{
	return SCMI_NOT_SUPPORTED;
}

int32_t plat_scmi_voltd_levels_by_step(unsigned int agent_id __unused,
				       unsigned int scmi_id __unused,
				       int *steps __unused)
{
	return SCMI_NOT_SUPPORTED;
}

int32_t plat_scmi_voltd_set_state(unsigned int agent_id __unused,
				  unsigned int domain_id __unused,
				  bool enable __unused)
{
	return SCMI_NOT_SUPPORTED;
}

int32_t plat_scmi_voltd_get_state(unsigned int agent_id __unused,
				  unsigned int domain_id __unused,
				  bool *enable __unused)
{
	return SCMI_NOT_SUPPORTED;
}

int32_t plat_scmi_voltd_set_state_custom(unsigned int agent_id __unused,
					 unsigned int domain_id __unused,
					 uint8_t state __unused)
{
	return SCMI_NOT_SUPPORTED;
}

int32_t plat_scmi_voltd_get_state_custom(unsigned int agent_id __unused,
					 unsigned int domain_id __unused,
					 uint8_t *state __unused)
{
	return SCMI_NOT_SUPPORTED;
}


int32_t plat_scmi_voltd_set_level(unsigned int agent_id __unused,
				  unsigned int domain_id __unused,
				  int level __unused)
{
	return SCMI_NOT_SUPPORTED;
}

int32_t plat_scmi_voltd_get_level(unsigned int agent_id __unused,
				  unsigned int domain_id __unused,
				  int *level __unused)
{
	return SCMI_NOT_SUPPORTED;
}

static void report_version(struct scmi_msg *msg)
{
	struct scmi_protocol_version_p2a return_values = {
		.status = SCMI_SUCCESS,
		.version = SCMI_PROTOCOL_VERSION_VOLTAGE_DOMAIN,
	};

	if (msg->in_size != 0) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	scmi_write_response(msg, &return_values, sizeof(return_values));
}

static void report_attributes(struct scmi_msg *msg)
{
	struct scmi_protocol_attributes_p2a return_values = {
		.status = SCMI_SUCCESS,
	};

	if (msg->in_size != 0) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	return_values.attributes = plat_scmi_voltd_count(msg->agent_id);

	scmi_write_response(msg, &return_values, sizeof(return_values));
}

static void report_message_attributes(struct scmi_msg *msg)
{
	struct scmi_protocol_message_attributes_a2p *in_args = (void *)msg->in;
	struct scmi_protocol_message_attributes_p2a return_values = {
		.status = SCMI_SUCCESS,
		/* For this protocol, attributes shall be zero */
		.attributes = 0U,
	};

	if (msg->in_size != sizeof(*in_args)) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	if (!message_id_is_supported(in_args->message_id)) {
		scmi_status_response(msg, SCMI_NOT_FOUND);
		return;
	}

	scmi_write_response(msg, &return_values, sizeof(return_values));
}

static void scmi_voltd_attributes(struct scmi_msg *msg)
{
	const struct scmi_voltd_attributes_a2p *in_args = (void *)msg->in;
	struct scmi_voltd_attributes_p2a return_values = {
		.status = SCMI_SUCCESS,
	};
	const char *name = NULL;
	unsigned int domain_id = 0U;

	if (msg->in_size != sizeof(*in_args)) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	domain_id = SPECULATION_SAFE_VALUE(in_args->domain_id);

	if (domain_id >= plat_scmi_voltd_count(msg->agent_id)) {
		scmi_status_response(msg, SCMI_INVALID_PARAMETERS);
		return;
	}

	name = plat_scmi_voltd_get_name(msg->agent_id, domain_id);
	if (name == NULL) {
		scmi_status_response(msg, SCMI_NOT_FOUND);
		return;
	}

	COPY_NAME_IDENTIFIER(return_values.name, name);

	return_values.attributes = plat_scmi_voltd_get_attributes(msg->agent_id, domain_id);

	scmi_write_response(msg, &return_values, sizeof(return_values));
}

#define LEVELS_ARRAY_SIZE_MAX	(SCMI_PLAYLOAD_MAX - \
				 sizeof(struct scmi_voltd_describe_levels_p2a))

#define SCMI_LEVELS_BY_ARRAY(_nb_levels, _rem_levels) \
	SCMI_VOLTD_DESCRIBE_LEVELS_FLAGS((_nb_levels), \
					 SCMI_VOLTAGE_LEVEL_FORMAT_LIST, \
					 (_rem_levels))
#define SCMI_LEVELS_BY_STEP \
	SCMI_VOLTD_DESCRIBE_LEVELS_FLAGS(3U, \
					 SCMI_VOLTAGE_LEVEL_FORMAT_RANGE, \
					 0U)

#define LEVEL_DESC_SIZE		sizeof(int32_t)

static void write_level_desc_array_in_buffer(char *dest, int *levels,
					    size_t nb_elt)
{
	int32_t *out = (int32_t *)(uintptr_t)dest;
	size_t n;

	ASSERT_SYM_PTR_ALIGN(out);

	for (n = 0U; n < nb_elt; n++) {
		out[n] = levels[n];
	}
}

static void scmi_voltd_describe_levels(struct scmi_msg *msg)
{
	const struct scmi_voltd_describe_levels_a2p *in_args = (void *)msg->in;
	struct scmi_voltd_describe_levels_p2a p2a = {
		.status = SCMI_SUCCESS,
	};
	size_t nb_levels;
	int32_t status;
	unsigned int domain_id;

	if (msg->in_size != sizeof(*in_args)) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	domain_id = SPECULATION_SAFE_VALUE(in_args->domain_id);

	if (domain_id >= plat_scmi_voltd_count(msg->agent_id)) {
		scmi_status_response(msg, SCMI_INVALID_PARAMETERS);
		return;
	}

	/* Platform may support array level description */
	status = plat_scmi_voltd_levels_array(msg->agent_id, domain_id, NULL,
					      &nb_levels, 0);
	if (status == SCMI_SUCCESS) {
		/* Currently 12 cells max, so it's affordable for the stack */
		int plat_levels[LEVELS_ARRAY_SIZE_MAX / LEVEL_DESC_SIZE];
		size_t max_nb = LEVELS_ARRAY_SIZE_MAX / LEVEL_DESC_SIZE;
		size_t ret_nb = MIN(nb_levels - in_args->level_index, max_nb);
		size_t rem_nb = nb_levels - in_args->level_index - ret_nb;

		status =  plat_scmi_voltd_levels_array(msg->agent_id, domain_id,
						       plat_levels, &ret_nb,
						       in_args->level_index);
		if (status == SCMI_SUCCESS) {
			write_level_desc_array_in_buffer(msg->out + sizeof(p2a),
							plat_levels, ret_nb);

			p2a.flags = SCMI_LEVELS_BY_ARRAY(ret_nb, rem_nb);
			p2a.status = SCMI_SUCCESS;

			memcpy(msg->out, &p2a, sizeof(p2a));
			msg->out_size_out = sizeof(p2a) +
					    ret_nb * LEVEL_DESC_SIZE;
		}
	} else if (status == SCMI_NOT_SUPPORTED) {
		int triplet[3] = { 0U, 0U, 0U };

		/* Platform may support min/max/step triplet description */
		status = plat_scmi_voltd_levels_by_step(msg->agent_id, domain_id,
							triplet);
		if (status == SCMI_SUCCESS) {
			write_level_desc_array_in_buffer(msg->out + sizeof(p2a),
							 triplet, 3U);

			p2a.flags = SCMI_LEVELS_BY_STEP;
			p2a.status = SCMI_SUCCESS;

			memcpy(msg->out, &p2a, sizeof(p2a));
			msg->out_size_out = sizeof(p2a) + (3U * LEVEL_DESC_SIZE);
		}
	} else {
		/* Fallthrough generic exit sequence below with error status */
	}

	if (status != SCMI_SUCCESS) {
		scmi_status_response(msg, status);
	} else {
		/*
		 * Message payload is already written to msg->out, and
		 * msg->out_size_out updated.
		 */
	}
}

static void scmi_voltd_config_set(struct scmi_msg *msg)
{
	const struct scmi_voltd_config_set_a2p *in_args = (void *)msg->in;
	int32_t status = 0;
	unsigned int domain_id = 0U;
	unsigned int config = 0U;
	uint8_t mode = 0U;

	if (msg->in_size != sizeof(*in_args)) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	domain_id = SPECULATION_SAFE_VALUE(in_args->domain_id);

	if (domain_id >= plat_scmi_voltd_count(msg->agent_id)) {
		scmi_status_response(msg, SCMI_INVALID_PARAMETERS);
		return;
	}

	config = SPECULATION_SAFE_VALUE(in_args->config);
	mode = config & SCMI_VOLTD_CONFIG_MODE_ID_MASK;

	if ((config & SCMI_VOLTD_CONFIG_MODE_TYPE_IMPL) == 0) {
		status = plat_scmi_voltd_set_state(msg->agent_id,
						   domain_id,
						   mode == SCMI_VOLTD_CONFIG_MODE_ID_ON);
	} else {
		status = plat_scmi_voltd_set_state_custom(msg->agent_id,
							  domain_id,
							  mode);
	}

	scmi_status_response(msg, status);
}

static void scmi_voltd_config_get(struct scmi_msg *msg)
{
	const struct scmi_voltd_config_get_a2p *in_args = (void *)msg->in;
	struct scmi_voltd_config_get_p2a return_values = {
		.status = SCMI_SUCCESS,
	};
	int32_t status = 0;
	unsigned int domain_id = 0U;
	bool enable = false;
	uint8_t mode = 0U;

	if (msg->in_size != sizeof(*in_args)) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	domain_id = SPECULATION_SAFE_VALUE(in_args->domain_id);

	if (domain_id >= plat_scmi_voltd_count(msg->agent_id)) {
		scmi_status_response(msg, SCMI_INVALID_PARAMETERS);
		return;
	}

	/* Platform may support architectural on/off description */
	status = plat_scmi_voltd_get_state(msg->agent_id,
					   domain_id,
					   &enable);
	if (status == SCMI_SUCCESS) {
		return_values.config = enable ? SCMI_VOLTD_CONFIG_MODE_ID_ON :
						SCMI_VOLTD_CONFIG_MODE_ID_OFF;
	} else if (status == SCMI_NOT_SUPPORTED) {
		/* Platform may support implementation-defined description */
		status = plat_scmi_voltd_get_state_custom(msg->agent_id,
							  domain_id,
							  &mode);
		return_values.config = SCMI_VOLTD_CONFIG_MODE_TYPE_IMPL |
				       (mode & SCMI_VOLTD_CONFIG_MODE_ID_MASK);
	} else {
		/* Fallthrough generic exit sequence below with error status */
	}

	if (status != SCMI_SUCCESS) {
		scmi_status_response(msg, status);
	}

	scmi_write_response(msg, &return_values, sizeof(return_values));
}

static void scmi_voltd_level_set(struct scmi_msg *msg)
{
	const struct scmi_voltd_level_set_a2p *in_args = (void *)msg->in;
	int32_t status = 0;
	unsigned int domain_id = 0U;
	int level = 0;

	if (msg->in_size != sizeof(*in_args)) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	domain_id = SPECULATION_SAFE_VALUE(in_args->domain_id);

	if (domain_id >= plat_scmi_voltd_count(msg->agent_id)) {
		scmi_status_response(msg, SCMI_INVALID_PARAMETERS);
		return;
	}

	level = SPECULATION_SAFE_VALUE(in_args->voltage_level);

	status = plat_scmi_voltd_set_level(msg->agent_id, domain_id, level);

	scmi_status_response(msg, status);
}

static void scmi_voltd_level_get(struct scmi_msg *msg)
{
	const struct scmi_voltd_level_get_a2p *in_args = (void *)msg->in;
	int32_t status = 0;
	struct scmi_voltd_level_get_p2a return_values = {
		.status = SCMI_SUCCESS,
	};
	unsigned int domain_id = 0U;

	if (msg->in_size != sizeof(*in_args)) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	domain_id = SPECULATION_SAFE_VALUE(in_args->domain_id);

	if (domain_id >= plat_scmi_voltd_count(msg->agent_id)) {
		scmi_status_response(msg, SCMI_INVALID_PARAMETERS);
		return;
	}

	status = plat_scmi_voltd_get_level(msg->agent_id, domain_id, &return_values.voltage_level);
	if (status == SCMI_SUCCESS) {
		scmi_write_response(msg, &return_values, sizeof(return_values));
	} else {
		scmi_status_response(msg, status);
	}
}

static const scmi_msg_handler_t scmi_voltd_handler_table[] = {
	[SCMI_PROTOCOL_VERSION] = report_version,
	[SCMI_PROTOCOL_ATTRIBUTES] = report_attributes,
	[SCMI_PROTOCOL_MESSAGE_ATTRIBUTES] = report_message_attributes,
	[SCMI_VOLTAGE_DOMAIN_ATTRIBUTES] = scmi_voltd_attributes,
	[SCMI_VOLTAGE_DOMAIN_DESCRIBE_LEVELS] = scmi_voltd_describe_levels,
	[SCMI_VOLTAGE_DOMAIN_CONFIG_SET] = scmi_voltd_config_set,
	[SCMI_VOLTAGE_DOMAIN_CONFIG_GET] = scmi_voltd_config_get,
	[SCMI_VOLTAGE_DOMAIN_LEVEL_SET] = scmi_voltd_level_set,
	[SCMI_VOLTAGE_DOMAIN_LEVEL_GET] = scmi_voltd_level_get,
};

static bool message_id_is_supported(unsigned int message_id)
{
	return (message_id < ARRAY_SIZE(scmi_voltd_handler_table)) &&
	       (scmi_voltd_handler_table[message_id] != NULL);
}

scmi_msg_handler_t scmi_msg_get_voltd_handler(struct scmi_msg *msg)
{
	const size_t array_size = ARRAY_SIZE(scmi_voltd_handler_table);
	unsigned int message_id = SPECULATION_SAFE_VALUE(msg->message_id);

	if (message_id >= array_size) {
		VERBOSE("Voltage domain handle not found %u", msg->message_id);
		return NULL;
	}

	return scmi_voltd_handler_table[message_id];
}
