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

#include "usb_serial_endpoints.h"

#pragma GCC diagnostic push
// external code, so ignore warnings
#pragma GCC diagnostic ignored "-Wstrict-prototypes"

#include <common/usb.h>
#include <common/usb_request.h>
#include <common/usb_standard_request.h>
#include <hackrf_usb/usb_device.h>
#include <hackrf_usb/usb_endpoint.h>

#pragma GCC diagnostic pop

usb_endpoint_t usb_endpoint_control_out = {
    .address = USB_CONTROL_OUT_EP_ADDR,
    .device = &usb_device,
    .in = &usb_endpoint_control_in,
    .out = &usb_endpoint_control_out,
    .setup_complete = usb_setup_complete,
    .transfer_complete = usb_control_out_complete,
};
USB_DEFINE_QUEUE(usb_endpoint_control_out, 4);

usb_endpoint_t usb_endpoint_control_in = {
    .address = USB_CONTROL_IN_EP_ADDR,
    .device = &usb_device,
    .in = &usb_endpoint_control_in,
    .out = &usb_endpoint_control_out,
    .setup_complete = 0,
    .transfer_complete = usb_control_in_complete,
};
static USB_DEFINE_QUEUE(usb_endpoint_control_in, 4);

usb_endpoint_t usb_endpoint_int_in = {
    .address = USB_INT_IN_EP_ADDR,
    .device = &usb_device,
    .in = &usb_endpoint_int_in,
    .out = 0,
    .setup_complete = 0,
    .transfer_complete = usb_queue_transfer_complete};
static USB_DEFINE_QUEUE(usb_endpoint_int_in, 1);

usb_endpoint_t usb_endpoint_bulk_in = {
    .address = USB_BULK_IN_EP_ADDR,
    .device = &usb_device,
    .in = &usb_endpoint_bulk_in,
    .out = 0,
    .setup_complete = 0,
    .transfer_complete = usb_queue_transfer_complete};
static USB_DEFINE_QUEUE(usb_endpoint_bulk_in, 1);

usb_endpoint_t usb_endpoint_bulk_out = {
    .address = USB_BULK_OUT_EP_ADDR,
    .device = &usb_device,
    .in = 0,
    .out = &usb_endpoint_bulk_out,
    .setup_complete = 0,
    .transfer_complete = usb_queue_transfer_complete};
static USB_DEFINE_QUEUE(usb_endpoint_bulk_out, 1);
