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

#include "usb_serial_io.h"
#include "ff.h"
#include "chprintf.h"

#include <string>

#define SHELL_WA_SIZE THD_WA_SIZE(1024)
#define palOutputPad(port, pad) (LPC_GPIO->DIR[(port)] |= 1 << (pad))

static void cmd_reset(BaseSequentialStream* chp, int argc, char* argv[]) {
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

static const ShellCommand commands[] = {
    {"reset", cmd_reset},
    {"dfu", cmd_dfu},
    {"hackrf", cmd_hackrf},
    {"sd_over_usb", cmd_sd_over_usb},
    {"flash", cmd_flash},
    {"write_memory_8", cmd_reset},
    {"write_memory_32", cmd_reset},
    {"read_memory", cmd_reset},
    {"sd_list_dir", cmd_reset},
    {"sd_open_file", cmd_reset},
    {"sd_close_file", cmd_reset},
    {"sd_delete", cmd_reset},
    {"sd_read", cmd_reset},
    {"sd_write", cmd_reset},
    {"sd_seek", cmd_reset},
    {"screenshot", cmd_reset},
    {"button", cmd_reset},
    {NULL, NULL}};

static const ShellConfig shell_cfg1 = {
    (BaseSequentialStream*)&SUSBD1,
    commands};

void create_shell() {
    shellCreate(&shell_cfg1, SHELL_WA_SIZE, NORMALPRIO);
}
