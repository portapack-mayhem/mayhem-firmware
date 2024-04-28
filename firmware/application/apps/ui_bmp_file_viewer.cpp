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

#include "ui_bmp_file_viewer.hpp"

extern ui::SystemView* system_view_ptr;

using namespace portapack;
namespace fs = std::filesystem;

namespace ui {

BMPFileViewer::BMPFileViewer(
    NavigationView& nav,
    const std::filesystem::path& path)
    : nav_{nav},
      path_{path} {
    add_children(
        {&bmp});
    bmp.set_enter_pass(true);  // pass the enter key to me, so i can exit. this will disable zoom + pos reset
    set_focusable(true);
    system_view_ptr->set_app_fullscreen(true);
}

BMPFileViewer::~BMPFileViewer() {
    system_view_ptr->set_app_fullscreen(false);
}

void BMPFileViewer::focus() {
    bmp.focus();
}

bool BMPFileViewer::on_key(KeyEvent k) {
    if (k == KeyEvent::Select) {
        nav_.pop();
        return true;
    }
    return false;
}

void BMPFileViewer::paint(Painter&) {
    bmp.load_bmp(path_);
}

}  // namespace ui
