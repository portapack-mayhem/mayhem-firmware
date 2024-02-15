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

#include "ui_styles.hpp"
#include "ui_painter.hpp"
#include "ui_font_fixed_5x8.hpp"
#include "ui_font_fixed_8x16.hpp"

using namespace ui;

const Style Styles::white{
    .font = ui::font::fixed_8x16,
    .background = ui::Color::black(),
    .foreground = ui::Color::white(),
};

const Style Styles::bg_white{
    .font = ui::font::fixed_8x16,
    .background = ui::Color::white(),
    .foreground = ui::Color::black(),
};

const Style Styles::white_small{
    .font = font::fixed_5x8,
    .background = Color::black(),
    .foreground = Color::white(),
};

const Style Styles::bg_white_small{
    .font = ui::font::fixed_5x8,
    .background = ui::Color::white(),
    .foreground = ui::Color::black(),
};

const Style Styles::bg_yellow_small{
    .font = ui::font::fixed_5x8,
    .background = ui::Color::yellow(),
    .foreground = ui::Color::black(),
};

const Style Styles::yellow{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::yellow(),
};

const Style Styles::green{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::green(),
};

const Style Styles::red{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::red(),
};

const Style Styles::blue{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::blue(),
};

const Style Styles::bg_blue{
    .font = font::fixed_8x16,
    .background = Color::blue(),
    .foreground = Color::white(),
};

const Style Styles::bg_dark_blue{
    .font = font::fixed_8x16,
    .background = Color::dark_blue(),
    .foreground = Color::white(),
};

const Style Styles::light_grey{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::light_grey(),
};

const Style Styles::grey{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::grey(),
};

const Style Styles::dark_grey{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::dark_grey(),
};

const Style Styles::bg_dark_grey{
    .font = font::fixed_8x16,
    .background = Color::dark_grey(),
    .foreground = Color::white(),
};

const Style Styles::orange{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::orange(),
};
