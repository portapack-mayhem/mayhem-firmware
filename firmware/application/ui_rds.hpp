/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "ui.hpp"
#include "ui_menu.hpp"
#include "ui_navigation.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "ui_receiver.hpp"
#include "ui_textentry.hpp"
#include "message.hpp"

namespace ui {

class RDSView : public View {
public:
	RDSView(NavigationView& nav);
	~RDSView();

	void focus() override;
	void paint(Painter& painter) override;

	std::string title() const override { return "RDS transmit"; };

private:
	char PSN[9];
	char RadioText[25];
	bool txing = false;
	
	uint16_t message_length;
	
	void start_tx();
	void on_tuning_frequency_changed(rf::Frequency f);

	FrequencyField field_frequency {
		{ 1 * 8, 1 * 16 },
	};
	
	OptionsField options_pty {
		{ 1 * 8, 2 * 16 },
		8,
		{
			{ "None", 0 },
			{ "News", 1 },
			{ "Affairs", 2 },
			{ "Info", 3 },
			{ "Sport", 4 },
			{ "Educate", 5 },
			{ "Drama", 6 },
			{ "Culture", 7 },
			{ "Science", 8 },
			{ "Varied", 9 },
			{ "Pop", 10 },
			{ "Rock", 11 },
			{ "Easy", 12 },
			{ "Light", 13 },
			{ "Classics", 14 },
			{ "Other", 15 },
			{ "Weather", 16 },
			{ "Finance", 17 },
			{ "Children", 18 },
			{ "Social", 19 },
			{ "Religion", 20 },
			{ "PhoneIn", 21 },
			{ "Travel", 22 },
			{ "Leisure", 23 },
			{ "Jazz", 24 },
			{ "Country", 25 },
			{ "National", 26 },
			{ "Oldies", 27 },
			{ "Folk", 28 },
			{ "Docs", 29 },
			{ "AlarmTst", 30 },
			{ "Alarm", 31 }
		}
	};
	
	OptionsField options_countrycode {
		{ 1 * 8, 3 * 16 },
		11,
		{
			{ "Albania", 	9 },
			{ "Algeria", 	2 },
			{ "Andorra", 	3 },
			{ "Austria", 	10 },
			{ "Azores", 	8 },
			{ "Belgium", 	6 },
			{ "Belarus", 	15 },
			{ "Bosnia", 	15 },
			{ "Bulgaria", 	8 },
			{ "Canaries", 	14 },
			{ "Croatia", 	12 },
			{ "Cyprus", 	2 },
			{ "Czech-Rep", 	2 },
			{ "Denmark", 	9 },
			{ "Egypt", 		15 },
			{ "Estonia", 	2 },
			{ "Faroe", 		9 },
			{ "Finland", 	6 },
			{ "France", 	15 },
			{ "Germany 1", 	13 },
			{ "Germany 2", 	1 },
			{ "Gibraltar", 	10 },
			{ "Greece", 	1 },
			{ "Hungary", 	11 },
			{ "Iceland", 	10 },
			{ "Iraq", 		11 },
			{ "Ireland", 	2 },
			{ "Israel", 	4 },
			{ "Italy", 		5 },
			{ "Jordan", 	5 },
			{ "Latvia", 	9 },
			{ "Lebanon", 	10 },
			{ "Libya", 		13 },
			{ "Liechtenst.", 9 },
			{ "Lithuania", 	12 },
			{ "Luxembourg", 7 },
			{ "Macedonia", 	4 },
			{ "Madeira", 	8 },
			{ "Malta", 		12 },
			{ "Moldova", 	1 },
			{ "Monaco", 	11 },
			{ "Morocco", 	1 },
			{ "Netherlands", 8 },
			{ "Norway", 	15 },
			{ "Palestine", 	8 },
			{ "Poland", 	3 },
			{ "Portugal", 	8 },
			{ "Romania", 	14 },
			{ "Russia", 	7 },
			{ "San Marino", 3 },
			{ "Slovakia", 	5 },
			{ "Slovenia", 	9 },
			{ "Spain", 		14 },
			{ "Sweden", 	14 },
			{ "Switzerland", 4 },
			{ "Syria", 		6 },
			{ "Tunisia", 	7 },
			{ "Turkey", 	3 },
			{ "Ukraine", 	6 },
			{ "U.K.", 		12 },
			{ "Vatican", 	4 },
			{ "Yugoslavia", 13 }
		}
	};
	
	OptionsField options_coverage {
		{ 1 * 8, 4 * 16 },
		13,
		{
			{ "Local", 0 },
			{ "International", 1 },
			{ "National", 2 },
			{ "Sup-regional", 3 },
			{ "R11", 4 },
			{ "R12", 5 },
			{ "R13", 6 },
			{ "R14", 7 },
			{ "R15", 8 },
			{ "R16", 9 },
			{ "R17", 10 },
			{ "R18", 11 },
			{ "R19", 12 },
			{ "R110", 13 },
			{ "R111", 14 },
			{ "R112", 15 }
		}
	};

	Text text_tx {
		{ 19 * 8, 4 * 16, 9 * 8, 16 },
		"Transmit:"
	};

	Button button_editpsn {
		{ 2 * 8, 6 * 16, 64, 24 },
		"Set"
	};
	Text text_psn {
		{ 2 * 8, 15 * 8, 8 * 8, 16 },
		"TEST1234"
	};
	Button button_txpsn {
		{ 19 * 8, 6 * 16, 10 * 8, 32 },
		"PSN"
	};

	Button button_editradiotext {
		{ 2 * 8, 9 * 16, 64, 24 },
		"Set"
	};
	Text text_radiotexta {
		{ 2 * 8, 21 * 8, 16 * 8, 16 },
		"Radiotext test !"
	};
	Text text_radiotextb {
		{ 2 * 8, 23 * 8, 16 * 8, 16 },
		"--------"
	};
	Button button_txradiotext {
		{ 19 * 8, 9 * 16, 10 * 8, 32 },
		"Radiotext"
	};
	
	Button button_exit {
		{ 72, 260, 96, 32 },
		"Exit"
	};
};

} /* namespace ui */
