/*
 * Copyright (C) 2023 Kyle Reed
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

#include "screenshot_reader.hpp"
#include <cstdio>

/* PNG Header magic numbers. */
static constexpr uint64_t png_header = 0x0A1A0A0D'474E5089;
static constexpr uint64_t ihdr_dimensions = 0x40010000'F0000000;

/* This is not a rigorous PNG parser. Certain values are sniffed along
 * the way to verify this is output from the PNGWriter. Many important
 * fields are ignored, e.g. checksums. */

bool ScreenshotReader::parse(const uint8_t* bytes, uint32_t size) {
    for (auto i = 0u; i < size && state_ != State::Error; ++i) {
        uint8_t b = bytes[i];

        switch (state_) {
            case State::PngHdr:  // 8 bytes.
                if (pos_ < 8)
                    scratch_[scratch_index_++] = b;
                else {
                    // Does it have the PNG header?
                    auto header = scratch_as<uint64_t>();
                    state_ = header == png_header ? State::IHdr : State::Error;
                    scratch_index_ = 0;
                }
                break;

            case State::IHdr:  // 25 bytes.
                if (pos_ > 15 && pos_ < 24)
                    scratch_[scratch_index_++] = b;
                else if (pos_ == 24) {
                    // Are the dimensions right?
                    auto dimensions = scratch_as<uint64_t>();
                    if (dimensions != ihdr_dimensions) {
                        state_ == State::Error;
                        break;
                    }
                } else if (pos_ == 32)
                    state_ = State::ChunkHdr;
                break;

            case State::ChunkHdr:  // 10 bytes
                if (pos_ == 42) {
                    state_ = State::LineHdr;
                    scratch_index_ = 0;
                }
                break;

            case State::LineHdr:  // 6 bytes
                if (scratch_index_++ == 5) {
                    state_ = State::LineData;
                    scratch_index_ = 0;
                }
                break;

            case State::LineData:  // 720 bytes
                set_line_byte(scratch_index_++, b);
                if (scratch_index_ == 720) {
                    if (on_line) on_line(line_);
                    ++line_count_;
                    scratch_index_ = 0;

                    state_ = line_count_ < 320 ? State::LineHdr : state_ = State::End;
                }
                break;

            case State::End:
                return true;

            default:
                printf("Default case!\n");
                state_ = State::Error;
                break;
        };

        ++pos_;
    }

    return state_ != State::Error;
}