/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#pragma pack(push, 1)
struct bmp_header_t {
    uint16_t signature;
    uint32_t size;
    uint16_t reserved_1;
    uint16_t reserved_2;
    uint32_t image_data;
    uint32_t BIH_size;
    uint32_t width;
    int32_t height;  // can be negative, to signal the bottom-up or reserve status
    uint16_t planes;
    uint16_t bpp;
    uint32_t compression;
    uint32_t data_size;
    uint32_t h_res;
    uint32_t v_res;
    uint32_t colors_count;
    uint32_t icolors_count;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct bmp_palette_t {
    struct color_t {
        uint8_t B;
        uint8_t G;
        uint8_t R;
        uint8_t A;
    } color[16];
};
#pragma pack(pop)
