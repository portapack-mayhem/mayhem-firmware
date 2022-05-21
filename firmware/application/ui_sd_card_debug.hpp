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
	SignalToken sd_card_status_signal_token { };

	void on_status(const sd_card::Status status);
	void on_test();

	Text text_title {
		{ (240 - (7 * 8)) / 2, 1 * 16, (7 * 8), 16 },
		"SD Card",
	};

	Text text_csd_title {
		{ 0, 3 * 16, (8 * 8), 16 },
		"CSD",
	};

	Text text_csd_value_3 {
		{ 240 - ((8 + 1 + 8) * 8), 3 * 16, (8 * 8), 16 },
		"",
	};

	Text text_csd_value_2 {
		{ 240 - (8 * 8), 3 * 16, (8 * 8), 16 },
		"",
	};

	Text text_csd_value_1 {
		{ 240 - ((8 + 1 + 8) * 8), 4 * 16, (8 * 8), 16 },
		"",
	};

	Text text_csd_value_0 {
		{ 240 - (8 * 8), 4 * 16, (8 * 8), 16 },
		"",
	};

	static constexpr size_t bus_width_characters = 1;

	Text text_bus_width_title {
		{ 0, 5 * 16, (9 * 8), 16 },
		"Bus width",
	};

	Text text_bus_width_value {
		{ 240 - (bus_width_characters * 8), 5 * 16, (bus_width_characters * 8), 16 },
		"",
	};

	static constexpr size_t card_type_characters = 13;

	Text text_card_type_title {
		{ 0, 6 * 16, (9 * 8), 16 },
		"Card type",
	};

	Text text_card_type_value {
		{ 240 - (card_type_characters * 8), 6 * 16, (card_type_characters * 8), 16 },
		"",
	};

	static constexpr size_t block_size_characters = 5;

	Text text_block_size_title {
		{ 0, 8 * 16, (10 * 8), 16 },
		"Block size",
	};

	Text text_block_size_value {
		{ 240 - (block_size_characters * 8), 8 * 16, (block_size_characters * 8), 16 },
		"",
	};

	static constexpr size_t block_count_characters = 9;

	Text text_block_count_title {
		{ 0, 9 * 16, (11 * 8), 16 },
		"Block count",
	};

	Text text_block_count_value {
		{ 240 - (block_count_characters * 8), 9 * 16, (block_count_characters * 8), 16 },
		"",
	};

	static constexpr size_t capacity_characters = 10;

	Text text_capacity_title {
		{ 0, 10 * 16, (8 * 8), 16 },
		"Capacity",
	};

	Text text_capacity_value {
		{ 240 - (capacity_characters * 8), 10 * 16, (capacity_characters * 8), 16 },
		"",
	};

	///////////////////////////////////////////////////////////////////////

	static constexpr size_t test_write_time_characters = 23;

	Text text_test_write_time_title {
		{ 0, 12 * 16, (4 * 8), 16 },
		"W ms",
	};

	Text text_test_write_time_value {
		{ 240 - (test_write_time_characters * 8), 12 * 16, (test_write_time_characters * 8), 16 },
		"",
	};

	static constexpr size_t test_write_rate_characters = 23;

	Text text_test_write_rate_title {
		{ 0, 13 * 16, (6 * 8), 16 },
		"W MB/s",
	};

	Text text_test_write_rate_value {
		{ 240 - (test_write_rate_characters * 8), 13 * 16, (test_write_rate_characters * 8), 16 },
		"",
	};

	///////////////////////////////////////////////////////////////////////

	static constexpr size_t test_read_time_characters = 23;

	Text text_test_read_time_title {
		{ 0, 14 * 16, (4 * 8), 16 },
		"R ms",
	};

	Text text_test_read_time_value {
		{ 240 - (test_read_time_characters * 8), 14 * 16, (test_read_time_characters * 8), 16 },
		"",
	};

	static constexpr size_t test_read_rate_characters = 23;

	Text text_test_read_rate_title {
		{ 0, 15 * 16, (6 * 8), 16 },
		"R MB/s",
	};

	Text text_test_read_rate_value {
		{ 240 - (test_read_rate_characters * 8), 15 * 16, (test_read_rate_characters * 8), 16 },
		"",
	};

	///////////////////////////////////////////////////////////////////////

	Button button_test {
		{ 16, 17 * 16, 96, 24 },
		"Test"
	};

	Button button_ok {
		{ 240 - 96 - 16, 17 * 16, 96, 24 },
		"OK"
	};
};

} /* namespace ui */

#endif/*__UI_SD_CARD_DEBUG_H__*/
