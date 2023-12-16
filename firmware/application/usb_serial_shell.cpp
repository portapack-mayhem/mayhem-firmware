/*
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

#include "usb_serial_shell.hpp"
#include "event_m0.hpp"
#include "baseband_api.hpp"
#include "core_control.hpp"
#include "bitmap.hpp"
#include "png_writer.hpp"
#include "irq_controls.hpp"

#include "usb_serial_io.h"
#include "ff.h"
#include "chprintf.h"

#include <string>

#define SHELL_WA_SIZE THD_WA_SIZE(1024)
#define palOutputPad(port, pad) (LPC_GPIO->DIR[(port)] |= 1 << (pad))

static void cmd_reboot(BaseSequentialStream* chp, int argc, char* argv[]) {
    (void)chp;
    (void)argc;
    (void)argv;

    LPC_RGU->RESET_CTRL[0] = (1 << 0);
}

static void cmd_dfu(BaseSequentialStream* chp, int argc, char* argv[]) {
    (void)chp;
    (void)argc;
    (void)argv;

    LPC_SCU->SFSP2_8 = (LPC_SCU->SFSP2_8 & ~(7)) | 4;
    palOutputPad(5, 7);
    palSetPad(5, 7);

    LPC_RGU->RESET_CTRL[0] = (1 << 0);
}

static void cmd_hackrf(BaseSequentialStream* chp, int argc, char* argv[]) {
    (void)chp;
    (void)argc;
    (void)argv;

    EventDispatcher::request_stop();
}

static void cmd_sd_over_usb(BaseSequentialStream* chp, int argc, char* argv[]) {
    (void)chp;
    (void)argc;
    (void)argv;

    ui::Painter painter;
    painter.fill_rectangle(
        {0, 0, portapack::display.width(), portapack::display.height()},
        ui::Color::black());

    painter.draw_bitmap(
        {portapack::display.width() / 2 - 8, portapack::display.height() / 2 - 8},
        ui::bitmap_icon_hackrf,
        ui::Color::yellow(),
        ui::Color::black());

    sdcDisconnect(&SDCD1);
    sdcStop(&SDCD1);

    portapack::shutdown(true);
    m4_init(portapack::spi_flash::image_tag_usb_sd, portapack::memory::map::m4_code, false);
    m0_halt();
}

static void cmd_flash(BaseSequentialStream* chp, int argc, char* argv[]) {
    (void)chp;
    (void)argc;
    (void)argv;

    if (argc != 1) {
        chprintf((BaseSequentialStream*)&SUSBD1, "Usage: flash /FIRMWARE/portapack-mayhem.bin\r\n");
        return;
    }

    char16_t* wide_string = new char16_t[strlen(argv[0]) + 1];
    for (size_t i = 0; i < strlen(argv[0]); i++) {
        wide_string[i] = argv[0][i];
    }
    wide_string[strlen(argv[0])] = 0;

    std::u16string path = std::u16string(wide_string);
    delete wide_string;

    std::memcpy(&shared_memory.bb_data.data[0], path.c_str(), (path.length() + 1) * 2);
    m4_init(portapack::spi_flash::image_tag_flash_utility, portapack::memory::map::m4_code, false);
    m0_halt();
}

static void cmd_screenshot(BaseSequentialStream* chp, int argc, char* argv[]) {
    (void)chp;
    (void)argc;
    (void)argv;

    ensure_directory("SCREENSHOTS");
    auto path = next_filename_matching_pattern(u"SCREENSHOTS/SCR_????.PNG");

    if (path.empty())
        return;

    PNGWriter png;
    auto error = png.create(path);
    if (error)
        return;

    for (int i = 0; i < ui::screen_height; i++) {
        std::array<ui::ColorRGB888, ui::screen_width> row;
        portapack::display.read_pixels({0, i, ui::screen_width, 1}, row);
        png.write_scanline(row);
    }
}

static void cmd_write_memory(BaseSequentialStream* chp, int argc, char* argv[]) {
    (void)chp;

    if (argc != 2) {
        chprintf((BaseSequentialStream*)&SUSBD1, "usage: write_memory <address> <value (1 or 4 bytes)>\r\n");
        chprintf((BaseSequentialStream*)&SUSBD1, "example: write_memory 0x40004008 0x00000002\r\n");
        return;
    }

    int value_length = strlen(argv[1]);
    if (value_length != 10 && value_length != 4) {
        chprintf((BaseSequentialStream*)&SUSBD1, "usage: write_memory <address> <value (1 or 4 bytes)>\r\n");
        chprintf((BaseSequentialStream*)&SUSBD1, "example: write_memory 0x40004008 0x00000002\r\n");
        return;
    }

    uint32_t address = (uint32_t)strtol(argv[0], NULL, 16);
    uint32_t value = (uint32_t)strtol(argv[1], NULL, 16);

    if (value_length == 10) {
        uint32_t* data_pointer = (uint32_t*)address;
        *data_pointer = value;
    } else {
        uint8_t* data_pointer = (uint8_t*)address;
        *data_pointer = (uint8_t)value;
    }
}

static void cmd_read_memory(BaseSequentialStream* chp, int argc, char* argv[]) {
    (void)chp;
    (void)argc;

    if (argc != 1) {
        chprintf((BaseSequentialStream*)&SUSBD1, "usage: read_memory 0x40004008\r\n");
        return;
    }

    int address = (int)strtol(argv[0], NULL, 16);
    uint32_t* data_pointer = (uint32_t*)address;

    chprintf((BaseSequentialStream*)&SUSBD1, "%x\r\n", *data_pointer);
}

static void cmd_button(BaseSequentialStream* chp, int argc, char* argv[]) {
    if (argc != 1) {
        chprintf(chp, "usage: button 1\r\n");
        return;
    }

    int button = (int)strtol(argv[0], NULL, 10);
    if (button < 1 || button > 8) {
        chprintf(chp, "usage: button 1\r\n");
        return;
    }

    control::debug::inject_switch(button);
}

static const ShellCommand commands[] = {
    {"reboot", cmd_reboot},
    {"dfu", cmd_dfu},
    {"hackrf", cmd_hackrf},
    {"sd_over_usb", cmd_sd_over_usb},
    {"flash", cmd_flash},
    {"screenshot", cmd_screenshot},
    {"write_memory", cmd_write_memory},
    {"read_memory", cmd_read_memory},
    {"button", cmd_button},
    {"sd_list_dir", cmd_reboot},
    {"sd_open_file", cmd_reboot},
    {"sd_close_file", cmd_reboot},
    {"sd_delete", cmd_reboot},
    {"sd_read", cmd_reboot},
    {"sd_write", cmd_reboot},
    {"sd_seek", cmd_reboot},
    {NULL, NULL}};

static const ShellConfig shell_cfg1 = {
    (BaseSequentialStream*)&SUSBD1,
    commands};

void create_shell() {
    shellCreate(&shell_cfg1, SHELL_WA_SIZE, NORMALPRIO);
}
