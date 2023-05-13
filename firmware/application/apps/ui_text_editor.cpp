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

 #include "ui_text_editor.hpp"

 using namespace portapack;
namespace fs = std::filesystem;

 namespace {
ui::FileInfo get_file_info() {
    return { };
}
 } // namespace

 namespace ui {

TextEditorView::TextEditorView(NavigationView& nav)
    : nav_{ nav }
{
    add_children({
        &text_position
});

    // TODO: need a temp backing file.

    log_.append("TEXTEDITOR.TXT");

    log_.write_entry("Opening file");
    file_.open("PGCSAG.TXT");

    get_file_info();
    text_position.set("test test");

    set_dirty();
}

 } // namespace ui