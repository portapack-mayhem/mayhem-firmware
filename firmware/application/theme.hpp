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

#ifndef __THEME_H__
#define __THEME_H__
#include <cstdint>
#include <cstddef>
#include <memory>
#include "ui_painter.hpp"
#include "ui_font_fixed_5x8.hpp"
#include "ui_font_fixed_8x16.hpp"

namespace ui {

class ThemeDefault {
   public:
    // definition of ui styles and colors thet will be refereced.
    Style bg_lightest{
        .font = font::fixed_8x16,
        .background = Color::white(),
        .foreground = Color::black(),
    };
    Style bg_lightest_small{
        .font = font::fixed_8x16,
        .background = Color::white(),
        .foreground = Color::black(),
    };
    Style bg_light{
        .font = font::fixed_8x16,
        .background = Color::light_grey(),
        .foreground = Color::white(),
    };
    Style bg_medium{
        .font = font::fixed_8x16,
        .background = Color::grey(),
        .foreground = Color::white(),
    };
    Style bg_dark{
        .font = font::fixed_8x16,
        .background = Color::dark_grey(),
        .foreground = Color::white(),
    };
    Style bg_darker{
        .font = font::fixed_8x16,
        .background = Color::darker_grey(),
        .foreground = Color::white(),
    };

    Style bg_darkest{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::white(),
    };
    Style bg_darkest_small{
        .font = font::fixed_5x8,
        .background = Color::black(),
        .foreground = Color::white(),
    };

    Style bg_important_small{
        .font = ui::font::fixed_5x8,
        .background = ui::Color::yellow(),
        .foreground = ui::Color::black(),
    };

    Style error_dark{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::red(),
    };
    Style warning_dark{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::yellow(),
    };
    Style ok_dark{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::green(),
    };

    Style fg_dark{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::dark_grey(),
    };
    Style fg_medium{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::grey(),
    };
    Style fg_light{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::light_grey(),
    };

    Style fg_red{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::red(),
    };
    Style fg_green{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::green(),
    };
    Style fg_yellow{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::yellow(),
    };
    Style fg_orange{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::orange(),
    };
    Style fg_blue{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::blue(),
    };
    Style fg_cyan{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::cyan(),
    };
    Style fg_darkcyan{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::dark_cyan(),
    };
    Style fg_magenta{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::magenta(),
    };

    Style option_active{
        .font = font::fixed_8x16,
        .background = Color::blue(),
        .foreground = Color::white(),
    };

    Color status_active{0, 255, 0};  // green, the status bar icons when active

    Color bg_table_header{0, 0, 255};
};

class ThemeYellow : public ThemeDefault {
   public:
    // definition of ui styles and colors thet will be refereced.
    Style bg_lightest{
        .font = font::fixed_8x16,
        .background = Color::white(),
        .foreground = Color::black(),
    };
    Style bg_lightest_small{
        .font = font::fixed_8x16,
        .background = Color::white(),
        .foreground = Color::black(),
    };
    Style bg_light{
        .font = font::fixed_8x16,
        .background = Color::light_grey(),
        .foreground = Color::white(),
    };
    Style bg_medium{
        .font = font::fixed_8x16,
        .background = Color::grey(),
        .foreground = Color::white(),
    };
    Style bg_dark{
        .font = font::fixed_8x16,
        .background = Color::dark_grey(),
        .foreground = Color::white(),
    };
    Style bg_darker{
        .font = font::fixed_8x16,
        .background = Color::darker_grey(),
        .foreground = Color::white(),
    };

    Style bg_darkest{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::white(),
    };
    Style bg_darkest_small{
        .font = font::fixed_5x8,
        .background = Color::black(),
        .foreground = Color::white(),
    };

    Style bg_important_small{
        .font = ui::font::fixed_5x8,
        .background = ui::Color::yellow(),
        .foreground = ui::Color::black(),
    };

    Style error_dark{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::red(),
    };
    Style warning_dark{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::yellow(),
    };
    Style ok_dark{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::green(),
    };

    Style fg_dark{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::dark_grey(),
    };
    Style fg_medium{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::grey(),
    };
    Style fg_light{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::light_grey(),
    };

    Style fg_red{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::red(),
    };
    Style fg_green{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::green(),
    };
    Style fg_yellow{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::yellow(),
    };
    Style fg_orange{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::orange(),
    };
    Style fg_blue{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::blue(),
    };
    Style fg_cyan{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::cyan(),
    };
    Style fg_darkcyan{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::dark_cyan(),
    };
    Style fg_magenta{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::magenta(),
    };

    Style option_active{
        .font = font::fixed_8x16,
        .background = Color::blue(),
        .foreground = Color::white(),
    };

    Color status_active{0, 255, 0};  // green, the status bar icons when active

    Color bg_table_header{0, 0, 255};
};

class Theme {
   public:
    enum ThemeId {
        DefaultGrey = 0,
        Yellow = 1
    };
    static void SetTheme(ThemeId theme);
    static ThemeDefault* current;
};

}  // namespace ui
#endif /*__THEME_H__*/
