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

#include <usb_type.h>
#include <usb_queue.h>

#define USB_CONTROL_IN_EP_ADDR (0x80)
#define USB_CONTROL_OUT_EP_ADDR (0x00)

#define USB_INT_IN_EP_ADDR (0x82)

#define USB_BULK_OUT_EP_ADDR (0x01)
#define USB_BULK_IN_EP_ADDR (0x81)

extern usb_endpoint_t usb_endpoint_control_out;
extern USB_DECLARE_QUEUE(usb_endpoint_control_out);

extern usb_endpoint_t usb_endpoint_control_in;
extern USB_DECLARE_QUEUE(usb_endpoint_control_in);

extern usb_endpoint_t usb_endpoint_int_in;
extern USB_DECLARE_QUEUE(usb_endpoint_int_in);

extern usb_endpoint_t usb_endpoint_bulk_in;
extern USB_DECLARE_QUEUE(usb_endpoint_bulk_in);

extern usb_endpoint_t usb_endpoint_bulk_out;
extern USB_DECLARE_QUEUE(usb_endpoint_bulk_out);
