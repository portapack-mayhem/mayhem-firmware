/*
 * Copyright (C) 2024 Bernd Herzog
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

#ifndef __UI_EXTERNAL_MODULE_VIEW_H
#define __UI_EXTERNAL_MODULE_VIEW_H

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

#include "i2cdevmanager.hpp"
#include "i2cdev_ppmod.hpp"

namespace ui {

class ExternalModuleView : public View {
   public:
    ExternalModuleView(NavigationView& nav)
        : nav_(nav) {
        add_children({&text_header,
                      &text_name,
                      &text_version,
                      &text_number_apps,
                      &text_app1_name,
                      &text_app2_name,
                      &text_app3_name,
                      &text_app4_name,
                      &text_app5_name,
                      &dummy});

        text_header.set("No module connected");

        signal_token_tick_second = rtc_time::signal_tick_second += [this]() {
            on_tick_second();
        };
    }

    ~ExternalModuleView() {
        rtc_time::signal_tick_second -= signal_token_tick_second;
    }

    std::string title() const override { return "Ext Module"; };
    void focus() override;

   private:
    NavigationView& nav_;
    Text text_header{{16, 16, 208, 16}};
    Text text_name{{24, 32, 200, 16}};
    Text text_version{{24, 48, 200, 16}};
    Text text_number_apps{{24, 64, 200, 16}};

    Text text_app1_name{{24, 96, 200, 16}};
    Text text_app2_name{{24, 112, 200, 16}};
    Text text_app3_name{{24, 128, 200, 16}};
    Text text_app4_name{{24, 144, 200, 16}};
    Text text_app5_name{{24, 160, 200, 16}};

    Button dummy{
        {screen_width, 0, 0, 0},
        ""};

    SignalToken signal_token_tick_second{};

    void on_tick_second();
};

}  // namespace ui

#endif
