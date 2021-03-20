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
#include "freqman.hpp"

namespace ui {

	class FreqmanSetupViewFreq : public View {
		public:
			FreqmanSetupViewFreq( NavigationView& nav, Rect parent_rect );
			void focus() override;

		private:
			Text text_input_file {
				{ 1 * 8 , 1 * 16, 18 * 8, 22 },  
					"Frequency"
			};

			ButtonWithEncoder button_freq {
				{ 1 * 8, 2 * 16, 11 * 8, 22 },
					""
			};
	};

	class FreqmanSetupViewRange : public View {
		public:
			FreqmanSetupViewRange( Rect parent_rect );
			void focus() override;
		private:
			ButtonWithEncoder button_freq_start {
				{ 0 * 8, 11 * 16, 11 * 8, 28 },
					""
			};
			ButtonWithEncoder button_freq_end {
				{ 0 * 8, 11 * 16, 11 * 8, 28 },
					""
			};
	};

	class FreqmanSetupViewHamRadio : public View {
		public:
			FreqmanSetupViewHamRadio( Rect parent_rect );
			void focus() override;
		private:
			ButtonWithEncoder button_freq_rx {
				{ 0 * 8, 11 * 16, 11 * 8, 28 },
					""
			};
			ButtonWithEncoder button_freq_tx {
				{ 0 * 8, 11 * 16, 11 * 8, 28 },
					""
			};
	};

	class FreqmanSaveView : public View {
		public:
			FreqmanSetupView( NavigationView& nav , std::string _input_file , std::string _output_file );

			std::function<void( std::vector<std::string> messages )> on_changed { };

			void focus() override;

			std::string title() const override { return "Search setup"; };

		private:
			NavigationView& nav_;

			std::string input_file  = { "SEARCH" };
			std::string output_file = { "SEARCHRESULTS" };

			Rect view_rect = { 0, 3 * 8, 240, 230 };

			FreqmanSetupViewFreq viewFreq{ nav_ , view_rect , input_file , output_file };
			FreqmanSetupViewRange viewRange{ view_rect };
			FreqmanSetupViewHamRadio viewHamRadio{ view_rect };

			TabView tab_view {
				{ "Freq", Color::cyan() , &viewFreq },
				{ "Range", Color::green(), &viewRange },
				{ "HamRadio", Color::green(), &viewHamRadio }
			};
			Button button_save {
				{ 9 * 8, 255, 14 * 8 , 40 },
					"SAVE"
			};
	};

} /* namespace ui */
