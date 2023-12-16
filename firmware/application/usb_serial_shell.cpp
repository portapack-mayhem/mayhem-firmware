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

#include "usb_serial_io.h"

#define SHELL_WA_SIZE THD_WA_SIZE(1024)

static void cmd_reset(BaseSequentialStream* chp, int argc, char* argv[]) {
    LPC_CREG->M4MEMMAP = 0;
    LPC_RGU->RESET_CTRL[0] = (1 << 0);

    (void)argv;
}

static const ShellCommand commands[] = {
    {"reset", cmd_reset},
    {"dfu", cmd_reset},
    {"hackrf", cmd_reset},
    {"sd_over_usb", cmd_reset},
    {"flash", cmd_reset},
    {"write_memory_8", cmd_reset},
    {"write_memory_16", cmd_reset},
    {"write_memory_32", cmd_reset},
    {"read_memory", cmd_reset},
    {"sd_list_dir", cmd_reset},
    {"sd_open_file", cmd_reset},
    {"sd_close_file", cmd_reset},
    {"sd_delete", cmd_reset},
    {"sd_read", cmd_reset},
    {"sd_write", cmd_reset},
    {"sd_seek", cmd_reset},
    {NULL, NULL}};

static const ShellConfig shell_cfg1 = {
    (BaseSequentialStream*)&SUSBD1,
    commands};

void create_shell() {
    shellCreate(&shell_cfg1, SHELL_WA_SIZE, NORMALPRIO);
}
