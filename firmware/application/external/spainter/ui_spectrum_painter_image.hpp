/*
 * Copyright (C) 2023 Bernd Herzog
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

#pragma once

#include "ui.hpp"
#include "ui_widget.hpp"

#include "ui_navigation.hpp"
#include "ui_tabview.hpp"
#include "ui_transmitter.hpp"
#include "baseband_api.hpp"

#include "portapack.hpp"
#include "message.hpp"

namespace ui::external_app::spainter {

class SpectrumInputImageView : public View {
   public:
    SpectrumInputImageView(NavigationView& nav);
    ~SpectrumInputImageView();

    void focus() override;
    void paint(Painter&) override;

    uint16_t get_width();
    uint16_t get_height();
    std::vector<uint8_t> get_line(uint16_t);

    std::function<void()> on_input_available{};

   private:
    bool painted{false};
    std::string file{""};
    uint16_t width{0};
    uint16_t height{0};
    uint8_t type{0};
    uint32_t data_start{0};

    Button button_load_image{
        {0 * 8, 11 * 16 - 4, 30 * 8, 28},
        "Load Image ..."};

    bool drawBMP_scaled(const ui::Rect r, const std::string file);
};

}  // namespace ui::external_app::spainter
