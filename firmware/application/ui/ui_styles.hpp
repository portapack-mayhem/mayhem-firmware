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

#ifndef __UI_STYLES_H__
#define __UI_STYLES_H__

#include "ui_painter.hpp"

namespace ui {

class Styles {
   public:
    /* The default font, white on black background. */
    static const Style style_default;

    /* The small font, white on black background. */
    static const Style style_small;

    /* Grey foreground. */
    static const Style style_grey;

    /* Yellow foreground. */
    static const Style style_yellow;

    /* Green foreground. */
    static const Style style_green;

    /* Red foreground. */
    static const Style style_red;

    /* Blue foreground. */
    static const Style style_blue;

    /* Blue background. */
    static const Style style_bg_blue;

    /* Dark grey foreground. */
    static const Style style_dark_grey;

    /* Dark grey background. */
    static const Style style_bg_dark_grey;

    /* Light grey foreground. */
    static const Style style_light_grey;

    /* Orange foreground. */
    static const Style style_orange;
};

}  // namespace ui

#endif /*__UI_STYLES_H__*/