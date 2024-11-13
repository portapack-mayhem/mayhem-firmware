/*
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

#include "flipper_subfile.hpp"

#include "convert.hpp"
#include "file_reader.hpp"
#include "string_format.hpp"
#include <string_view>

namespace fs = std::filesystem;
using namespace std::literals;

const std::string filetype_name = "Filetype";
const std::string frequency_name = "Frequency";
const std::string latitude_name_old = "Latitute";
const std::string longitude_name_old = "Longitude";
const std::string latitude_name = "Lat";
const std::string longitude_name = "Lon";
const std::string protocol_name = "Protocol";
const std::string preset_name = "Preset";
const std::string te_name = "TE";          // only in BinRAW
const std::string bit_count_name = "Bit";  // for us, only in BinRAW

/*
Filetype: Flipper SubGhz Key File
Version: 1
Frequency: 433920000
Preset: FuriHalSubGhzPresetOok650Async
Latitute: nan
Longitude: nan
Protocol: BinRAW
Bit: 1730
TE: 495
Bit_RAW: 1730
Data_RAW: 02 10 84

te: is the quantization interval, in us.
bit: all bit counts in file.
bit_raw: the bits stored in the next data_raw. this 2 can repeat
data_raw: is an encoded sequence of durations, where each bit in the sequence encodes one TE interval: 1 - high level (there is a carrier), 0 - low (no carrier). For example, TE=100, Bit_RAW=8, Data_RAW=0x37 => 0b00110111, that is, -200 200 -100 300 will be transmitted



Filetype: Flipper SubGhz RAW File
Version: 1
Frequency: 433920000
Preset: FuriHalSubGhzPresetOok650Async
Protocol: RAW
RAW_Data: 5832 -12188 130 -162

raw_data- positive: carrier for n time, negative: no carrier for n time. (us)
*/

Optional<flippersub_metadata> read_flippersub_file(const fs::path& path) {
    File f;
    auto error = f.open(path);
    if (error)
        return {};
    flippersub_metadata metadata{};

    char ch = 0;
    std::string line = "";
    auto fr = f.read(&ch, 1);
    while (!fr.is_error() && fr.value() > 0) {
        if (line.length() < 130 && ch != '\n') line += ch;
        if (ch != '\n') {
            fr = f.read(&ch, 1);
            continue;
        }
        auto it = line.find(':', 0);
        if (it == std::string::npos) {
            fr = f.read(&ch, 1);
            continue;  // Bad line.
        }
        std::string fixed = line.data() + it + 1;
        fixed = trim(fixed);
        std::string head = line.substr(0, it);
        line = "";

        if (fixed.length() <= 1) {
            fr = f.read(&ch, 1);
            continue;
        }

        if (head == filetype_name) {
            if (fixed != "Flipper SubGhz Key File" && fixed != "Flipper SubGhz RAW File") return {};  // not supported
        } else if (head == frequency_name)
            parse_int(fixed, metadata.center_frequency);
        else if (head == latitude_name)
            parse_float_meta(fixed, metadata.latitude);
        else if (head == longitude_name)
            parse_float_meta(fixed, metadata.longitude);
        else if (head == latitude_name_old)
            parse_float_meta(fixed, metadata.latitude);
        else if (head == longitude_name_old)
            parse_float_meta(fixed, metadata.longitude);
        else if (head == protocol_name) {
            if (fixed == "RAW") metadata.protocol = FLIPPER_PROTO_RAW;
            if (fixed == "BinRAW") metadata.protocol = FLIPPER_PROTO_BINRAW;
        } else if (head == te_name) {
            metadata.te = atoi(fixed.c_str());
        } else if (head == bit_count_name) {
            metadata.binraw_bit_count = atol(fixed.c_str());
        } else if (head == preset_name) {
            if (fixed.find("FSK") != std::string::npos) {
                metadata.preset = FLIPPER_PRESET_2FSK;
            } else if (fixed.find("Ook") != std::string::npos) {
                metadata.preset = FLIPPER_PRESET_OOK;
            } else if (fixed.find("Custom") != std::string::npos) {
                metadata.preset = FLIPPER_PRESET_CUSTOM;
            }
        }
        fr = f.read(&ch, 1);
    }
    f.close();
    if (metadata.center_frequency == 0) return {};  // Parse failed.

    return metadata;
}

bool seek_flipper_raw_first_data(File& f) {
    f.seek(0);
    std::string chs = "";
    char ch;
    while (f.read(&ch, 1)) {
        if (ch == '\r') continue;
        if (ch == '\n') {
            chs = "";
            continue;
        };
        chs += ch;
        if (ch == 0) break;
        if (chs == "RAW_Data: ") {
            return true;
        }
    }
    return false;
}
bool seek_flipper_binraw_first_data(File& f, bool seekzero) {
    if (seekzero) f.seek(0);
    std::string chs = "";
    char ch;
    while (f.read(&ch, 1)) {
        if (ch == '\r') continue;
        if (ch == '\n') {
            chs = "";
            continue;
        };
        if (ch == 0) break;
        chs += ch;
        if (chs == "Data_RAW: ") {
            return true;
        }
    }
    return false;
}

Optional<int32_t> read_flipper_raw_next_data(File& f) {
    // RAW_Data: 5832 -12188 130 -162
    std::string chs = "";
    char ch = 0;
    while (f.read(&ch, 1).is_ok()) {
        if (ch == '\r') continue;  // should not present
        if ((ch == ' ') || ch == '\n') {
            if (chs == "RAW_Data:") {
                chs = "";
                continue;
            }
            break;
        };
        if (ch == 0) break;
        chs += ch;
    }
    if (chs == "") return {};
    return atol(chs.c_str());
}

Optional<uint8_t> read_flipper_binraw_next_data(File& f) {
    // Data_RAW: 02 10 84 BUT THERE ARE  Bit_RAW lines to skip!
    std::string chs = "";
    char ch = 0;
    while (f.read(&ch, 1)) {
        if (ch == '\r') continue;  // should not present
        if ((ch == ' ') || ch == '\n') {
            if (chs == "RAW_Data:") {
                chs = "";
                continue;
            }
            break;
        };
        if (ch == 0) break;
        chs += ch;
    }
    if (chs == "") return {};
    return static_cast<uint8_t>(std::stoul(chs, nullptr, 16));
}

bool get_flipper_binraw_bitvalue(uint8_t byte, uint8_t nthBit) {
    return (byte & (1 << nthBit)) != 0;
}