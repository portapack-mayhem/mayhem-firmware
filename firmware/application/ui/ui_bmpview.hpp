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

#ifndef __UIBMPVIEW_H__
#define __UIBMPVIEW_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "bmpfile.hpp"

class BMPViewer : public Widget {
   public:
    BMPViewer(Rect parent_rect);
    BMPViewer(Rect parent_rect, const std::filesystem::path& file);
    BMPViewer(const BMPViewer& other) = delete;
    BMPViewer& operator=(const BMPViewer& other) = delete;

    bool load_bmp(const std::filesystem::path& file);

    void paint(Painter& painter) override;
    void on_focus() override;
    void on_blur() override;
    bool on_key(const KeyEvent key) override;
    bool on_encoder(EncoderEvent delta) override;
    bool on_keyboard(const KeyboardEvent event) override;

    void reset_pos();
    void set_zoom(int8_t new_zoom);
    int8_t get_zoom();

    void set_enter_pass(bool pass);
    bool get_enter_pass();

   private:
    void get_line(ui::Color* line, uint32_t bx, uint32_t by, uint32_t cnt);
    bool move_pos(int32_t delta_x, int32_t delta_y);
    BMPFile bmp{};
    int8_t zoom = 1;      // positive = zoom in, negative = zoom out 0-invalid 1- no zoom
    int8_t zoom_fit = 1;  // if this value is set, the image will fit the screen the most
    int8_t max_zoom = 10;
    int8_t min_zoom = -20;  // will be calculated on load
    uint32_t cx = 0;        // current top-left coordinate
    uint32_t cy = 0;
    uint32_t mvx = 1;  // how much to move on key
    uint32_t mvy = 1;
    bool enter_pass = false;
};

#endif