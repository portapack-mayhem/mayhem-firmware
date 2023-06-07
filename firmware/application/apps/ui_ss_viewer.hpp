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

#ifndef __UI_SS_VIEWER_H__
#define __UI_SS_VIEWER_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_painter.hpp"
#include "ui_styles.hpp"
#include "ui_widget.hpp"
#include "file.hpp"

namespace ui {

class ScreenshotViewer : public View {
   public:
    ScreenshotViewer(NavigationView& nav, const std::filesystem::path& path);
    bool on_key(KeyEvent key) override;
    void paint(Painter& painter) override;

   private:
    NavigationView& nav_;
    std::filesystem::path path_{};
};

}  // namespace ui

#endif  // __UI_SS_VIEWER_H__
