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

#include "serializer.hpp"
#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_tabview.hpp"
#include "ui_navigation.hpp"
#include "string_format.hpp"

namespace ui {
	
bool SearchAppSetupLoadStrings( std::string source, std::string &input_file , std::string &output_file );
bool SearchAppSetupSaveStrings( std::string dest, const std::string input_file , const std::string output_file );

class SearchAppSetupViewMain : public View {
public:
	SearchAppSetupViewMain( NavigationView& nav, Rect parent_rect , std::string input_file , std::string output_file );
	void Save( std::string &input_file , std::string &output_file );
	void focus() override;
	
private:
	std::string _input_file  = { "SEARCH" };
	std::string _output_file = { "SEARCHRESULTS" };
	
	
	Button button_load_freqs {
		{ 1 * 8 , 12 , 18 * 8 , 22 },
		"select input file"
	};
	Text text_input_file {
		{ 1 * 8 , 4 + 2 * 16, 18 * 8, 22 },  
		"SEARCH"
	};

	Button button_save_freqs {
		{ 1 * 8 , 4 * 16 - 8 , 18 * 8 , 22 },
		"select output file"
	};
	Button button_output_file {
		{ 1 * 8 , 5 * 16 - 2, 18 * 8, 22 },  
		"SEARCHRESULTS"	
	};

	Checkbox checkbox_autosave_freqs {
		{ 1 * 8, 7 * 16 - 4 },
		3,
		"autosave freqs"
	};

	Checkbox checkbox_autostart_search {
		{ 1 * 8, 9 * 16 - 4 },
		3,
		"autostart searching"
	};

	Checkbox checkbox_continuous {
		{ 1 * 8, 11 * 16 - 4 },
		3,
		"continuous"
	};
	Checkbox checkbox_clear_output {
		{ 1 * 8, 13 * 16 - 4 },
		3,
		"clear output at start"
	};
};

class SearchAppSetupViewMore : public View {
public:
	SearchAppSetupViewMore( Rect parent_rect );
	void Save();
	
	void focus() override;
	
private:
	Checkbox checkbox_load_freqs {
		{ 1 * 8, 12 },
		3,
		"input: load freqs"
	};

	Checkbox checkbox_load_ranges {
		{ 1 * 8, 42 },
		3,
		"input: load ranges"
	};
	Checkbox checkbox_update_ranges_when_searching {
		{ 1 * 8, 72 },
		3,
		"auto update m-ranges"
		};
};




class SearchAppSetupView : public View {
public:
	SearchAppSetupView( NavigationView& nav , std::string _input_file , std::string _output_file );
	
	std::function<void( std::vector<std::string> messages )> on_changed { };
	
	void focus() override;
	
	std::string title() const override { return "Search setup"; };

private:
	NavigationView& nav_;
	
	std::string input_file  = { "SEARCH" };
	std::string output_file = { "SEARCHRESULTS" };
	
	Rect view_rect = { 0, 3 * 8, 240, 230 };

	SearchAppSetupViewMain viewMain{ nav_ , view_rect , input_file , output_file };
	SearchAppSetupViewMore viewMore{ view_rect };

	TabView tab_view {
		{ "Main", Color::cyan() , &viewMain },
		{ "More", Color::green(), &viewMore }
	};
	Button button_save {
		{ 9 * 8, 255, 14 * 8 , 40 },
		"SAVE"
	};
};

} /* namespace ui */
