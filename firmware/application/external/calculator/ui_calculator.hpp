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

    Button button_F{{UI_POS_X_TABLE(4, 0), UI_POS_Y_BOTTOM(9), UI_POS_WIDTH(6), UI_POS_HEIGHT(1.5)}, "F"};
    Button button_7{{UI_POS_X_TABLE(4, 1), UI_POS_Y_BOTTOM(9), UI_POS_WIDTH(6), UI_POS_HEIGHT(1.5)}, "7"};
    Button button_8{{UI_POS_X_TABLE(4, 2), UI_POS_Y_BOTTOM(9), UI_POS_WIDTH(6), UI_POS_HEIGHT(1.5)}, "8"};
    Button button_9{{UI_POS_X_TABLE(4, 3), UI_POS_Y_BOTTOM(9), UI_POS_WIDTH(6), UI_POS_HEIGHT(1.5)}, "9"};
    Button button_E{{UI_POS_X_TABLE(4, 0), UI_POS_Y_BOTTOM(7), UI_POS_WIDTH(6), UI_POS_HEIGHT(1.5)}, "E"};
    Button button_4{{UI_POS_X_TABLE(4, 1), UI_POS_Y_BOTTOM(7), UI_POS_WIDTH(6), UI_POS_HEIGHT(1.5)}, "4"};
    Button button_5{{UI_POS_X_TABLE(4, 2), UI_POS_Y_BOTTOM(7), UI_POS_WIDTH(6), UI_POS_HEIGHT(1.5)}, "5"};
    Button button_6{{UI_POS_X_TABLE(4, 3), UI_POS_Y_BOTTOM(7), UI_POS_WIDTH(6), UI_POS_HEIGHT(1.5)}, "6"};
    Button button_N{{UI_POS_X_TABLE(4, 0), UI_POS_Y_BOTTOM(5), UI_POS_WIDTH(6), UI_POS_HEIGHT(1.5)}, "N"};
    Button button_1{{UI_POS_X_TABLE(4, 1), UI_POS_Y_BOTTOM(5), UI_POS_WIDTH(6), UI_POS_HEIGHT(1.5)}, "1"};
    Button button_2{{UI_POS_X_TABLE(4, 2), UI_POS_Y_BOTTOM(5), UI_POS_WIDTH(6), UI_POS_HEIGHT(1.5)}, "2"};
    Button button_3{{UI_POS_X_TABLE(4, 3), UI_POS_Y_BOTTOM(5), UI_POS_WIDTH(6), UI_POS_HEIGHT(1.5)}, "3"};
    Button button_C{{UI_POS_X_TABLE(4, 0), UI_POS_Y_BOTTOM(3), UI_POS_WIDTH(6), UI_POS_HEIGHT(1.5)}, "C"};
    Button button_0{{UI_POS_X_TABLE(4, 1), UI_POS_Y_BOTTOM(3), UI_POS_WIDTH(6), UI_POS_HEIGHT(1.5)}, "0"};
    Button button_P{{UI_POS_X_TABLE(4, 2), UI_POS_Y_BOTTOM(3), UI_POS_WIDTH(6), UI_POS_HEIGHT(1.5)}, "."};
    Button button_D{{UI_POS_X_TABLE(4, 3), UI_POS_Y_BOTTOM(3), UI_POS_WIDTH(6), UI_POS_HEIGHT(1.5)}, "D"};

    Console console{
        {UI_POS_X(0), UI_POS_Y(0), screen_width, UI_POS_HEIGHT_REMAINING(10)}};

    void on_button_press(uint8_t button);
    void update_button_labels();
};

}  // namespace ui::external_app::calculator

#endif /*__UI_calculator_H__*/
