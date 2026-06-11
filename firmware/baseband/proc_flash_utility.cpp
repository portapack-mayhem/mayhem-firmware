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



#include <cstring>



#include "w25q80bv.hpp"

#include "debug.hpp"

#include "portapack_shared_memory.hpp"



#define PAGE_LEN 256U

/* Program the packed image size (LIMIT), not the supported SPI map size (SIZE). */

#define NUM_PAGES (4096U * (uint32_t)FLASH_SIZE_LIMIT_MB)

#define PROGRAM_BYTES (NUM_PAGES * PAGE_LEN)



static TCHAR firmware_path[128];



static uint8_t* flash_data_buffer() {

    return &shared_memory.bb_data.data[SharedMemory::flash_utility_data_offset];

}



static void copy_firmware_path() {

    std::memset(firmware_path, 0, sizeof(firmware_path));

    std::memcpy(

        firmware_path,

        shared_memory.bb_data.data,

        SharedMemory::flash_utility_path_bytes - sizeof(TCHAR));

}



static uint32_t checksum_bytes(const uint8_t* ptr, const size_t length) {

    uint32_t checksum = 0;

    size_t i = 0;

    for (; i + 3 < length; i += 4) {

        uint32_t chunk = 0;

        std::memcpy(&chunk, ptr + i, 4);

        checksum += chunk;

    }

    if (i < length) {

        uint32_t remainder = 0;

        std::memcpy(&remainder, ptr + i, length - i);

        checksum += remainder;

    }

    return checksum;

}



static bool verify_programmed_image(const uint32_t expected_bytes) {

    uint8_t* const buffer = flash_data_buffer();

    uint32_t checksum = 0;



    for (size_t offset = 0; offset < expected_bytes; offset += PAGE_LEN) {

        w25q80bv::wait_not_busy();

        w25q80bv::read(offset, buffer, PAGE_LEN);

        checksum += checksum_bytes(buffer, PAGE_LEN);

    }



    return checksum == 0;

}



void initialize_flash();

void erase_flash();

void initialize_sdcard();

void write_firmware(FIL*);

void write_page(size_t, uint8_t*, size_t);



int main() {

    copy_firmware_path();



    initialize_sdcard();



    FIL firmware_file;

    if (f_open(&firmware_file, firmware_path, FA_READ) != FR_OK) {

        chDbgPanic("no file");

    }



    const FSIZE_t file_size = f_size(&firmware_file);

    if (file_size != PROGRAM_BYTES) {

        f_close(&firmware_file);

        chDbgPanic("bad size");

    }



    initialize_flash();

    if (!w25q80bv::wait_for_device_bounded()) {

        f_close(&firmware_file);

        chDbgPanic("no flash");

    }



    const uint8_t device_id = w25q80bv::get_device_id();

    const uint32_t capacity = w25q80bv::capacity_bytes_for_device_id(device_id);

    if ((capacity == 0) || (PROGRAM_BYTES > capacity)) {

        f_close(&firmware_file);

        chDbgPanic("flash cap");

    }



    palSetPad(LED_PORT, LEDRX_PAD);

    erase_flash();



    palSetPad(LED_PORT, LEDTX_PAD);



    write_firmware(&firmware_file);



    if (!verify_programmed_image(PROGRAM_BYTES)) {

        f_close(&firmware_file);

        chDbgPanic("verify");

    }



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

    uint8_t* const data_buffer = flash_data_buffer();



    for (size_t page_index = 0; page_index < NUM_PAGES; page_index++) {

        if (page_index % 32 == 0)

            palTogglePad(LED_PORT, LEDTX_PAD);



        UINT bytes_read = 0;

        if (f_read(firmware_file, data_buffer, PAGE_LEN, &bytes_read) != FR_OK) {

            chDbgPanic("no data");

        }



        if (bytes_read != PAGE_LEN) {

            chDbgPanic("short read");

        }



        write_page(page_index, data_buffer, bytes_read);

    }

}



void write_page(size_t page_index, uint8_t* data_buffer, size_t data_length) {

    w25q80bv::wait_not_busy();

    w25q80bv::remove_write_protection();

    w25q80bv::wait_not_busy();

    w25q80bv::write(page_index, data_buffer, data_length);

    w25q80bv::wait_not_busy();

}

