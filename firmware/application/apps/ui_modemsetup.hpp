/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#include "modems.hpp"
#include "serializer.hpp"

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"

namespace ui {

class ModemSetupView : public View {
   public:
    ModemSetupView(NavigationView& nav);

    void focus() override;

    std::string title() const override { return "Modem setup"; };

   private:
    Labels labels{
        {{2 * 8, 11 * 8}, "Baudrate:", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 13 * 8}, "Mark:      Hz", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 15 * 8}, "Space:     Hz", Theme::getInstance()->fg_light->foreground},
        {{140, 15 * 8}, "Repeat:", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 6 * 8}, "Modem preset:", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 22 * 8}, "Serial format:", Theme::getInstance()->fg_light->foreground}};

    NumberField field_baudrate{
        {11 * 8, 11 * 8},
        5,
        {50, 9600},
        25,
        ' '};

    NumberField field_mark{
        {8 * 8, 13 * 8},
        5,
        {100, 15000},
        25,
        ' '};

    NumberField field_space{
        {8 * 8, 15 * 8},
        5,
        {100, 15000},
        25,
        ' '};

    NumberField field_repeat{
        {204, 15 * 8},
        2,
        {1, 99},
        1,
        ' '};

    OptionsField options_modem{
        {15 * 8, 6 * 8},
        7,
        {}};

    SymField sym_format_data{
        {16 * 8, 22 * 8},
        1,
        "6789"};

    SymField sym_format_parity{
        {17 * 8, 22 * 8},
        1,
        "NEo"};

    SymField sym_format_stop{
        {18 * 8, 22 * 8},
        1,
        "012"};

    SymField sym_format_msb{
        {19 * 8, 22 * 8},
        1,
        "ML"};

    Button button_set_modem{
        {23 * 8, 6 * 8 - 4, 6 * 8, 24},
        "SET"};

    Button button_save{
        {9 * 8, 250, 96, 40},
        "Save"};
};

} /* namespace ui */
