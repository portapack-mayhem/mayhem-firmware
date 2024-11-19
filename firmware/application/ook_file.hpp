/*
 * Copyright (C) 2024 gullradriel
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

#ifndef __OOK_FILE_HPP__
#define __OOK_FILE_HPP__

#include "portapack.hpp"
#include "metadata_file.hpp"
#include "encoders.hpp"  // Includes data encoding functions for transmission
#include "convert.hpp"
#include "file_reader.hpp"
#include "string_format.hpp"
#include <string_view>
#include "transmitter_model.hpp"
#include "baseband_api.hpp"  // Includes baseband API for handling transmission settings

struct ook_file_data {
    rf::Frequency frequency = rf::Frequency(0);  // Default frequency
    uint32_t sample_rate = 0;                    // Default sample rate
    uint16_t symbol_rate = 0;                    // Default bit duration
    uint16_t pause_symbol_duration = 0;          // Default pause between repeat
    uint16_t repeat = 0;                         // Default repeat
    std::string payload = "";                    // Default payload
};

bool read_ook_file(const std::filesystem::path& path, ook_file_data& ook_data);
bool save_ook_file(ook_file_data& data, const std::filesystem::path& path);
void start_ook_file_tx(ook_file_data& ook_data);
void stop_ook_file_tx();

#endif  // __FLIPPER_SUBFILE_HPP__
