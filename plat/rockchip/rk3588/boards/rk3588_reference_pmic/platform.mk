#
# Copyright (c) 2024, Mario Bălănică <mariobalanica02@gmail.com>
#
# SPDX-License-Identifier: BSD-3-Clause
#

BOARD				:= ${PLAT}
RK_PLAT_BOARD 			:= plat/rockchip/rk3588/boards/${BOARD}

BL31_SOURCES		+=	drivers/scmi-msg/voltage_domain.c		\
				${RK_PLAT_COMMON}/drivers/spi/spi.c		\
				${RK_PLAT_COMMON}/drivers/rk806/rk806.c		\
				${RK_PLAT_COMMON}/scmi/scmi_voltd.c		\
				${RK_PLAT_BOARD}/scmi/rk3588_voltd.c		\
				${RK_PLAT_BOARD}/board.c

PLAT_INCLUDES		+=	-I${RK_PLAT_BOARD}/

$(eval $(call add_define_val,BOARD,'"${BOARD}"'))

include plat/rockchip/rk3588/platform.mk
