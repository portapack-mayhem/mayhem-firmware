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

const Style Styles::style_default{
    .font = ui::font::fixed_8x16,
    .background = ui::Color::black(),
    .foreground = ui::Color::white()
};

const Style Styles::style_small{
    .font = font::fixed_5x8,
    .background = Color::black(),
    .foreground = Color::white(),
};

const Style Styles::style_grey{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::grey(),
};

const Style Styles::style_yellow{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::yellow(),
};

const Style Styles::style_green{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::green(),
};

const Style Styles::style_red{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::red(),
};

const Style Styles::style_blue{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::blue(),
};

const Style Styles::style_bg_blue{
    .font = font::fixed_8x16,
    .background = Color::blue(),
    .foreground = Color::white(),
};

const Style Styles::style_dark_grey{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::dark_grey(),
};

const Style Styles::style_light_grey{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::light_grey(),
};

const Style Styles::style_orange{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::orange(),
};