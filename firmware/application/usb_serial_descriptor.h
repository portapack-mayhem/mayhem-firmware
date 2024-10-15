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

#ifndef __USB_SERIAL_DESCRIPTOR_H
#define __USB_SERIAL_DESCRIPTOR_H

#include "ch.h"
#include "hal.h"

extern uint8_t usb_descriptor_device[];
extern uint8_t usb_descriptor_device_qualifier[];
extern uint8_t usb_descriptor_configuration_full_speed[];
extern uint8_t usb_descriptor_configuration_high_speed[];
extern uint8_t usb_descriptor_string_languages[];
extern uint8_t usb_descriptor_string_manufacturer[];
extern uint8_t usb_descriptor_string_product[];

#define USB_DESCRIPTOR_STRING_SERIAL_LEN 32
#define USB_DESCRIPTOR_STRING_SERIAL_BUF_LEN \
    (USB_DESCRIPTOR_STRING_SERIAL_LEN * 2 + 2) /* UTF-16LE */
extern uint8_t usb_descriptor_string_serial_number[];

extern uint8_t* usb_descriptor_strings[];

#define USB_WCID_VENDOR_REQ 0x19
extern uint8_t wcid_string_descriptor[];
extern uint8_t wcid_feature_descriptor[];

#endif
