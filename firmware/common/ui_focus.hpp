/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __UI_FOCUS_H__
#define __UI_FOCUS_H__

#include "ui.hpp"

namespace ui {

class Widget;

class FocusManager {
   public:
    Widget* focus_widget() const;
    void set_focus_widget(Widget* const new_focus_widget);

    void update(Widget* const top_widget, const KeyEvent event);
    // void update(Widget* const top_widget, const TouchEvent event);

    void setMirror(Widget* const mirror_widget);
    void clearMirror();

   private:
    Widget* focus_widget_{nullptr};
    Widget* mirror_widget_{nullptr};
};

} /* namespace ui */

#endif /*__UI_FOCUS_H__*/
