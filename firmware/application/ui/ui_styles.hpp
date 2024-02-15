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
    /* White foreground. */
    static const Style white;

    /* White background. */
    static const Style bg_white;

    /* White foreground, small font. */
    static const Style white_small;

    /* White background, small font. */
    static const Style bg_white_small;

    /* Red background, small font. */
    static const Style bg_yellow_small;

    /* Yellow foreground. */
    static const Style yellow;

    /* Green foreground. */
    static const Style green;

    /* Red foreground. */
    static const Style red;

    /* Blue foreground. */
    static const Style blue;

    /* Blue background. */
    static const Style bg_blue;

    /* Dark blue background. */
    static const Style bg_dark_blue;

    /* Light grey foreground. */
    static const Style light_grey;

    /* Grey foreground. */
    static const Style grey;

    /* Dark grey foreground. */
    static const Style dark_grey;

    /* Dark grey background. */
    static const Style bg_dark_grey;

    /* Orange foreground. */
    static const Style orange;
};

}  // namespace ui

#endif /*__UI_STYLES_H__*/
