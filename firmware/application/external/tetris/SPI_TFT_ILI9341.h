/*
 * Copyright (C) 2024 Mark Thompson
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

// "HAL" display layer for Tetris code to run on PortaPack without its original ILI9341 functions

#ifndef __UI_SPI_TFT_ILI9341_H__
#define __UI_SPI_TFT_ILI9341_H__

ui::Painter painter;

static int x_pos{0};
static int y_pos{0};
static int fg_color;
static int bg_color;

enum {
    White,
    Blue,
    Yellow,
    Purple,
    Green,
    Red,
    Maroon,
    Orange,
    Black,
};

// pp_colors must be in same order as enums above
static const Color pp_colors[] = {
    Color::white(),
    Color::blue(),
    Color::yellow(),
    Color::purple(),
    Color::green(),
    Color::red(),
    Color::magenta(),
    Color::orange(),
    Color::black(),
};

// NB: ELIMINATED SPI_TFT_ILI9341 DISPLAY CLASS DUE TO GLOBAL OBJECT INITIALIZATION ISSUE WITH EXTERNAL APPS

static void claim(__FILE* x) {
    (void)x;
};

static void cls() {
    painter.fill_rectangle({0, 0, portapack::display.width(), portapack::display.height()}, Color::black());
};

static void background(int color) {
    bg_color = color;
};

static void foreground(int color) {
    fg_color = color;
};

static void locate(int x, int y) {
    x_pos = x;
    y_pos = y;
};

static void set_orientation(int x) {
    (void)x;
};

static void set_font(unsigned char* x) {
    (void)x;
};

static void fillrect(int x1, int y1, int x2, int y2, int color) {
    painter.fill_rectangle({x1, y1, x2 - x1, y2 - y1}, pp_colors[color]);
};

static void rect(int x1, int y1, int x2, int y2, int color) {
    painter.draw_rectangle({x1, y1, x2 - x1, y2 - y1}, pp_colors[color]);
};

static void printf(std::string str) {
    auto style = (fg_color == White) ? *ui::Theme::getInstance()->bg_darkest : *ui::Theme::getInstance()->bg_lightest;
    painter.draw_string({x_pos, y_pos - 1}, style, str);
};

static void printf(std::string str, int v) {
    if (str.find_first_of("%") != std::string::npos) {
        str.resize(str.find_first_of("%"));  // remove %d from end of string
    }
    printf(str + to_string_dec_uint(v));
};

#endif /*__UI_SPI_TFT_ILI9341_H__*/
