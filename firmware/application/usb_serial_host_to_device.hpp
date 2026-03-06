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

#define USB_BULK_BUFFER_SIZE 64

void init_host_to_device();
void reset_transfer_queues();
void serial_bulk_transfer_complete(void* user_data, unsigned int bytes_transferred);
void schedule_host_to_device_transfer();
void complete_host_to_device_transfer();

typedef void (*kiss_raw_handler_t)(const uint8_t* data, size_t len);

/**
 * Register a handler for raw bytes received over the USB bulk endpoint.
 *
 * When set, all incoming USB serial bytes are routed to this handler instead
 * of the normal shell iqueue. The handler:
 * - Is called from the USB transfer completion context (main event loop thread).
 * - Must return quickly; long or blocking work will stall the USB event loop.
 * - Receives a pointer into an internal reusable USB bulk buffer; the pointer
 *   is only valid for the duration of the call and must not be retained.
 *
 * Pass nullptr to restore normal shell routing.
 */
void set_kiss_raw_handler(kiss_raw_handler_t handler);

#endif
