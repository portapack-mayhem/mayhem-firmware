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

#ifndef __USB_SD_OVER_USB_H__
#define __USB_SD_OVER_USB_H__

#include <stddef.h>
#include <common/usb.h>
#include <common/usb_request.h>
#include <common/usb_standard_request.h>
#include <hackrf_usb/usb_device.h>
#include <hackrf_usb/usb_endpoint.h>
#include <libopencm3/lpc43xx/m4/nvic.h>
#include <libopencm3/lpc43xx/cgu.h>
#include "platform_detect.h"
#include "hackrf_core.h"
#include "usb_bulk_buffer.h"

void start_usb(void);
void stop_usb(void);
void irq_usb(void);
void usb_transfer(void);

#endif /* __USB_SD_OVER_USB_H__ */