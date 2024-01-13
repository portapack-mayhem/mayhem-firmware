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

#pragma once

#include "ch.h"
#include "hal.h"

#define USB_BULK_BUFFER_SIZE 64

#ifndef USBSERIAL_BUFFERS_SIZE
#define USBSERIAL_BUFFERS_SIZE 128
#endif

struct SerialUSBDriverVMT {
    _base_asynchronous_channel_methods
};

struct SerialUSBDriver {
    /** @brief Virtual Methods Table.*/
    const struct SerialUSBDriverVMT* vmt;
    InputQueue iqueue;                  /* Output queue.*/
    OutputQueue oqueue;                 /* Input circular buffer.*/
    uint8_t ib[USBSERIAL_BUFFERS_SIZE]; /* Output circular buffer.*/
    uint8_t ob[USBSERIAL_BUFFERS_SIZE];
};

typedef struct SerialUSBDriver SerialUSBDriver;

extern SerialUSBDriver SUSBD1;

void init_serial_usb_driver(SerialUSBDriver* sdp);

#ifdef __cplusplus
extern "C" {
#endif
size_t fillOBuffer(OutputQueue* oqp, const uint8_t* bp, size_t n);
#ifdef __cplusplus
}
#endif
