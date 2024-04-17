/*
* Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
* Copyright (C) 2017 Furrtek
* Copyleft (ɔ) 2024 zxkmm with the GPL license
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

#ifndef MAYHEM_FIRMWARE_USB_SERIAL_DEBUG_BRIDGE_H
#define MAYHEM_FIRMWARE_USB_SERIAL_DEBUG_BRIDGE_H

#include <vector>
#include <string>
#include <sstream>
#include <chprintf.h>
#include "usb_serial_device_to_host.h"

class UsbSerialDebugBridge {

   public:
    static void ppdbg(const std::string& data) {
        std::ostringstream oss;
        oss << data;
        chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", oss.str().c_str());
    }

    static void ppdbg(const std::vector<std::string>& data) {
        std::ostringstream oss;
        for (const auto& item : data) {
            oss << item << ' ';
        }
        chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", oss.str().c_str());
    }

};


#endif  // MAYHEM_FIRMWARE_USB_SERIAL_DEBUG_BRIDGE_H