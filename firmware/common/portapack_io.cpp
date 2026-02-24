/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * copyleft 2024 zxkmm AKA zix aka sommermorgentraum
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

#include "portapack_io.hpp"
#include "portapack_persistent_memory.hpp"

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

#include "utility.hpp"

#include <ch.h>

#include <cstdint>

namespace portapack {

DeviceType device_type = DEV_PORTAPACK;

void IO::lcd_read_bytes(uint8_t* byte, size_t byte_count) {
    if (portapack::device_type == portapack::DeviceType::DEV_PORTAPACK) {
        size_t word_count = byte_count / 2;
        while (word_count) {
            const auto word = lcd_read_data();
            *(byte++) = word >> 8;
            *(byte++) = word >> 0;
            word_count--;
        }
        if (byte_count & 1) {
            const auto word = lcd_read_data();
            *(byte++) = word >> 8;
        }
        return;
    }
    // prf
    //--dummy read:
    dir_read();
    lcd_rd_assert();
    halPolledDelay(71);
    data_read();
    lcd_rd_deassert();
    size_t word_count = byte_count / 3;
    for (size_t i = 0; i < word_count; i++) {
        uint32_t word = lcd_read_data();  // reads 3 byte of data
        uint8_t r = ((word >> 16) & 0xff);
        uint8_t g = ((word >> 8) & 0xff);
        uint8_t b = ((word >> 0) & 0xff);
        *(byte++) = r;
        *(byte++) = g << 2;
        *(byte++) = b;
    }
}

void IO::init() {
    data_mask_set();
    data_write_high(0);

    dir_read();
    lcd_rd_deassert();
    lcd_wr_deassert();
    io_stb_deassert();
    addr(0);

    gpio_dir.output();
    gpio_lcd_rdx.output();
    gpio_lcd_wrx.output();
    gpio_io_stbx.output();
    gpio_addr.output();
    gpio_rot_a.input();
    gpio_rot_b.input();
}

void IO::lcd_backlight(const bool value) {
    io_reg = (io_reg & 0x7f) | ((value ? 1 : 0) << 7);
    io_write(1, io_reg);
}

void IO::lcd_reset_state(const bool active) {
    io_reg = (io_reg & 0xfe) | ((active ? 1 : 0) << 0);
    io_write(1, io_reg);
}

void IO::audio_reset_state(const bool active) {
    /* NOTE: This overwrites the contents of the IO register, which for now
     * have no significance. But someday...?
     */
    io_reg = (io_reg & 0xfd) | ((active ? 1 : 0) << 1);
    io_write(1, io_reg);
}

void IO::reference_oscillator(const bool enable) {
    const uint8_t mask = 1 << 6;
    io_reg = (io_reg & ~mask) | (enable ? mask : 0);
    io_write(1, io_reg);
}

bool IO::get_dark_cover() {
    return portapack::persistent_memory::apply_fake_brightness();
}

bool IO::get_is_normally_black() {
    return portapack::persistent_memory::config_lcd_normally_black();
}

uint8_t IO::get_brightness() {
    return portapack::persistent_memory::fake_brightness_level();
}

void IO::update_cached_values() {
    lcd_normally_black = get_is_normally_black();
    dark_cover_enabled = get_dark_cover();
    brightness = get_brightness();
}

uint32_t IO::io_update(const TouchPinsConfig write_value) {
    /* Very touchy code to save context of PortaPack data bus while the
     * resistive touch pin drive is changed. Order of operations is
     * important to prevent latching spurious data into the LCD or IO
     * registers.
     */

    const auto save_data = data_read();
    const auto addr = gpio_addr.read();
    const auto dir = gpio_dir.read();

    io_stb_assert();

    /* Switch to read */
    dir_read();
    addr_0();
    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
    const auto switches_raw = data_read();

    /* Switch to write */
    data_write_low(toUType(write_value));
    dir_write();
    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
    io_stb_deassert();

    data_write_low(save_data);
    if (dir) { /* 0 (write) -> 1 (read) */
        dir_read();
    }
    gpio_addr.write(addr);

    auto dfu_btn = portapack::io.dfu_read() & 0x01;
    return (switches_raw & 0x7f) | (dfu_btn << 7);
}

}  // namespace portapack
