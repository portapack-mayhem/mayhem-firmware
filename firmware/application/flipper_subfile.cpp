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

const std::string_view filetype_name = "Filetype"sv;
const std::string_view frequency_name = "Frequency"sv;
const std::string_view latitude_name = "Latitute"sv;
const std::string_view longitude_name = "Longitude"sv;
const std::string_view protocol_name = "Protocol"sv;
const std::string_view preset_name = "Preset"sv;
const std::string_view te_name = "TE"sv;  // only in BinRAW

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

    auto reader = FileLineReader(f);
    for (const auto& line : reader) {
        auto cols = split_string(line, ':');

        if (cols.size() != 2)
            continue;  // Bad line.
        if (cols[1].length() <= 1) continue;
        std::string fixed = cols[1].data() + 1;
        fixed = trim(fixed);
        if (cols[0] == filetype_name) {
            if (fixed != "Flipper SubGhz Key File" && fixed != "Flipper SubGhz RAW File") return {};  // not supported
        } else if (cols[0] == frequency_name)
            parse_int(fixed, metadata.center_frequency);
        else if (cols[0] == latitude_name)
            parse_float_meta(fixed, metadata.latitude);
        else if (cols[0] == longitude_name)
            parse_float_meta(fixed, metadata.longitude);
        else if (cols[0] == protocol_name) {
            if (fixed == "RAW") metadata.protocol = FLIPPER_PROTO_RAW;
            if (fixed == "BinRAW") metadata.protocol = FLIPPER_PROTO_BINRAW;
        } else if (cols[0] == te_name) {
            metadata.te = atoi(fixed.c_str());
        } else if (cols[0] == preset_name) {
            if (fixed.find("FSK") != std::string::npos) {
                metadata.preset = FLIPPER_PRESET_2FSK;
            } else if (fixed.find("Ook") != std::string::npos) {
                metadata.preset = FLIPPER_PRESET_OOK;
            } else if (fixed.find("Custom") != std::string::npos) {
                metadata.preset = FLIPPER_PRESET_CUSTOM;
            }

        } else
            continue;
    }

    if (metadata.center_frequency == 0) return {};  // Parse failed.

    return metadata;
}
