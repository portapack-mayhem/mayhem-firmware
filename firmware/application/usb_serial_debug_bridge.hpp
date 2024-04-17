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

#ifndef USB_SERIAL_DEBUG_BRIDGE_H
#define USB_SERIAL_DEBUG_BRIDGE_H

#include <vector>
#include <string>
#include <sstream>
#include <chprintf.h>
#include "usb_serial_device_to_host.h"

class UsbSerialDebugBridge {
   public:
    template <typename STRINGCOVER>
    static void ppdbg(const STRINGCOVER& data);

    template <typename VECTORCOVER>
    static void ppdbg(const std::vector<VECTORCOVER>& data);
};

template <>
void UsbSerialDebugBridge::ppdbg<int64_t>(const int64_t& data) {
    chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", to_string_dec_int(data).c_str());
}

template <>
void UsbSerialDebugBridge::ppdbg<uint64_t>(const uint64_t& data) {
    chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", to_string_dec_uint(data).c_str());
}

template <>
void UsbSerialDebugBridge::ppdbg<std::filesystem::path>(const std::filesystem::path& data) {
    chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", data.c_str());
}


template <typename T>
void UsbSerialDebugBridge::ppdbg(const std::vector<T>& data) {
    for (const auto& item : data) {
        ppdbg(item);
    }
}

#endif  // USB_SERIAL_DEBUG_BRIDGE_H
