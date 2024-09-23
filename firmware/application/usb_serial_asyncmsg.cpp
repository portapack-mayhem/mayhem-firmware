/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
 * Copyleft (ɔ) 2024 zxkmm with the GPL license
 * Copyright (C) 2024 HTotoo
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

#include "usb_serial_asyncmsg.hpp"

/// value
// to_string_bin/ to_string_decimal/ to_string_hex/ to_string_hex_array/ to_string_dec_uint/ to_string_dec_int etc seems usellss so i didn't add them here
// usage:     UsbSerialAsyncmsg::asyncmsg(num);

template <>
void UsbSerialAsyncmsg::asyncmsg<int64_t>(const int64_t& data) {
    if (!portapack::async_tx_enabled) {
        return;
    }
    chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", to_string_dec_int(data).c_str());
}

template <>
void UsbSerialAsyncmsg::asyncmsg<int32_t>(const int32_t& data) {
    if (!portapack::async_tx_enabled) {
        return;
    }
    chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", to_string_dec_int(data).c_str());
}

template <>
void UsbSerialAsyncmsg::asyncmsg<int16_t>(const int16_t& data) {
    if (!portapack::async_tx_enabled) {
        return;
    }
    chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", to_string_dec_int(data).c_str());
}

template <>
void UsbSerialAsyncmsg::asyncmsg<int8_t>(const int8_t& data) {
    if (!portapack::async_tx_enabled) {
        return;
    }
    chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", to_string_dec_int(data).c_str());
}

template <>
void UsbSerialAsyncmsg::asyncmsg<uint8_t>(const uint8_t& data) {
    if (!portapack::async_tx_enabled) {
        return;
    }
    chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", to_string_dec_int(data).c_str());
}

template <>
void UsbSerialAsyncmsg::asyncmsg<uint16_t>(const uint16_t& data) {
    if (!portapack::async_tx_enabled) {
        return;
    }
    chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", to_string_dec_int(data).c_str());
}

template <>
void UsbSerialAsyncmsg::asyncmsg<uint32_t>(const uint32_t& data) {
    if (!portapack::async_tx_enabled) {
        return;
    }
    chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", to_string_dec_int(data).c_str());
}

template <>
void UsbSerialAsyncmsg::asyncmsg<uint64_t>(const uint64_t& data) {
    if (!portapack::async_tx_enabled) {
        return;
    }
    chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", to_string_dec_int(data).c_str());
}

template <>
void UsbSerialAsyncmsg::asyncmsg<float>(const float& data) {
    if (!portapack::async_tx_enabled) {
        return;
    }
    chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", to_string_decimal(data, 7).c_str());
}

/// fs things

template <>
// usage:     UsbSerialAsyncmsg::asyncmsg(path);
void UsbSerialAsyncmsg::asyncmsg<std::filesystem::path>(const std::filesystem::path& data) {
    if (!portapack::async_tx_enabled) {
        return;
    }
    std::string path_str = data.string();
    chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", path_str.c_str());
}

template <>
void UsbSerialAsyncmsg::asyncmsg<std::filesystem::path::string_type>(const std::filesystem::path::string_type& data) {
    if (!portapack::async_tx_enabled) {
        return;
    }
    std::string str_data(data.begin(), data.end());
    chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", str_data.c_str());
}

/// string

// string obj
template <>
// usage:     UsbSerialAsyncmsg::asyncmsg(str);
void UsbSerialAsyncmsg::asyncmsg<std::string>(const std::string& data) {
    if (!portapack::async_tx_enabled) {
        return;
    }
    chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", data.c_str());
}

// string literal AKA char[]
// usage: UsbSerialAsyncmsg::asyncmsg("abc");
void UsbSerialAsyncmsg::asyncmsg(const char* data) {
    if (!portapack::async_tx_enabled) {
        return;
    }
    chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", data);
}

/// bool
template <>
// usage:     UsbSerialAsyncmsg::asyncmsg(true);
void UsbSerialAsyncmsg::asyncmsg<bool>(const bool& data) {
    if (!portapack::async_tx_enabled) {
        return;
    }
    chprintf((BaseSequentialStream*)&SUSBD1, "%s\r\n", data ? "true" : "false");
}