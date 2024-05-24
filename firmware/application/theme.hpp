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

#include "ui_painter.hpp"
#include "ui_font_fixed_5x8.hpp"
#include "ui_font_fixed_8x16.hpp"

namespace ui {

class Theme {
   public:
    enum ThemeId {
        DefaultGrey = 0
    };
    static void SetTheme(ThemeId theme);

    // definition of ui styles and colors thet will be refereced.
    static Style bg_lightest;
    static Style bg_lightest_small;
    static Style bg_light;    //
    static Style bg_dark;     // navbar, buttons
    static Style bg_darker;   // infobar
    static Style bg_darkest;  // usually black
    static Style bg_darkest_small;

    static Style bg_important_small;

    static Style error_dark;    // red
    static Style warning_dark;  // yellow
    static Style ok_dark;       // green

    static Style fg_darker;
    static Style fg_dark;
    static Style fg_light;  // for example labels

    static Style fg_red;
    static Style fg_green;
    static Style fg_yellow;
    static Style fg_orange;
    static Style fg_blue;
    static Style fg_cyan;
    static Style fg_darkcyan;
    static Style fg_magenta;

    static Style option_active;

    static Color status_active;  // green, the status bar icons when active

    static Color bg_table_header;
};

}  // namespace ui
#endif /*__THEME_H__*/
