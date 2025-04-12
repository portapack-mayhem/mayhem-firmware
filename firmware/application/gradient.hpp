/*
 * Copyright (C) 2025 Belousov Oleg
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

#ifndef __GRADIENT_H__
#define __GRADIENT_H__

#include "ui.hpp"
#include "file.hpp"

#include <array>
#include <cstdint>

extern const std::filesystem::path default_gradient_file;

class Gradient {
   public:
    std::array<ui::Color, 256> lut{};

    Gradient();

    void set_default();
    bool load_file(const std::filesystem::path& file_path);

   private:
    int16_t prev_index = 0;
    int16_t prev_r = 0;
    int16_t prev_g = 0;
    int16_t prev_b = 0;

    void step(int16_t index, int16_t r, int16_t g, int16_t b);
};

#endif /* __GRADIENT_H__ */
