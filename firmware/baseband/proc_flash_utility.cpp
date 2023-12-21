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

#include "ch.h"
#include "hal.h"
#include "ff.h"

#include "w25q80bv.hpp"
#include "debug.hpp"
#include "portapack_shared_memory.hpp"

#define PAGE_LEN 256U
#define NUM_PAGES 4096U

void initialize_flash();
void erase_flash();
void initialize_sdcard();
void write_firmware(FIL*);
void write_page(size_t, uint8_t*, size_t);

int main() {
    const TCHAR* filename = reinterpret_cast<const TCHAR*>(&shared_memory.bb_data.data[0]);

    initialize_flash();
    palSetPad(LED_PORT, LEDRX_PAD);
    erase_flash();

    initialize_sdcard();

    FIL firmware_file;
    if (f_open(&firmware_file, filename, FA_READ) != FR_OK) chDbgPanic("no file");

    palSetPad(LED_PORT, LEDTX_PAD);

    write_firmware(&firmware_file);

    palClearPad(LED_PORT, LEDTX_PAD);
    palClearPad(LED_PORT, LEDRX_PAD);

    f_close(&firmware_file);

    LPC_RGU->RESET_CTRL[0] = (1 << 0);

    while (1)
        __WFE();

    return 0;
}

void initialize_flash() {
    w25q80bv::disable_spifi();
    w25q80bv::initialite_spi();
    w25q80bv::setup();

    w25q80bv::wait_for_device();
    w25q80bv::wait_not_busy();
}

void erase_flash() {
    w25q80bv::remove_write_protection();
    w25q80bv::wait_not_busy();

    w25q80bv::erase_chip();
    w25q80bv::wait_not_busy();
}

void initialize_sdcard() {
    static FATFS fs;

    sdcStart(&SDCD1, nullptr);
    if (sdcConnect(&SDCD1) == CH_FAILED) chDbgPanic("no sd card #1");
    if (f_mount(&fs, reinterpret_cast<const TCHAR*>(_T("")), 1) != FR_OK) chDbgPanic("no sd card #2");
}

void write_firmware(FIL* firmware_file) {
    uint8_t* data_buffer = &shared_memory.bb_data.data[0];

    for (size_t page_index = 0; page_index < NUM_PAGES; page_index++) {
        if (page_index % 32 == 0)
            palTogglePad(LED_PORT, LEDTX_PAD);

        size_t bytes_read;
        if (f_read(firmware_file, data_buffer, PAGE_LEN, &bytes_read) != FR_OK) chDbgPanic("no data");

        if (bytes_read > 0)
            write_page(page_index, data_buffer, bytes_read);

        if (bytes_read < PAGE_LEN)
            return;
    }
}

void write_page(size_t page_index, uint8_t* data_buffer, size_t data_length) {
    w25q80bv::wait_not_busy();
    w25q80bv::remove_write_protection();
    w25q80bv::wait_not_busy();
    w25q80bv::write(page_index, data_buffer, data_length);
    w25q80bv::wait_not_busy();
}
