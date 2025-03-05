/*
 * Copyright (C) 2023 Mark Thompson
 * copyleft Whiterose of the Dark Army
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

#ifndef __UI_DEBUG_H__
#define __UI_DEBUG_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"
#include "ui_menu.hpp"
#include "ui_navigation.hpp"

#include <functional>
#include <utility>

using namespace ui;

namespace ui::external_app::font_viewer {

class DebugFontsView : public View {
   public:
    DebugFontsView(NavigationView& nav);
    void paint(Painter& painter) override;
    void focus() override;
    std::string title() const override { return "Fonts"; };

   private:
    uint16_t display_font(Painter& painter, uint16_t y_offset, const Style* font_style, std::string_view font_name, bool is_big_font);
    void update_address_text();

    NumberField field_cursor{
        {0 * 8, 0 * 8},
        1,
        {0, 1000},
        1,
        ' '};
    Text text_address{
        {screen_width / 2, 0 * 16, screen_width / 2, 16},
        "0x20",
    };
    NavigationView& nav_;
};

} /* namespace ui::external_app::font_viewer */

#endif /*__UI_FONT_VIEWER_H__*/
