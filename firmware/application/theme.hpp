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

class ThemeTemplate {
   public:
    ~ThemeTemplate();
    Style* bg_lightest;
    Style* bg_lightest_small;
    Style* bg_light;
    Style* bg_medium;
    Style* bg_dark;
    Style* bg_darker;

    Style* bg_darkest;
    Style* bg_darkest_small;

    Style* bg_important_small;

    Style* error_dark;
    Style* warning_dark;
    Style* ok_dark;

    Style* fg_dark;
    Style* fg_medium;
    Style* fg_light;

    Style* fg_red;
    Style* fg_green;
    Style* fg_yellow;
    Style* fg_orange;
    Style* fg_blue;
    Style* fg_cyan;
    Style* fg_darkcyan;
    Style* fg_magenta;

    Style* option_active;

    Color* status_active;  // green, the status bar icons when active
    Color* bg_table_header;
};

class ThemeDefault : public ThemeTemplate {
   public:
    ThemeDefault();
};

class ThemeYellow : public ThemeTemplate {
   public:
    ThemeYellow();
};

class ThemeAqua : public ThemeTemplate {
   public:
    ThemeAqua();
};

class ThemeGreen : public ThemeTemplate {
   public:
    ThemeGreen();
};

class ThemeRed : public ThemeTemplate {
   public:
    ThemeRed();
};

class Theme {
   public:
    enum ThemeId {
        DefaultGrey = 0,
        Yellow = 1,
        Aqua = 2,
        Green = 3,
        Red = 4,
        MAX
    };
    static ThemeTemplate* getInstance();

    static void SetTheme(ThemeId theme);
    static ThemeTemplate* current;

   private:
};

}  // namespace ui
#endif /*__THEME_H__*/
