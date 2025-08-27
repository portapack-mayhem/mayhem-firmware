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

#ifndef __BMPFILE__H
#define __BMPFILE__H

#include <cstring>
#include <string>

#include "file.hpp"
#include "bmp.hpp"
#include "ui.hpp"

class BMPFile {
   public:
    ~BMPFile();
    bool open(const std::filesystem::path& file, bool readonly);
    bool create(const std::filesystem::path& file, uint32_t x, uint32_t y);
    void close();
    bool is_loaded();
    bool seek(uint32_t x, uint32_t y);
    bool expand_y(uint32_t new_y);
    bool expand_y_delta(uint32_t delta_y);
    uint32_t getbpr() { return byte_per_row; };

    bool read_next_px(ui::Color& px, bool seek);
    bool write_next_px(ui::Color& px);
    uint32_t get_real_height();
    uint32_t get_width();
    bool is_bottomup();
    void set_bg_color(ui::Color background);
    void delete_bg_color();

   private:
    bool advance_curr_px(uint32_t num);
    bool is_opened = false;
    bool is_read_only = true;

    File bmpimage{};
    size_t file_pos = 0;
    bmp_header_t bmp_header{};
    uint8_t type = 0;
    uint8_t byte_per_px = 1;
    uint32_t byte_per_row = 0;

    uint32_t currx = 0;
    uint32_t curry = 0;
    ui::Color bg{};
    // uint8_t color_palette[256][4];
    bool use_bg = false;
};

#endif