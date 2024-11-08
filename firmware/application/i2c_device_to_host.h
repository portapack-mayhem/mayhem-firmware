/*
 * Copyright (C) 2023 Bernd Herzog
 * Copyright (C) 2024 HTotoo
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

#ifndef __I2C_DEVICE_TO_HOST_H
#define __I2C_DEVICE_TO_HOST_H

#include "ch.h"
#include "hal.h"

#ifndef I2CSHELL_BUFFERS_SIZE
#define I2CSHELL_BUFFERS_SIZE 64
#endif

struct I2CShellDriverVMT {
    _base_asynchronous_channel_methods
};

struct I2CShellDriver {
    /** @brief Virtual Methods Table.*/
    const struct I2CShellDriverVMT* vmt;
    InputQueue iqueue;                 /* Output queue.*/
    OutputQueue oqueue;                /* Input circular buffer.*/
    uint8_t ib[I2CSHELL_BUFFERS_SIZE]; /* Output circular buffer.*/
    uint8_t ob[I2CSHELL_BUFFERS_SIZE];
};

typedef struct I2CShellDriver I2CShellDriver;

extern I2CShellDriver I2CD1;

#ifdef __cplusplus
extern "C" {
#endif
void init_i2c_shell_driver(I2CShellDriver* sdp);
#ifdef __cplusplus
}
#endif

#endif
