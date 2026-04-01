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

#ifndef __USB_SERIAL_HOST_TO_DEVICE_H
#define __USB_SERIAL_HOST_TO_DEVICE_H

#include "ch.h"
#include "hal.h"
#include "usb_serial_device_to_host.h"

#define USB_BULK_BUFFER_SIZE 64

void init_host_to_device();
void reset_transfer_queues();
void serial_bulk_transfer_complete(void* user_data, unsigned int bytes_transferred);
void schedule_host_to_device_transfer();
void complete_host_to_device_transfer();

typedef void (*usb_serial_input_handler_t)(const uint8_t* data, size_t len);

/**
 * Global storing the currently active USB serial input handler.
 * When non-null, all incoming USB bytes are routed to this handler
 * instead of the normal shell iqueue.
 *
 * Managed via UsbSerialInputHandler RAII below — do not write directly.
 */
extern usb_serial_input_handler_t usb_serial_active_input_handler;

/**
 * RAII wrapper that registers a USB serial input handler on construction
 * and automatically deregisters it on destruction.
 *
 * Only one handler may be active at a time.
 *
 * The handler is called from the USB transfer completion context (main event
 * loop thread). It must return quickly; blocking will stall USB/UI servicing.
 * The data pointer is only valid for the duration of the call.
 *
 * Use write() to send data to the host. By default bytes are dropped if the
 * TX queue is full (TIME_IMMEDIATE); pass a timeout in ticks to block instead.
 */
class UsbSerialInputHandler {
   public:
    UsbSerialInputHandler() = delete;
    explicit UsbSerialInputHandler(usb_serial_input_handler_t handler) {
        usb_serial_active_input_handler = handler;
    }
    ~UsbSerialInputHandler() {
        usb_serial_active_input_handler = nullptr;
    }
    UsbSerialInputHandler(const UsbSerialInputHandler&) = delete;
    UsbSerialInputHandler& operator=(const UsbSerialInputHandler&) = delete;

    // Send bytes to the USB host. timeout defaults to TIME_IMMEDIATE (drop if full).
    void write(const uint8_t* data, size_t len, systime_t timeout = TIME_IMMEDIATE) {
        chOQWriteTimeout(&SUSBD1.oqueue, data, len, timeout);
    }
};

#endif
