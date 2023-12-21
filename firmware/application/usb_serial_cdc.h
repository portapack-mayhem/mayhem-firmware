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

#ifndef __cplusplus
#pragma GCC diagnostic push
// external code, so ignore warnings
#pragma GCC diagnostic ignored "-Wstrict-prototypes"

#include <common/usb.h>
#include <common/usb_request.h>
#include <common/usb_standard_request.h>
#include <hackrf_usb/usb_device.h>
#include <hackrf_usb/usb_endpoint.h>

#pragma GCC diagnostic pop

usb_request_status_t usb_class_request(usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage);
usb_request_status_t usb_get_line_coding_request(usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage);
usb_request_status_t usb_set_control_line_state_request(usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage);
usb_request_status_t usb_set_line_coding_request(usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage);
#endif

void setup_usb_serial_controller(void);
