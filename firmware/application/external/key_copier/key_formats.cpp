/*
 * Copyright (C) 2026 osahv
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
 *
 * Key Copier is a port of KeyCopier for Flipper Zero (https://github.com/zinongli/KeyCopier),
 * Copyright (c) 2024 zinongli, MIT License (see LICENSE-KeyCopier-MIT in this folder).
 * The key format table and the contour algorithm come from there.
 */

// Key format table ported from zinongli/KeyCopier key_formats.c (MIT). 23 formats.
// Lengths are in 1/10000 inch, the tangent in 1/1000 (integer math, no FPU on the M0).
#include "key_formats.hpp"
namespace ui::external_app::key_copier {
const KeyFormat all_formats[FORMAT_NUM] = {
    {"Kwikset", "KW1", 0, 0, 2470, 1500, 5, 840, 1000, 1500, 3290, 230, 1, 7, 4, 3, 8470},
    {"Schlage", "SC4", 0, 0, 2310, 1562, 6, 310, 1000, 1000, 3350, 150, 0, 9, 7, 8, 10120},
    {"Arrow", "AR4", 0, 0, 2650, 1550, 6, 600, 1000, 1000, 3120, 140, 0, 9, 6, 7, 10400},
    {"Master Lock", "M1", 0, 0, 1850, 1260, 5, 390, 1000, 1000, 2760, 150, 0, 7, 7, 6, 6890},
    {"American", "AM7", 0, 0, 1570, 1250, 6, 390, 1000, 1000, 2830, 160, 1, 8, 7, 5, 7810},
    {"Yale", "Y2", 0, 0, 2000, 1650, 6, 540, 1000, 1000, 3200, 190, 0, 9, 9, 4, 10250},
    {"Yale", "Y11", 0, 0, 1240, 950, 5, 390, 1000, 1000, 2460, 200, 1, 5, 7, 3, 5020},
    {"Sargent", "S22", 0, 0, 2160, 1560, 6, 630, 1000, 1000, 3280, 200, 1, 10, 7, 5, 9960},
    {"National", "NA25", 0, 0, 2500, 1560, 5, 390, 1000, 1000, 3040, 120, 0, 9, 7, 8, 8740},
    {"Corbin", "CO88", 0, 0, 2500, 1560, 6, 470, 1000, 1000, 3430, 140, 1, 10, 7, 8, 10300},
    {"Lockwood", "LW4", 0, 0, 2450, 1562, 5, 310, 1000, 1000, 3440, 140, 0, 9, 9, 8, 8700},
    {"Lockwood", "LW5", 0, 0, 2450, 1562, 6, 310, 1000, 1000, 3440, 140, 0, 9, 9, 8, 10262},
    {"National", "NA12", 0, 0, 1500, 1400, 5, 390, 1000, 1000, 2700, 130, 0, 9, 7, 8, 7100},
    {"Russwin", "RU45", 0, 0, 2500, 1560, 6, 530, 1000, 1000, 3430, 280, 1, 6, 5, 3, 10300},
    {"Weiser", "WR3", 0, 0, 2370, 1560, 5, 900, 1000, 1500, 3150, 180, 0, 10, 6, 3, 8610},
    {"Ford", "H75", 2, 2, 2010, 920, 8, 390, 1000, 2010, 3540, 250, 1, 5, 5, 2, 8450},
    {"Chevrolet", "B102", 2, 2, 2050, 930, 10, 390, 1000, 2050, 3150, 260, 1, 4, 5, 2, 10370},
    {"Dodge", "Y159", 2, 2, 2970, 920, 8, 390, 1000, 2970, 3390, 470, 1, 4, 5, 1, 9410},
    {"Kawasaki", "KA14", 2, 0, 980, 980, 6, 390, 1000, 1000, 2580, 200, 1, 4, 4, 3, 5910},
    {"Suzuki", "SUZ18", 2, 0, 1600, 950, 7, 450, 1000, 1000, 2800, 200, 1, 4, 4, 3, 7300},
    {"Yamaha", "YM63", 2, 0, 1570, 980, 7, 390, 1000, 1000, 2950, 200, 1, 4, 4, 3, 7480},
    {"Best (A2)", "SFIC", 0, 2, 2500, 1490, 6, 510, 1000, 810, 3180, 250, 0, 9, 5, 3, 9980},
    {"RV (FIC,GL,Bauer)", "RV", 2, 0, 1260, 940, 5, 390, 1000, 1260, 2600, 400, 1, 3, 3, 1, 5040},
};
}
