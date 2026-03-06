/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2023 Bernd Herzog
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "w25q80bv.hpp"
#include "debug.hpp"

namespace w25q80bv {

void disable_spifi() {
    RESET_CTRL1 = RESET_CTRL1_SPIFI_RST;
}

void initialite_spi() {
    palSetPad(W25Q80BV_SELECT_PORT, W25Q80BV_SELECT_PAD);
    palOutputPad(W25Q80BV_SELECT_PORT, W25Q80BV_SELECT_PAD);

    const uint32_t clock_prescale_rate = 2;
    const uint32_t serial_clock_rate = 2;

    SSP_CR1(SSP0_BASE) = 0;
    SSP_CPSR(SSP0_BASE) = clock_prescale_rate;
    SSP_CR0(SSP0_BASE) = (serial_clock_rate << 8) | SSP_DATA_8BITS;
    SSP_CR1(SSP0_BASE) = SSP_ENABLE;
}

void setup() {
    /* Init SPIFI GPIO to Normal GPIO */

    MMIO32(PIN_GROUP3 + PIN3) = (SCU_SSP_IO | SCU_CONF_FUNCTION2);
    MMIO32(PIN_GROUP3 + PIN4) = (SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
    MMIO32(PIN_GROUP3 + PIN5) = (SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
    MMIO32(PIN_GROUP3 + PIN6) = (SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
    MMIO32(PIN_GROUP3 + PIN7) = (SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
    MMIO32(PIN_GROUP3 + PIN8) = (SCU_GPIO_FAST | SCU_CONF_FUNCTION4);

    /* configure SSP pins */
    MMIO32(SCU_SSP0_CIPO) = (SCU_SSP_IO | SCU_CONF_FUNCTION5);
    MMIO32(SCU_SSP0_COPI) = (SCU_SSP_IO | SCU_CONF_FUNCTION5);
    MMIO32(SCU_SSP0_SCK) = (SCU_SSP_IO | SCU_CONF_FUNCTION2);

    /* configure GPIO pins */
    MMIO32(SCU_FLASH_HOLD) = SCU_GPIO_FAST;
    MMIO32(SCU_FLASH_WP) = SCU_GPIO_FAST;
    MMIO32(SCU_SSP0_CS) = (SCU_GPIO_FAST | SCU_CONF_FUNCTION4);

    // remove hardware write protection
    /* drive CS, HOLD, and WP pins high */
    palSetPad(W25Q80BV_HOLD_PORT, W25Q80BV_HOLD_PAD);
    palSetPad(W25Q80BV_WP_PORT, W25Q80BV_WP_PAD);

    /* Set GPIO pins as outputs. */
    palOutputPad(W25Q80BV_HOLD_PORT, W25Q80BV_HOLD_PAD);
    palOutputPad(W25Q80BV_WP_PORT, W25Q80BV_WP_PAD);
}

void wait_for_device() {
    uint8_t device_id;
    do {
        device_id = get_device_id();
    } while (device_id != W25Q80BV_DEVICE_ID_RES &&
             device_id != W25Q16DV_DEVICE_ID_RES);
}

void wait_not_busy() {
    while (get_status() & W25Q80BV_STATUS_BUSY) {
        HALT_IF_DEBUGGING();
    }
}

void remove_write_protection() {
    uint8_t data[] = {W25Q80BV_WRITE_ENABLE};
    palClearPad(W25Q80BV_SELECT_PORT, W25Q80BV_SELECT_PAD);

    for (size_t j = 0; j < 1; j++) {
        data[j] = spi_ssp_transfer_word(data[j]);
    }

    palSetPad(W25Q80BV_SELECT_PORT, W25Q80BV_SELECT_PAD);

    while (!(get_status() & W25Q80BV_STATUS_WEL)) {
        HALT_IF_DEBUGGING();
    }
}

uint32_t spi_ssp_transfer_word(const uint32_t data) {
    while ((SSP_SR(SSP0_BASE) & SSP_SR_TNF) == 0) {
        HALT_IF_DEBUGGING();
    }
    SSP_DR(SSP0_BASE) = data;
    while (SSP_SR(SSP0_BASE) & SSP_SR_BSY) {
        HALT_IF_DEBUGGING();
    }
    while ((SSP_SR(SSP0_BASE) & SSP_SR_RNE) == 0) {
        HALT_IF_DEBUGGING();
    }
    return SSP_DR(SSP0_BASE);
}

uint8_t get_status() {
    uint8_t data[] = {W25Q80BV_READ_STATUS1, 0xFF};
    palClearPad(W25Q80BV_SELECT_PORT, W25Q80BV_SELECT_PAD);

    for (size_t j = 0; j < 2; j++) {
        data[j] = spi_ssp_transfer_word(data[j]);
    }

    palSetPad(W25Q80BV_SELECT_PORT, W25Q80BV_SELECT_PAD);

    return data[1];
}

uint8_t get_device_id() {
    uint8_t data[] = {W25Q80BV_DEVICE_ID, 0xFF, 0xFF, 0xFF, 0xFF};
    palClearPad(W25Q80BV_SELECT_PORT, W25Q80BV_SELECT_PAD);

    for (size_t j = 0; j < 5; j++) {
        data[j] = spi_ssp_transfer_word(data[j]);
    }

    palSetPad(W25Q80BV_SELECT_PORT, W25Q80BV_SELECT_PAD);

    return data[4];
}

void erase_chip() {
    uint8_t data[] = {W25Q80BV_CHIP_ERASE};
    palClearPad(W25Q80BV_SELECT_PORT, W25Q80BV_SELECT_PAD);

    for (size_t j = 0; j < 1; j++) {
        data[j] = spi_ssp_transfer_word(data[j]);
    }

    palSetPad(W25Q80BV_SELECT_PORT, W25Q80BV_SELECT_PAD);
}

void write(size_t page_index, uint8_t* data_buffer, size_t length) {
    size_t page_len = 256U;
    size_t addr = page_index * page_len;
    uint8_t header[] = {
        W25Q80BV_PAGE_PROGRAM,
        (uint8_t)((addr & 0xFF0000) >> 16),
        (uint8_t)((addr & 0xFF00) >> 8),
        (uint8_t)(addr & 0xFF)};

    palClearPad(W25Q80BV_SELECT_PORT, W25Q80BV_SELECT_PAD);
    for (size_t j = 0; j < 4; j++) {
        header[j] = spi_ssp_transfer_word(header[j]);
    }

    for (size_t j = 0; j < length; j++) {
        data_buffer[j] = spi_ssp_transfer_word(data_buffer[j]);
    }
    palSetPad(W25Q80BV_SELECT_PORT, W25Q80BV_SELECT_PAD);
}
}  // namespace w25q80bv
