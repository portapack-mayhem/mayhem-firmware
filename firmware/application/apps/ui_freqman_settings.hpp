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

	class FreqmanEditFreq : public View {
		public:
			FreqmanSetupViewFreq( NavigationView& nav, Rect parent_rect );
			void focus() override;

		private:
			Text text_entry_type {
				{ 1 * 8 , 1 * 16, 18 * 8, 22 },  
					"Frequency"
			};

			ButtonWithEncoder button_freq {
				{ 1 * 8, 2 * 16, 11 * 8, 22 },
					""
			};
			OptionsField field_mode {
				{ 5 * 8, (26 * 8) + 4 },
					6,
					{
					}
			};
			OptionsField field_bw {
				{ 3 * 8, 1 * 16 },
					4,
					{ }
			};
	};

	class FreqmanEditView : public View {
		public:
			FreqmanEditView( NavigationView& nav , freqman_entry &_rf_entry );

			std::function<void( std::vector<std::string> messages )> on_changed { };

			void focus() override;

			std::string title() const override { return "Freqman Edit View"; };

		private:
			NavigationView& nav_;

			freqman_entry entry ;

  			RFFreqmanSetupViewFreq viewFreq{ nav_ , view_rect , entry , output_file };
			FreqmanSetupViewRange viewRange{ view_rect };
			FreqmanSetupViewHamRadio viewHamRadio{ view_rect };

			TabView tab_view {
				{ "Freq", Color::red() , &viewFreq },
				{ "Range", Color::green(), &viewRange },
				{ "HamRadio", Color::blue(), &viewHamRadio }
			};
			Button button_save {
				{ 9 * 8, 255, 14 * 8 , 40 },
					"SAVE"
			};
	};

} /* namespace ui */
