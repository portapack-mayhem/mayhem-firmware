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

/*Notes:
 * - Don't use MayhemHub since it currently not support real time serial output
 * - If you don't use this class linker will drop it so it won't use any space
 * - so delete all debug things before you push your code to production
 * - use this client to filter only PP devices: https://github.com/zxkmm/Pyserial-Demo-portapack
 * - usage:
 *        #include "usb_serial_debug_bridge.hpp"
 *        UsbSerialDebugBridge::ppdbg("Hello PP");
 * */

/// value
// to_string_bin/ to_string_decimal/ to_string_hex/ to_string_hex_array/ to_string_dec_uint/ to_string_dec_int etc seems usellss so i didn't add them here

template <>
// usage:     UsbSerialDebugBridge::ppdbg(num);
void UsbSerialDebugBridge::ppdbg<int64_t>(const int64_t& data) {
    chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", to_string_dec_int(data).c_str());
}

template <>
void UsbSerialDebugBridge::ppdbg<int32_t>(const int32_t& data) {
    chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", to_string_dec_int(data).c_str());
}

template <>
void UsbSerialDebugBridge::ppdbg<int16_t>(const int16_t& data) {
    chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", to_string_dec_int(data).c_str());
}

template <>
void UsbSerialDebugBridge::ppdbg<int8_t>(const int8_t& data) {
    chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", to_string_dec_int(data).c_str());
}

template <>
void UsbSerialDebugBridge::ppdbg<uint8_t>(const uint8_t& data) {
    chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", to_string_dec_int(data).c_str());
}

template <>
void UsbSerialDebugBridge::ppdbg<uint16_t>(const uint16_t& data) {
    chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", to_string_dec_int(data).c_str());
}

template <>
void UsbSerialDebugBridge::ppdbg<uint32_t>(const uint32_t& data) {
    chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", to_string_dec_int(data).c_str());
}

template <>
void UsbSerialDebugBridge::ppdbg<uint64_t>(const uint64_t& data) {
    chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", to_string_dec_int(data).c_str());
}

/// fs things

template <>
// usage:     UsbSerialDebugBridge::ppdbg(path);
void UsbSerialDebugBridge::ppdbg<std::filesystem::path>(const std::filesystem::path& data) {
    std::string path_str = data.string();
    chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", path_str.c_str());
}

/// string
template <>
// usage:     UsbSerialDebugBridge::ppdbg(str);
void UsbSerialDebugBridge::ppdbg<std::string>(const std::string& data) {
    chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", data.c_str());
}

/// vec worker
// ussgae:    UsbSerialDebugBridge::ppdbg(vec);
template <typename VECTORCOVER>
void UsbSerialDebugBridge::ppdbg(const std::vector<VECTORCOVER>& data) {
    for (const auto& item : data) {
        ppdbg(item);
    }
}

#endif  // USB_SERIAL_DEBUG_BRIDGE_H
