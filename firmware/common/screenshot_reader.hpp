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

#ifndef __SCREENSHOT_READER_H__
#define __SCREENSHOT_READER_H__

#include "ui.hpp"
#include <array>
#include <functional>

/* An event based screenshot (PNG) parser. */
/* NB: Not a generic PNG reader. This will only read
 * the files created by PNGWriter. */
class ScreenshotReader {
   public:
    static constexpr uint32_t line_size = 240;
    static constexpr uint8_t scratch_size = 8;
    using Scanline = std::array<ui::ColorRGB888, line_size>;

    std::function<void(const Scanline&)> on_line{};

    /* Accepts new data to parse. Returns true if the data so far is valid. */
    bool parse(const uint8_t* bytes, uint32_t size);

   private:
    enum class State : uint8_t {
        Error,
        PngHdr,
        IHdr,
        ChunkHdr,
        LineHdr,
        LineData,
        End,
    };

    template <typename T>
    T scratch_as() const {
        return *(T*)scratch_;
    }

    void set_line_byte(uint32_t offset, uint8_t value) {
        auto start = (uint8_t*)&line_[0];
        *(start + offset) = value;
    }

    State state_{State::PngHdr};
    uint32_t line_count_{};
    uint32_t pos_{};

    /* Scratch buffer for temporary processing. */
    uint8_t scratch_[scratch_size]{};
    uint32_t scratch_index_{};

    /* Scanline buffer. */
    Scanline line_{};
};

#endif  // __SCREENSHOT_READER_H__