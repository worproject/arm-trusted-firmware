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
#include <lib/mmio.h>

#include <rockchip_spi.h>

#include "spi.h"

/** @defgroup How_To_Use How To Use
 *

 The SPI HAL driver can be used as follows:

 - Declare a rk_spi_controller handle structure, for example:
   ```
   struct rk_spi_controller instance;
   ```

 - Invoke rk_spi_init() API to configure default config:
     - op_mode: slave or master
     - apb_transform
     - endian_mode
     - ssd
     - Clock rate

 - Invoke rk_spi_configure() API to program other mode:
     - Data size
     - Clock polarity and phase
     - FirstBit
     - Clock div
     - Number of data frames received at RX only mode
     - IT FIFO Level and DMA FIFO Level
     - Transfer Mode

 - Blocking transfer:
     - The communication is performed in polling mode by calling rk_spi_transfer().
     - after transfer done, invoke rk_spi_stop to release the chip select.

 */

#define IS_SPI_MODE(__MODE__) (((__MODE__) == RK_SPI_CFG_OPM_SLAVE) || \
                               ((__MODE__) == RK_SPI_CFG_OPM_MASTER))

#define IS_SPI_DIRECTION(__MODE__) (((__MODE__) == RK_SPI_CFG_XFM_TR) || \
                                    ((__MODE__) == RK_SPI_CFG_XFM_TO) || \
                                    ((__MODE__) == RK_SPI_CFG_XFM_RO))

#define IS_SPI_DATASIZE(__DATASIZE__) (((__DATASIZE__) == RK_SPI_CFG_DATA_FRAME_SIZE_4BIT) || \
                                       ((__DATASIZE__) == RK_SPI_CFG_DATA_FRAME_SIZE_8BIT) || \
                                       ((__DATASIZE__) == RK_SPI_CFG_DATA_FRAME_SIZE_16BIT))

#define IS_SPI_CPOL(__CPOL__) (((__CPOL__) == RK_SPI_CFG_POLARITY_LOW) || \
                               ((__CPOL__) == RK_SPI_CFG_POLARITY_HIGH))

#define IS_SPI_CPHA(__CPHA__) (((__CPHA__) == RK_SPI_CFG_PHASE_1EDGE) || \
                               ((__CPHA__) == RK_SPI_CFG_PHASE_2EDGE))

#define IS_SPI_FIRST_BIT(__BIT__) (((__BIT__) == RK_SPI_CFG_FIRSTBIT_MSB) || \
                                   ((__BIT__) == RK_SPI_CFG_FIRSTBIT_LSB))

#define IS_SPI_APBTRANSFORM(__MODE__) (((__MODE__) == RK_SPI_CFG_BHT_16BIT) || \
                                      ((__MODE__) == RK_SPI_CFG_BHT_8BIT))

#define IS_SPI_ENDIAN_MODE(__MODE__) (((__MODE__) == RK_SPI_CFG_EM_BIG) || \
                                      ((__MODE__) == RK_SPI_CFG_EM_LITTLE))

#define IS_SPI_SSD_BIT(__MODE__) (((__MODE__) == RK_SPI_CFG_SSD_HALF) || \
                                  ((__MODE__) == RK_SPI_CFG_SSD_ONE))

#define IS_SPI_CSM(__NCYCLES__) (((__NCYCLES__) == RK_SPI_CFG_CSM_0CYCLE) ||  \
                                 ((__NCYCLES__) == RK_SPI_CFG_CSM_1CYCLE) ||  \
                                 ((__NCYCLES__) == RK_SPI_CFG_CSM_2CYCLES) || \
                                 ((__NCYCLES__) == RK_SPI_CFG_CSM_3CYCLES))

/**
  * @brief  Initialize the SPI according to the specified parameters.
  * @param  dev: pointer to a rk_spi_controller structure that contains
  *               the configuration information for SPI module.
  * @param  base: SPI controller register base address.
  * @return status
  */
int rk_spi_init(struct rk_spi_controller *dev, uintptr_t base, uint32_t max_freq)
{
	dev->base = base;
	dev->max_freq = max_freq;

	/* Default config */
	dev->config.op_mode = RK_SPI_CFG_OPM_MASTER;
	dev->config.apb_transform = RK_SPI_CFG_BHT_8BIT;
	dev->config.endian_mode = RK_SPI_CFG_EM_BIG;
	dev->config.ssd = RK_SPI_CFG_SSD_ONE;
	dev->config.csm = RK_SPI_CFG_CSM_0CYCLE;

	return 0;
}

/**
  * @brief  Start or stop the spi module.
  * @param  dev: pointer to a rk_spi_controller structure that contains
  *               the configuration information for SPI module.
  * @param  enable: start or stop the spi module.
  * @return status
  */
static inline int rk_spi_enable_chip(struct rk_spi_controller *dev, int enable)
{
	mmio_write_32(dev->base + SPI_ENR, (enable ? 1 : 0));

	return 0;
}

/**
  * @brief  Configure the spi clock division.
  * @param  dev: pointer to a rk_spi_controller structure that contains
  *               the configuration information for SPI module.
  * @param  div: clock division.
  * @return status
  */
static inline int rk_spi_set_clock(struct rk_spi_controller *dev, uint16_t div)
{
	mmio_write_32(dev->base + SPI_BAUDR, div);

	return 0;
}

/**
  * @brief  Configure the cs signal.
  * @param  dev: pointer to a rk_spi_controller structure that contains
  *               the configuration information for SPI module.
  * @param select: cs number select.
  * @param  enable: active or inactive the cs signal.
  * @return status
  */
int rk_spi_set_cs(struct rk_spi_controller *dev, uint8_t select, bool enable)
{
	uint32_t ser;

	assert(dev != NULL);

	ser = mmio_read_32(dev->base + SPI_SER) & SPI_SER_SER_MASK;

	if (enable) {
		ser |= 1 << select;
	} else {
		ser &= ~(1 << select);
	}

	mmio_write_32(dev->base + SPI_SER, ser);

	return 0;
}

/**
  * @brief  Wait for the transfer finished.
  * @param  dev: pointer to a rk_spi_controller structure that contains
  *               the configuration information for SPI module.
  * @return status
  */
int rk_spi_flush_fifo(struct rk_spi_controller *dev)
{
	assert(dev != NULL);

	while (mmio_read_32(dev->base + SPI_RXFLR)) {
		mmio_read_32(dev->base + SPI_RXDR);
	}

	return 0;
}

/**
  * @brief  The max amount of data can be written in blocking mode.
  * @param  dev: pointer to a rk_spi_controller structure that contains
  *               the configuration information for SPI module.
  * @return Max bytes can xfer.
  */
static inline uint32_t rk_spi_tx_max(struct rk_spi_controller *dev)
{
	uint32_t txLeft, txRoom;

	txLeft = (dev->tx_buffer_end - dev->tx_buffer) / dev->config.num_bytes;
	txRoom = SPI_FIFO_LENGTH - mmio_read_32(dev->base + SPI_TXFLR);

	return MIN(txLeft, txRoom);
}

/**
  * @brief  Send an amount of data in blocking mode.
  * @param  dev: pointer to a rk_spi_controller structure that contains
  *               the configuration information for SPI module.
  * @return status
  */
static int rk_spi_pio_write(struct rk_spi_controller *dev)
{
	uint32_t max = rk_spi_tx_max(dev);
	uint32_t txw = 0;

	while (max--) {
		if (dev->config.num_bytes == 1) {
			txw = *(const uint8_t *)(dev->tx_buffer);
		} else {
			txw = *(const uint16_t *)(dev->tx_buffer);
		}

		mmio_write_32(dev->base + SPI_TXDR, txw);
		dev->tx_buffer += dev->config.num_bytes;
	}

	return 0;
}

/**
  * @brief  Read an amount of data(byte) in blocking mode.
  * @param  dev: pointer to a rk_spi_controller structure that contains
  *               the configuration information for SPI module.
  * @return status
  */
static int rk_spi_pio_read_byte(struct rk_spi_controller *dev)
{
	uint32_t rx_left = dev->rx_buffer_end - dev->rx_buffer;
	uint32_t rx_room = mmio_read_32(dev->base + SPI_RXFLR);
	uint32_t max = MIN(rx_left, rx_room);

	while (max > 7) {
		*(dev->rx_buffer + 0) = (uint8_t)mmio_read_32(dev->base + SPI_RXDR);
		*(dev->rx_buffer + 1) = (uint8_t)mmio_read_32(dev->base + SPI_RXDR);
		*(dev->rx_buffer + 2) = (uint8_t)mmio_read_32(dev->base + SPI_RXDR);
		*(dev->rx_buffer + 3) = (uint8_t)mmio_read_32(dev->base + SPI_RXDR);
		*(dev->rx_buffer + 4) = (uint8_t)mmio_read_32(dev->base + SPI_RXDR);
		*(dev->rx_buffer + 5) = (uint8_t)mmio_read_32(dev->base + SPI_RXDR);
		*(dev->rx_buffer + 6) = (uint8_t)mmio_read_32(dev->base + SPI_RXDR);
		*(dev->rx_buffer + 7) = (uint8_t)mmio_read_32(dev->base + SPI_RXDR);
		dev->rx_buffer += 8;
		max -= 8;
	}

	while (max--) {
		*dev->rx_buffer = (uint8_t)mmio_read_32(dev->base + SPI_RXDR);
		dev->rx_buffer++;
	}

	return 0;
}

/**
  * @brief  Read an amount of data(short) in blocking mode.
  * @param  dev: pointer to a rk_spi_controller structure that contains
  *               the configuration information for SPI module.
  * @return status
  */
static int rk_spi_pio_read_short(struct rk_spi_controller *dev)
{
	uint32_t rx_left = (dev->rx_buffer_end - dev->rx_buffer) >> 1;
	uint32_t rx_room = mmio_read_32(dev->base + SPI_RXFLR);
	uint32_t max = MIN(rx_left, rx_room);

	while (max > 7) {
		*((uint16_t *)dev->rx_buffer + 0) = (uint16_t)mmio_read_32(dev->base + SPI_RXDR);
		*((uint16_t *)dev->rx_buffer + 1) = (uint16_t)mmio_read_32(dev->base + SPI_RXDR);
		*((uint16_t *)dev->rx_buffer + 2) = (uint16_t)mmio_read_32(dev->base + SPI_RXDR);
		*((uint16_t *)dev->rx_buffer + 3) = (uint16_t)mmio_read_32(dev->base + SPI_RXDR);
		*((uint16_t *)dev->rx_buffer + 4) = (uint16_t)mmio_read_32(dev->base + SPI_RXDR);
		*((uint16_t *)dev->rx_buffer + 5) = (uint16_t)mmio_read_32(dev->base + SPI_RXDR);
		*((uint16_t *)dev->rx_buffer + 6) = (uint16_t)mmio_read_32(dev->base + SPI_RXDR);
		*((uint16_t *)dev->rx_buffer + 7) = (uint16_t)mmio_read_32(dev->base + SPI_RXDR);
		dev->rx_buffer += 16;
		max -= 8;
	}

	while (max--) {
		*((uint16_t *)dev->rx_buffer) = (uint16_t)mmio_read_32(dev->base + SPI_RXDR);
		dev->rx_buffer += 2;
	}

	return 0;
}

/**
  * @brief  Transmit an amount of data in blocking mode.
  * @param  dev: pointer to a rk_spi_controller structure that contains
  *               the configuration information for SPI module.
  * @return status
  */
int rk_spi_transfer(struct rk_spi_controller *dev)
{
	uint32_t remain = 0;
	uint64_t timeout;

	assert(dev != NULL);

	timeout = timeout_init_us(rk_spi_calc_timeout_us(dev));

	rk_spi_enable_chip(dev, 1);

	do {
		if (dev->tx_buffer) {
			remain = dev->tx_buffer_end - dev->tx_buffer;
			rk_spi_pio_write(dev);
		}

		if (dev->rx_buffer) {
			remain = dev->rx_buffer_end - dev->rx_buffer;

			if (dev->config.num_bytes == 1) {
				rk_spi_pio_read_byte(dev);
			} else {
				rk_spi_pio_read_short(dev);
			}
		}

		if (timeout_elapsed(timeout)) {
			return -ETIMEDOUT;
		}
	} while (remain);

	return 0;
}

uint32_t rk_spi_calc_timeout_us(struct rk_spi_controller *dev)
{
	uint64_t timeout;

	assert(dev != NULL);

	timeout = dev->len * 8ULL * 1000000ULL / dev->actual_speed;
	timeout += timeout + 100000; /* some tolerance */

    	return timeout;
}

/**
  * @brief  Query the SPI bus state is idle or busy.
  * @param  dev: pointer to a rk_spi_controller structure that contains
  *               the configuration information for SPI module.
  * @return HAL status
  */
int rk_spi_query_bus_state(struct rk_spi_controller *dev)
{
	assert(dev != NULL);

	if (!(mmio_read_32(dev->base + SPI_SR) & SPI_SR_BUSY)) {
		return 0;
	}

	return -EBUSY;
}

/**
  * @brief  Stop the transmit.
  * @param  dev: pointer to a rk_spi_controller structure that contains
  *               the configuration information for SPI module.
  * @return status
  */
int rk_spi_stop(struct rk_spi_controller *dev)
{
	int ret = 0;
	uint64_t timeout;

	assert(dev != NULL);

	timeout = timeout_init_us(5000);

	while (rk_spi_query_bus_state(dev) != 0) {
		if (timeout_elapsed(timeout)) {
			ret = -ETIMEDOUT;
			break;
		}
	}

	rk_spi_enable_chip(dev, 0);

	return ret;
}

/**
  * @brief  Configure the SPI transfer mode depend on the tx/rx buffer.
  * @param  dev: pointer to a rk_spi_controller structure that contains
  *               the configuration information for SPI module.
  * @return status
  */
static int rk_spi_configure_transfer_mode(struct rk_spi_controller *dev)
{
	uint32_t cr0;

	if (dev->tx_buffer && dev->rx_buffer) {
		dev->config.xfm_mode = RK_SPI_CFG_XFM_TR;
	} else if (dev->tx_buffer) {
		dev->config.xfm_mode = RK_SPI_CFG_XFM_TO;
	} else if (dev->rx_buffer) {
		dev->config.xfm_mode = RK_SPI_CFG_XFM_RO;
	}

	cr0 = mmio_read_32(dev->base + SPI_CTRLR0);
	cr0 &= ~SPI_CTRLR0_XFM_MASK;
	cr0 |= (dev->config.xfm_mode << SPI_CTRLR0_XFM_SHIFT) & SPI_CTRLR0_XFM_MASK;

	mmio_write_32(dev->base + SPI_CTRLR0, cr0);

	return 0;
}

/**
  * @brief  Program the SPI config via this api.
  * @param  dev: pointer to a rk_spi_controller structure that contains
  *               the configuration information for SPI module.
  * @param  tx_data: pointer to TX buffer.
  * @param  rx_data: pointer to RX buffer.
  * @param  size: amount of data to be sent.
  * @return status
  */
int rk_spi_configure(struct rk_spi_controller *dev, const uint8_t *tx_data, uint8_t *rx_data, uint32_t size)
{
	uint32_t cr0 = 0;
	uint32_t div = 0;

	assert(dev != NULL);
	assert((tx_data != NULL) || (rx_data != NULL));
	assert(IS_SPI_MODE(dev->config.op_mode));
	assert(IS_SPI_DIRECTION(dev->config.xfm_mode));
	assert(IS_SPI_DATASIZE(dev->config.num_bytes));
	assert(IS_SPI_CPOL(dev->config.clk_polarity));
	assert(IS_SPI_CPHA(dev->config.clk_phase));
	assert(IS_SPI_FIRST_BIT(dev->config.first_bit));
	assert(IS_SPI_ENDIAN_MODE(dev->config.endian_mode));
	assert(IS_SPI_APBTRANSFORM(dev->config.apb_transform));
	assert(IS_SPI_SSD_BIT(dev->config.ssd));
	assert(IS_SPI_CSM(dev->config.csm));

	cr0 |= (dev->config.op_mode << SPI_CTRLR0_OPM_SHIFT) & SPI_CTRLR0_OPM_MASK;
	cr0 |= (dev->config.xfm_mode << SPI_CTRLR0_XFM_SHIFT) & SPI_CTRLR0_XFM_MASK;

	cr0 |= (dev->config.apb_transform << SPI_CTRLR0_BHT_SHIFT) & SPI_CTRLR0_BHT_MASK;
	cr0 |= (dev->config.endian_mode << SPI_CTRLR0_EM_SHIFT) & SPI_CTRLR0_EM_MASK;
	cr0 |= (dev->config.ssd << SPI_CTRLR0_SSD_SHIFT) & SPI_CTRLR0_SSD_MASK;

	/* Data width */
	cr0 |= (dev->config.num_bytes << SPI_CTRLR0_DFS_SHIFT) & SPI_CTRLR0_DFS_MASK;

	/* Mode for polarity, phase, first bit and endian */
	cr0 |= (dev->config.clk_polarity << SPI_CTRLR0_SCPOL_SHIFT) & SPI_CTRLR0_SCPOL_MASK;
	cr0 |= (dev->config.clk_phase << SPI_CTRLR0_SCPH_SHIFT) & SPI_CTRLR0_SCPH_MASK;
	cr0 |= (dev->config.first_bit << SPI_CTRLR0_FBM_SHIFT) & SPI_CTRLR0_FBM_MASK;

	/* Config CSM cycles */
	cr0 |= (dev->config.csm << SPI_CTRLR0_CSM_SHIFT) & SPI_CTRLR0_CSM_MASK;

	/* div doesn't support odd number */
	div = div_round_up(dev->max_freq, dev->config.speed);
	div = (div + 1) & 0xfffe;

	dev->actual_speed = dev->max_freq / div;

	mmio_write_32(dev->base + SPI_CTRLR0, cr0);

	mmio_write_32(dev->base + SPI_TXFTLR, SPI_FIFO_LENGTH / 2 - 1);
	mmio_write_32(dev->base + SPI_RXFTLR, SPI_FIFO_LENGTH / 2 - 1);

	mmio_write_32(dev->base + SPI_DMATDLR, SPI_FIFO_LENGTH / 2 - 1);
	mmio_write_32(dev->base + SPI_DMARDLR, 0);

	rk_spi_set_clock(dev, div);

	dev->tx_buffer = tx_data;
	dev->tx_buffer_end = tx_data + size;
	dev->rx_buffer = rx_data;
	dev->rx_buffer_end = rx_data + size;
	dev->len = size;

	rk_spi_configure_transfer_mode(dev);

	if (dev->config.xfm_mode == RK_SPI_CFG_XFM_RO) {
		if (dev->config.num_bytes == 1) {
			mmio_write_32(dev->base + SPI_CTRLR1, dev->len - 1);
		} else if (dev->config.num_bytes == 2) {
			mmio_write_32(dev->base + SPI_CTRLR1, (dev->len / 2) - 1);
		} else {
			mmio_write_32(dev->base + SPI_CTRLR1, (dev->len * 2) - 1);
		}
	}

	return 0;
}
