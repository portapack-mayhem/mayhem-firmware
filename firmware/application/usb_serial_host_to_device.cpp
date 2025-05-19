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

#include "usb_serial_host_to_device.hpp"
#include "event_m0.hpp"
#include "usb_serial_device_to_host.h"

extern "C" {
#include <common/usb.h>
#include <hackrf_usb/usb_device.h>
#include <hackrf_usb/usb_endpoint.h>
}

#include <queue>
#include <vector>

static Thread* thread_usb_event = NULL;

struct usb_bulk_buffer_t {
    uint8_t* data;
    size_t length;
    bool completed;
};

std::queue<usb_bulk_buffer_t*> usb_bulk_buffer_queue;
std::queue<usb_bulk_buffer_t*> usb_bulk_buffer_spare;

void serial_bulk_transfer_complete(void* user_data, unsigned int bytes_transferred) {
    usb_bulk_buffer_t* transfer_data = (usb_bulk_buffer_t*)user_data;

    transfer_data->length = bytes_transferred;
    transfer_data->completed = true;
}

void init_host_to_device() {
    thread_usb_event = chThdSelf();
}

void reset_transfer_queues() {
    while (usb_bulk_buffer_queue.empty() == false)
        usb_bulk_buffer_queue.pop();

    while (usb_bulk_buffer_spare.empty() == false)
        usb_bulk_buffer_spare.pop();
}

void schedule_host_to_device_transfer() {
    if (usb_bulk_buffer_queue.size() >= 8)
        return;

    static usb_bulk_buffer_t* transfer_data = nullptr;

    int ret;

    do {
        if (transfer_data == nullptr) {
            if (usb_bulk_buffer_spare.empty() == false) {
                transfer_data = usb_bulk_buffer_spare.front();
                transfer_data->length = 0;
                transfer_data->completed = false;
                usb_bulk_buffer_spare.pop();
            } else {
                transfer_data = new usb_bulk_buffer_t{
                    .data = new uint8_t[USB_BULK_BUFFER_SIZE],
                    .length = 0,
                    .completed = false};
            }
        }

        ret = usb_transfer_schedule(
            &usb_endpoint_bulk_out,
            transfer_data->data,
            USB_BULK_BUFFER_SIZE,
            serial_bulk_transfer_complete,
            transfer_data);

        if (ret != -1) {
            usb_bulk_buffer_queue.push(transfer_data);
            transfer_data = nullptr;

            if (usb_bulk_buffer_queue.size() >= 8)
                return;
        }
    } while (ret != -1);
}

void complete_host_to_device_transfer() {
    for (; !usb_bulk_buffer_queue.empty(); usb_bulk_buffer_queue.pop()) {
        usb_bulk_buffer_t* transfer_data = usb_bulk_buffer_queue.front();

        while (transfer_data->completed == false)
            return;

        chSysLock();
        for (unsigned int i = 0; i < transfer_data->length; i++) {
            msg_t ret;
            do {
                ret = chIQPutI(&SUSBD1.iqueue, transfer_data->data[i]);

                if (ret == Q_FULL) {
                    chSysUnlock();
                    chThdSleepMilliseconds(1);  // wait for shell thread when buffer is full
                    chSysLock();
                }

            } while (ret == Q_FULL);
        }
        chSysUnlock();

        usb_bulk_buffer_spare.push(transfer_data);
    }
}
