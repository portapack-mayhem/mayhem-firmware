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
    virtual ~ThemeTemplate();
    Style* bg_lightest = nullptr;
    Style* bg_lightest_small = nullptr;
    Style* bg_light = nullptr;
    Style* bg_medium = nullptr;
    Style* bg_dark = nullptr;
    Style* bg_darker = nullptr;

    Style* bg_darkest = nullptr;
    Style* bg_darkest_small = nullptr;

    Style* bg_important_small = nullptr;

    Style* error_dark = nullptr;
    Style* warning_dark = nullptr;
    Style* ok_dark = nullptr;

    Style* fg_dark = nullptr;
    Style* fg_medium = nullptr;
    Style* fg_light = nullptr;

    Style* fg_red = nullptr;
    Style* fg_green = nullptr;
    Style* fg_yellow = nullptr;
    Style* fg_orange = nullptr;
    Style* fg_blue = nullptr;
    Style* fg_cyan = nullptr;
    Style* fg_darkcyan = nullptr;
    Style* fg_magenta = nullptr;

    Style* option_active = nullptr;

    Color* status_active = nullptr;  // green, the status bar icons when active
    Color* bg_table_header = nullptr;
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

class ThemeDark : public ThemeTemplate {
   public:
    ThemeDark();
};

class Theme {
   public:
    enum ThemeId {
        DefaultGrey = 0,
        Yellow = 1,
        Aqua = 2,
        Green = 3,
        Red = 4,
        Dark = 5,
        MAX
    };
    static ThemeTemplate* getInstance();

    static void SetTheme(ThemeId theme);
    static ThemeTemplate* current;
    static void destroy();  // used from standalone app, to prevent memleak
   private:
};

}  // namespace ui
#endif /*__THEME_H__*/
