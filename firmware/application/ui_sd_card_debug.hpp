/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __UI_SD_CARD_DEBUG_H__
#define __UI_SD_CARD_DEBUG_H__

#include "ui_widget.hpp"
#include "ui_navigation.hpp"

#include "sd_card.hpp"

namespace ui {

class SDCardDebugView : public View {
   public:
    SDCardDebugView(NavigationView& nav);

    void on_show() override;
    void on_hide() override;

    void focus() override;

    std::string title() const override { return "SD Card"; };

   private:
    SignalToken sd_card_status_signal_token{};

    void on_status(const sd_card::Status status);
    void on_test();
    std::string fetch_sdcard_format();

    Labels labels{
        {{UI_POS_X(0), 1 * 16}, "Format", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 3 * 16}, "CSD", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 5 * 16}, "Bus width", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 6 * 16}, "Card type", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 8 * 16}, "Block size", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 9 * 16}, "Block count", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 10 * 16}, "Capacity", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 5 * 16}, "Bus width", Theme::getInstance()->fg_light->foreground},

    };

    Text text_format{
        {screen_width - (int)((sizeof("Undefined: 255") + 1) * 8), 1 * 16, (sizeof("Undefined: 255") + 1) * 8, 16},
        "Unknown",
    };

    Text text_csd_value_3{
        {screen_width - ((8 + 1 + 8) * 8), 3 * 16, (8 * 8), 16},
        "",
    };

    Text text_csd_value_2{
        {screen_width - (8 * 8), 3 * 16, (8 * 8), 16},
        "",
    };

    Text text_csd_value_1{
        {screen_width - ((8 + 1 + 8) * 8), 4 * 16, (8 * 8), 16},
        "",
    };

    Text text_csd_value_0{
        {screen_width - (8 * 8), 4 * 16, (8 * 8), 16},
        "",
    };

    static constexpr size_t bus_width_characters = 1;

    Text text_bus_width_value{
        {screen_width - (int)(bus_width_characters * 8), 5 * 16, (bus_width_characters * 8), 16},
        "",
    };

    static constexpr size_t card_type_characters = 13;

    Text text_card_type_value{
        {screen_width - (int)(card_type_characters * 8), 6 * 16, (card_type_characters * 8), 16},
        "",
    };

    static constexpr size_t block_size_characters = 5;

    Text text_block_size_value{
        {screen_width - (int)(block_size_characters * 8), 8 * 16, (block_size_characters * 8), 16},
        "",
    };

    static constexpr size_t block_count_characters = 9;

    Text text_block_count_value{
        {screen_width - (int)(block_count_characters * 8), 9 * 16, (block_count_characters * 8), 16},
        "",
    };

    static constexpr size_t capacity_characters = 10;

    Text text_capacity_value{
        {screen_width - (int)(capacity_characters * 8), 10 * 16, (capacity_characters * 8), 16},
        "",
    };

    ///////////////////////////////////////////////////////////////////////

    static constexpr size_t test_write_time_characters = 23;

    Text text_test_write_time_title{
        {0, 12 * 16, (4 * 8), 16},
        "W ms",
    };

    Text text_test_write_time_value{
        {screen_width - (int)(test_write_time_characters * 8), 12 * 16, (test_write_time_characters * 8), 16},
        "",
    };

    static constexpr size_t test_write_rate_characters = 23;

    Text text_test_write_rate_title{
        {0, 13 * 16, (6 * 8), 16},
        "W MB/s",
    };

    Text text_test_write_rate_value{
        {screen_width - (int)(test_write_rate_characters * 8), 13 * 16, (test_write_rate_characters * 8), 16},
        "",
    };

    ///////////////////////////////////////////////////////////////////////

    static constexpr size_t test_read_time_characters = 23;

    Text text_test_read_time_title{
        {0, 14 * 16, (4 * 8), 16},
        "R ms",
    };

    Text text_test_read_time_value{
        {screen_width - (int)(test_read_time_characters * 8), 14 * 16, (test_read_time_characters * 8), 16},
        "",
    };

    static constexpr size_t test_read_rate_characters = 23;

    Text text_test_read_rate_title{
        {0, 15 * 16, (6 * 8), 16},
        "R MB/s",
    };

    Text text_test_read_rate_value{
        {screen_width - (int)(test_read_rate_characters * 8), 15 * 16, (test_read_rate_characters * 8), 16},
        "",
    };

    ///////////////////////////////////////////////////////////////////////

    Button button_test{
        {16, 17 * 16, 96, 24},
        "Test"};

    Button button_ok{
        {screen_width - 96 - 16, 17 * 16, 96, 24},
        "OK"};
};

} /* namespace ui */

#endif /*__UI_SD_CARD_DEBUG_H__*/
