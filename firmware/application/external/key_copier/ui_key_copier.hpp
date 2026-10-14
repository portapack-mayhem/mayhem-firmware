/*
 * Copyright (C) 2026 osahv
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
 *
 * Key Copier is a port of KeyCopier for Flipper Zero (https://github.com/zinongli/KeyCopier),
 * Copyright (c) 2024 zinongli, MIT License (see LICENSE-KeyCopier-MIT in this folder).
 * The key format table and the contour algorithm come from there.
 */

#pragma once

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"
#include "ui_navigation.hpp"
#include "key_formats.hpp"

using namespace ui;

namespace ui::external_app::key_copier {

class KeyCopierView : public View {
   public:
    KeyCopierView(NavigationView& nav);
    void paint(Painter& painter) override;
    void focus() override;
    bool on_key(const KeyEvent key) override;
    bool on_encoder(const EncoderEvent delta) override;
    std::string title() const override { return "Key Copier"; }

   private:
    static constexpr int kMaxPins = 10;

    void select_format(int index);
    void line(Painter& painter, int x0, int y0, int x1, int y1);
    Point rot_point(int x, int y) const;  // maps drawing coordinates through the current orientation
    // Draws text at local coordinates. In landscape (90/270) the text is rotated with the key so it reads upright when
    // the device is turned on its side.
    void text_at(Painter& painter, int x, int y, Color fg, std::string_view s);
    int rel_depth(int pin) const;  // 0 outside the pin range
    int px(int units) const;       // 1/10000 inch to pixels

    NavigationView& nav_;
    int format_index_{0};
    int pin_{0};
    int rot_{0};             // 0..3 = 0/90/180/270 degrees
    int cx_{120}, cy_{170};  // rotation centre
    int depth_[kMaxPins]{};
    Color ink_{Color::black()};
};

}  // namespace ui::external_app::key_copier
