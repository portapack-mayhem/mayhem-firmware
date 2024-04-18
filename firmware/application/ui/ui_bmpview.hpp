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

class BMPView : public Widget {
   public:
    BMPView(Rect parent_rect);
    BMPView(const BMPView& other) = delete;
    BMPView& operator=(const BMPView& other) = delete;

    void paint(Painter& painter) override;
    void on_focus() override;
    void on_blur() override;
    bool on_key(const KeyEvent key) override;
    bool on_encoder(EncoderEvent delta) override;
    bool on_keyboard(const KeyboardEvent event) override;

    void set_zoom(int8_t new_zoom);
    int8_t get_zoom();

   private:
    int8_t zoom = 1;  // positive = zoom in, negative = zoom out 0-invalid 1- no zoom
};

#endif