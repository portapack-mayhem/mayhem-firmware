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
#include "ui_navigation.hpp"
#include "string_format.hpp"

namespace ui {
	
bool SearchSetupLoadStrings( std::string source, std::string &input_file , std::string &output_file );
bool SearchSetupSaveStrings( std::string dest, const std::string input_file , const std::string output_file );

class SearchSetupView : public View {
public:
	SearchSetupView(NavigationView& nav);
	
	
	void focus() override;
	
	std::string title() const override { return "Search setup"; };

private:
	NavigationView& nav_;

	std::string input_file ;
	std::string output_file ;

	Button button_load_freqs {
		{ 1 * 8 , 1 * 16 , 18 * 8 , 22 },
		"Input freqs File:"
	};
	Text text_input_file {
		{ 1 * 8 , 8 + 2 * 16, 20 * 8, 22 },  
		""
	};

	Button button_save_freqs {
		{ 1 * 8 , 4 * 16 , 18 * 8 , 22 },
		"Save freqs to   :"
	};
	Text text_output_file {
		{ 1 * 8 , 8 + 5 * 16, 20 * 8, 22 },  
		""	
	};

	Checkbox checkbox_autosave_freqs {
		{ 1 * 8, 7 * 16 },
		3,
		"Auto Save freqs"
	};

	Checkbox checkbox_autostart_search {
		{ 1 * 8, 9 * 16 },
		3,
		"Auto Start Search"
	};

	Checkbox checkbox_powersave {
		{ 1 * 8, 11 * 16 },
		3,
		"Slower GUI,more CPU power"
	};
	Checkbox checkbox_filemode {
		{ 1 * 8, 13 * 16 },
		3,
		"On:append, Off:overwrite"
	};

	Button button_save {
		{ 9 * 8, 250, 14 * 8 , 40 },
		"Save Settings"
	};
};

} /* namespace ui */
