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

#include "ui_afsksetup.hpp"
#include "ui_receiver.hpp"

#include "portapack.hpp"
#include "string_format.hpp"

#include "portapack_shared_memory.hpp"
#include "portapack_persistent_memory.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;
using namespace afsk;

namespace ui {

void AFSKSetupView::focus() {
	button_setfreq.focus();
}

void AFSKSetupView::update_freq(rf::Frequency f) {
	char finalstr[10] = {0};
	
	portapack::persistent_memory::set_tuned_frequency(f);
	
	auto mhz = to_string_dec_int(f / 1000000, 4);
	auto hz100 = to_string_dec_int((f / 100) % 10000, 4, '0');

	strcat(finalstr, mhz.c_str());
	strcat(finalstr, ".");
	strcat(finalstr, hz100.c_str());

	this->button_setfreq.set_text(finalstr);
}

AFSKSetupView::AFSKSetupView(
	NavigationView& nav
)
{
	using name_t = std::string;
	using value_t = int32_t;
	using option_t = std::pair<name_t, value_t>;
	using options_t = std::vector<option_t>;
	options_t format_options;
	uint8_t rpt;
	size_t i;
	
	add_children({ {
		&text_setfreq,
		&button_setfreq,
		&text_bps,
		&options_bps,
		&text_mark,
		&field_mark,
		&text_space,
		&field_space,
		&text_bw,
		&field_bw,
		&text_repeat,
		&field_repeat,
		&text_format,
		&options_format,
		&button_save
	} });
	
	for (i = 0; i < AFSK_MODES_COUNT; i++)
		format_options.emplace_back(std::make_pair(afsk_formats[i].fullname, i));
	
	options_format.set_options(format_options);
	options_format.set_selected_index(portapack::persistent_memory::afsk_format());
	
	update_freq(portapack::persistent_memory::tuned_frequency());
	
	field_mark.set_value(portapack::persistent_memory::afsk_mark_freq() * 25);
	field_space.set_value(portapack::persistent_memory::afsk_space_freq() * 25);
	field_bw.set_value(portapack::persistent_memory::afsk_bw());
	rpt = portapack::persistent_memory::afsk_repeats();
	if ((rpt > 99) || (!rpt)) rpt = 5;
	field_repeat.set_value(rpt);
	
	button_setfreq.on_select = [this,&nav](Button&) {
		auto new_view = nav.push<FrequencyKeypadView>(portapack::persistent_memory::tuned_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			update_freq(f);
		};
	};
	
	options_bps.set_by_value(portapack::persistent_memory::afsk_bitrate());

	button_save.on_select = [this,&nav](Button&) {
		uint32_t afsk_config = 0;
		
		portapack::persistent_memory::set_afsk_bitrate(options_bps.selected_index_value());
		portapack::persistent_memory::set_afsk_mark(field_mark.value() / 25);
		portapack::persistent_memory::set_afsk_space(field_space.value() / 25);
		portapack::persistent_memory::set_afsk_bw(field_bw.value());
		
		afsk_config |= (options_format.selected_index() << 16);
		afsk_config |= (field_repeat.value() << 24);
		portapack::persistent_memory::set_afsk_config(afsk_config);
		
		nav.pop();
	};
}

} /* namespace ui */
