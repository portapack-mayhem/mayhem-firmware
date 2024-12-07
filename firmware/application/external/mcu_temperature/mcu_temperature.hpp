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

#ifndef __MCU_TEMPERATURE_H__
#define __MCU_TEMPERATURE_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"
#include "ui_menu.hpp"
#include "ui_navigation.hpp"

#include "rffc507x.hpp"
#include "portapack.hpp"
#include "memory_map.hpp"
#include "irq_controls.hpp"

#include <functional>
#include <utility>

namespace ui::external_app::mcu_temperature {

class McuTemperatureWidget : public Widget {
   public:
    explicit McuTemperatureWidget(
        Rect parent_rect)
        : Widget{parent_rect} {
    }

    void paint(Painter& painter) override;

   private:
    using sample_t = uint32_t;
    using temperature_t = int32_t;

    temperature_t temperature(const sample_t sensor_value) const;
    Coord screen_y(const temperature_t temperature, const Rect& screen_rect) const;

    std::string temperature_str(const temperature_t temperature) const;

    static constexpr temperature_t display_temp_min = -10;  // Accomodate negative values, present in cold startup cases
    static constexpr temperature_t display_temp_scale = 3;
    static constexpr int bar_width = 1;
    static constexpr int temp_len = 5;  // Now scale shows up to 5 chars ("-10ºC")
};

class McuTemperatureView : public View {
   public:
    explicit McuTemperatureView(NavigationView& nav);

    void focus() override;

    std::string title() const override { return "Temperature"; };

   private:
    Text text_title{
        {76, 16, 240, 16},
        "Temperature",
    };

    McuTemperatureWidget temperature_widget{
        {0, 40, 240, 180},
    };

    Button button_done{
        {72, 264, 96, 24},
        "Done"};
};

}  // namespace ui::external_app::mcu_temperature

#endif /*__MCU_TEMPERATURE_H__*/
