/*
 * Copyright (c) 2020-2021, Rockchip, Inc. All rights reserved.
 * Copyright (c) 2024, Mario Bălănică <mariobalanica02@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ROCKCHIP_SPI_API_H
#define ROCKCHIP_SPI_API_H

#define SPI_MASTER_MAX_SCLK_OUT		50000000 /**< Max io clock in master mode */
#define SPI_SLAVE_MAX_SCLK_OUT		20000000 /**< Max io in clock in slave mode */

#define RK_SPI_CFG_DATA_FRAME_SIZE_4BIT		0x00
#define RK_SPI_CFG_DATA_FRAME_SIZE_8BIT		0x01
#define RK_SPI_CFG_DATA_FRAME_SIZE_16BIT	0x02

/* serial clock toggles in middle of first data bit */
#define RK_SPI_CFG_PHASE_1EDGE		0x00
/* serial clock toggles at start of first data bit */
#define RK_SPI_CFG_PHASE_2EDGE		0x01

#define RK_SPI_CFG_POLARITY_LOW		0x00
#define RK_SPI_CFG_POLARITY_HIGH	0x01

/*
 * The period between ss_n active and
 * sclk_out active is half sclk_out cycles
 */
#define RK_SPI_CFG_SSD_HALF		0x00
/*
 * The period between ss_n active and
 * sclk_out active is one sclk_out cycle
 */
#define RK_SPI_CFG_SSD_ONE		0x01

#define RK_SPI_CFG_EM_LITTLE		0x0
#define RK_SPI_CFG_EM_BIG		0x1

#define RK_SPI_CFG_FIRSTBIT_MSB		0x0
#define RK_SPI_CFG_FIRSTBIT_LSB		0x1

#define RK_SPI_CFG_BHT_16BIT		0x0
#define RK_SPI_CFG_BHT_8BIT		0x1

#define RK_SPI_CFG_XFM_TR		0x00
#define RK_SPI_CFG_XFM_TO		0x01
#define RK_SPI_CFG_XFM_RO		0x02

#define RK_SPI_CFG_OPM_MASTER		0x00
#define RK_SPI_CFG_OPM_SLAVE		0x01

#define RK_SPI_CFG_CSM_0CYCLE		0x00
#define RK_SPI_CFG_CSM_1CYCLE		0x01
#define RK_SPI_CFG_CSM_2CYCLES		0x02
#define RK_SPI_CFG_CSM_3CYCLES		0x03

/***************************** Structure Definition **************************/

/** @brief  SPI Configuration Structure definition */
struct rk_spi_config {
	uint32_t op_mode;		/**< Specifies the SPI operating mode, master or slave. */
	uint32_t xfm_mode;		/**< Specifies the SPI bidirectional mode state, tx only, rx only or trx mode. */
	uint32_t num_bytes;		/**< Specifies the SPI data size. */
	uint32_t clk_polarity;		/**< Specifies the serial clock steady state. */
	uint32_t clk_phase;		/**< Specifies the clock active edge for the bit capture. */
	uint32_t first_bit;		/**< Specifies whether data transfers start from MSB or LSB bit. */
	uint32_t endian_mode;		/**< Specifies whether data transfers start from little or big endian. */
	uint32_t apb_transform;		/**< Specifies apb transform type. */
	uint32_t ssd;			/**< Specifies period between ss_n active and sclk_out. */
	uint32_t speed;			/**< Specifies the Baud Rate prescaler value which will be
					     used to configure the transmit and receive SCK clock. */
	uint32_t ssi_type;		/**< Specifies if the TI mode is enabled or not.*/
	uint32_t csm;			/**< Specifies Motorola SPI Master SS_N high cycles for each frame data is transfer. */
};


/** @brief  SPI handle Structure definition */
struct rk_spi_controller {
	uintptr_t base;			/**< Specifies SPI registers base address. */
	uint32_t max_freq;		/**< Specifies SPI clock frequency. */
	uint32_t actual_speed;		/**< Specifies actual SPI clock frequency after source division */
	struct rk_spi_config config;	/**< Specifies SPI communication parameters. */
	const uint8_t *tx_buffer;	/**< Specifies pointer to SPI Tx transfer Buffer. */
	const uint8_t *tx_buffer_end;	/**< Specifies pointer to SPI Tx _end transfer Buffer. */
	uint8_t *rx_buffer;		/**< Specifies pointer to SPI Rx transfer Buffer. */
	uint8_t *rx_buffer_end;		/**< Specifies pointer to SPI Rx _end transfer Buffer. */
	uint32_t len;			/**< Specifies the transfer length . */
};

int rk_spi_init(struct rk_spi_controller *dev, uintptr_t base, uint32_t max_freq);
int rk_spi_flush_fifo(struct rk_spi_controller *dev);
int rk_spi_set_cs(struct rk_spi_controller *dev, uint8_t select, bool enable);
int rk_spi_transfer(struct rk_spi_controller *dev);
int rk_spi_stop(struct rk_spi_controller *dev);
int rk_spi_configure(struct rk_spi_controller *dev, const uint8_t *tx_data,
		     uint8_t *rx_data, uint32_t Size);
uint32_t rk_spi_calc_timeout_us(struct rk_spi_controller *dev);

#endif /* ROCKCHIP_SPI_API_H */
