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

#include "sd_over_usb.h"
#include "scsi.h"

volatile bool scsi_running = false;

usb_request_status_t report_max_lun(
    usb_endpoint_t* const endpoint,
    const usb_transfer_stage_t stage) {
    if (stage == USB_TRANSFER_STAGE_SETUP) {
        endpoint->buffer[0] = 0;
        usb_transfer_schedule_block(
            endpoint->in,
            &endpoint->buffer,
            1,
            NULL,
            NULL);
    } else if (stage == USB_TRANSFER_STAGE_DATA) {
        usb_transfer_schedule_ack(endpoint->out);

        scsi_running = true;
    }

    return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_class_request(usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage) {
    usb_request_status_t status = USB_REQUEST_STATUS_STALL;

    volatile uint8_t request = endpoint->setup.request;

    if (request == 0xFE)
        return report_max_lun(endpoint, stage);

    return status;
}

const usb_request_handlers_t usb_request_handlers = {
    .standard = usb_standard_request,
    .class = usb_class_request,
    .vendor = 0,
    .reserved = 0};

void usb_configuration_changed(usb_device_t* const device) {
    (void)device;

    usb_endpoint_init(&usb_endpoint_bulk_in);
    usb_endpoint_init(&usb_endpoint_bulk_out);
}

void start_usb(void) {
    detect_hardware_platform();
    pin_setup();
    cpu_clock_init();

    usb_set_configuration_changed_cb(usb_configuration_changed);
    usb_peripheral_reset();

    usb_device_init(0, &usb_device);

    usb_queue_init(&usb_endpoint_control_out_queue);
    usb_queue_init(&usb_endpoint_control_in_queue);
    usb_queue_init(&usb_endpoint_bulk_out_queue);
    usb_queue_init(&usb_endpoint_bulk_in_queue);

    usb_endpoint_init(&usb_endpoint_control_out);
    usb_endpoint_init(&usb_endpoint_control_in);

    nvic_set_priority(NVIC_USB0_IRQ, 255);

    usb_run(&usb_device);
}

void stop_usb(void) {
    usb_peripheral_reset();
}

void irq_usb(void) {
    usb0_isr();
}

volatile bool transfer_complete = false;
void scsi_bulk_transfer_complete(void* user_data, unsigned int bytes_transferred) {
    (void)user_data;
    (void)bytes_transferred;

    transfer_complete = true;
}

void usb_transfer(void) {
    if (scsi_running) {
        transfer_complete = false;
        usb_transfer_schedule_block(
            &usb_endpoint_bulk_out,
            &usb_bulk_buffer[0x4000],
            USB_TRANSFER_SIZE,
            scsi_bulk_transfer_complete,
            NULL);

        while (!transfer_complete)
            ;

        msd_cbw_t* msd_cbw_data = (msd_cbw_t*)&usb_bulk_buffer[0x4000];

        if (msd_cbw_data->signature == MSD_CBW_SIGNATURE) {
            scsi_command(msd_cbw_data);
        }
    }
}