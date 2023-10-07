/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#ifndef __UI_calculator_H__
#define __UI_calculator_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "ui_record_view.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "log_file.hpp"
#include "utility.hpp"

using namespace ui;

namespace ui::external_app::calculator {

class CalculatorView : public View {
   public:
    CalculatorView(NavigationView& nav);

    void focus() override;

    std::string title() const override { return "Calculator"; };

   private:
    NavigationView& nav_;

    Button button_F{{0 * 8, 11 * 16, 6 * 8, 16 + 8}, "F"};
    Button button_7{{8 * 8, 11 * 16, 6 * 8, 16 + 8}, "7"};
    Button button_8{{16 * 8, 11 * 16, 6 * 8, 16 + 8}, "8"};
    Button button_9{{24 * 8, 11 * 16, 6 * 8, 16 + 8}, "9"};
    Button button_E{{0 * 8, 13 * 16, 6 * 8, 16 + 8}, "E"};
    Button button_4{{8 * 8, 13 * 16, 6 * 8, 16 + 8}, "4"};
    Button button_5{{16 * 8, 13 * 16, 6 * 8, 16 + 8}, "5"};
    Button button_6{{24 * 8, 13 * 16, 6 * 8, 16 + 8}, "6"};
    Button button_N{{0 * 8, 15 * 16, 6 * 8, 16 + 8}, "N"};
    Button button_1{{8 * 8, 15 * 16, 6 * 8, 16 + 8}, "1"};
    Button button_2{{16 * 8, 15 * 16, 6 * 8, 16 + 8}, "2"};
    Button button_3{{24 * 8, 15 * 16, 6 * 8, 16 + 8}, "3"};
    Button button_C{{0 * 8, 17 * 16, 6 * 8, 16 + 8}, "C"};
    Button button_0{{8 * 8, 17 * 16, 6 * 8, 16 + 8}, "0"};
    Button button_P{{16 * 8, 17 * 16, 6 * 8, 16 + 8}, "."};
    Button button_D{{24 * 8, 17 * 16, 6 * 8, 16 + 8}, "D"};

    Console console{
        {0 * 8, 0 * 16, screen_width, 10 * 16}};

    void on_button_press(uint8_t button);
    void update_button_labels();
};

}  // namespace ui::external_app::calculator

#endif /*__UI_calculator_H__*/
