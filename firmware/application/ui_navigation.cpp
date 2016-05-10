/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "ui_navigation.hpp"
#include "ui_loadmodule.hpp"

#include "modules.h"

#include "portapack.hpp"
#include "event_m0.hpp"
#include "portapack_persistent_memory.hpp"
#include "splash.hpp"

#include "ui_about.hpp"
#include "ui_setup.hpp"
#include "ui_debug.hpp"

#include "ui_handwrite.hpp"			// DEBUG
#include "ui_soundboard.hpp"		// DEBUG

#include "analog_audio_app.hpp"
#include "ais_app.hpp"
#include "ert_app.hpp"
#include "tpms_app.hpp"
#include "capture_app.hpp"

#include "core_control.hpp"

#include "file.hpp"
#include "png_writer.hpp"

namespace ui {

/* SystemStatusView ******************************************************/

SystemStatusView::SystemStatusView() {
	add_children({ {
		&button_back,
		&title,
		&button_camera,
		&button_sleep,
		&sd_card_status_view,
	} });

	button_back.on_select = [this](Button&){
		if( this->on_back ) {
			this->on_back();
		}
	};

	button_camera.on_select = [this](ImageButton&) {
		this->on_camera();
	};

	button_sleep.on_select = [this](ImageButton&) {
		DisplaySleepMessage message;
		EventDispatcher::message_map().send(&message);
	};
}

void SystemStatusView::set_back_enabled(bool new_value) {
	button_back.set_text(new_value ? back_text_enabled : back_text_disabled);
	button_back.set_focusable(new_value);
}

void SystemStatusView::set_title(const std::string new_value) {
	if( new_value.empty() ) {
		title.set(default_title);
	} else {
		title.set(new_value);
	}
}

void SystemStatusView::on_camera() {
	const auto filename_stem = next_filename_stem_matching_pattern("SCR_????");
	if( filename_stem.empty() ) {
		return;
	}

	PNGWriter png { filename_stem + ".PNG" };

	for(int i=0; i<320; i++) {
		std::array<ColorRGB888, 240> row;
		portapack::display.read_pixels({ 0, i, 240, 1 }, row);
		png.write_scanline(row);
	}
}

/* Navigation ************************************************************/

bool NavigationView::is_top() const {
	return view_stack.size() == 1;
}

View* NavigationView::push_view(std::unique_ptr<View> new_view) {
	free_view();

	const auto p = new_view.get();
	view_stack.emplace_back(std::move(new_view));

	update_view();

	return p;
}

void NavigationView::pop() {
	// Can't pop last item from stack.
	if( view_stack.size() > 1 ) {
		free_view();

		view_stack.pop_back();

		update_view();
	}
}

void NavigationView::free_view() {
	remove_child(view());
}

void NavigationView::update_view() {
	const auto new_view = view_stack.back().get();
	add_child(new_view);
	new_view->set_parent_rect({ {0, 0}, size() });
	focus();
	set_dirty();

	if( on_view_changed ) {
		on_view_changed(*new_view);
	}
}

Widget* NavigationView::view() const {
	return children_.empty() ? nullptr : children_[0];
}

void NavigationView::focus() {
	if( view() ) {
		view()->focus();
	}
}

/* TranspondersMenuView **************************************************/

TranspondersMenuView::TranspondersMenuView(NavigationView& nav) {
	add_items<3>({ {
		{ "AIS:  Boats", ui::Color::white(),          [&nav](){ nav.push<AISAppView>(); } },
		{ "ERT:  Utility Meters", ui::Color::white(), [&nav](){ nav.push<ERTAppView>(); } },
		{ "TPMS: Cars", ui::Color::white(),           [&nav](){ nav.push<TPMSAppView>(); } },
	} });
	on_left = [&nav](){ nav.pop(); };
}

/* ReceiverMenuView ******************************************************/

ReceiverMenuView::ReceiverMenuView(NavigationView& nav) {
	add_items<2>({ {
		{ "Audio", ui::Color::white(),        		[&nav](){ nav.push<LoadModuleView>(md5_baseband, 10, true); } },
		{ "Transponders", ui::Color::white(), 		[&nav](){ nav.push<TranspondersMenuView>(); } },
	} });
	on_left = [&nav](){ nav.pop(); };
}

/* SystemMenuView ********************************************************/

SystemMenuView::SystemMenuView(NavigationView& nav) {
	add_items<10>({ {
		{ "Play dead",		ui::Color::red(),  		[&nav](){ nav.push<PlayDeadView>(false); } },
		{ "Receiver                  RX", 	ui::Color::cyan(),		[&nav](){ nav.push<ReceiverMenuView>(); } },
		{ "Soundboard                TX", 	ui::Color::orange(),  	[&nav](){ nav.push<LoadModuleView>(md5_baseband_tx, 3, true); } },
		{ "Audio TX                  TX", 	ui::Color::yellow(),  	[&nav](){ nav.push<LoadModuleView>(md5_baseband_tx, 0, true); } },
		{ "Xylos                     TX", 	ui::Color::yellow(),  	[&nav](){ nav.push<LoadModuleView>(md5_baseband_tx, 1, true); } },
		{ "TEDI/LCR                  TX", 	ui::Color::yellow(),  	[&nav](){ nav.push<LoadModuleView>(md5_baseband_tx, 2, true); } },
		//{ "Capture", 		ui::Color::white(), 	[&nav](){ nav.push<NotImplementedView>(); } },
		//{ "Analyze", 		ui::Color::white(),  	[&nav](){ nav.push<NotImplementedView>(); } },
		{ "Setup", 			ui::Color::white(),    	[&nav](){ nav.push<SetupMenuView>(); } },
		{ "About", 			ui::Color::white(),    	[&nav](){ nav.push<AboutView>(); } },
		{ "Debug", 			ui::Color::white(),    	[&nav](){ nav.push<DebugMenuView>(); } },
		{ "HackRF", 		ui::Color::white(),	   	[&nav](){ nav.push<HackRFFirmwareView>(); } },
	} });

		//{ "Nordic/BTLE RX", ui::Color::cyan(),	[&nav](){ nav.push(new NotImplementedView { nav }); } },
		//{ "Jammer", ui::Color::white(),   		[&nav](){ nav.push<LoadModuleView>(md5_baseband, new JammerView(nav)); } },
		//{ "Audio file TX", ui::Color::white(),	[&nav](){ nav.push(new NotImplementedView { nav }); } },
		//{ "Encoder TX", ui::Color::green(),		[&nav](){ nav.push(new NotImplementedView { nav }); } },
		//{ "Whistle", ui::Color::purple(),  		[&nav](){ nav.push(new LoadModuleView { nav, md5_baseband, new WhistleView { nav, transmitter_model }}); } },
		//{ "SIGFOX RX", ui::Color::orange(),  		[&nav](){ nav.push(new LoadModuleView { nav, md5_baseband, new SIGFRXView 		  { nav, receiver_model }}); } },
		//{ "Xylos RX", ui::Color::green(),  		[&nav](){ nav.push(new LoadModuleView { nav, md5_baseband_tx, new XylosRXView	{ nav, receiver_model }}); } },
		//{ "AFSK RX", ui::Color::cyan(),  			[&nav](){ nav.push(new LoadModuleView { nav, md5_baseband, new AFSKRXView         { nav, receiver_model }}); } },
		//{ "Numbers station", ui::Color::purple(),	[&nav](){ nav.push(new LoadModuleView { nav, md5_baseband_tx, new NumbersStationView { nav, transmitter_model }}); } },

}

/* SystemView ************************************************************/

static constexpr ui::Style style_default {
	.font = ui::font::fixed_8x16,
	.background = ui::Color::black(),
	.foreground = ui::Color::white(),
};

SystemView::SystemView(
	Context& context,
	const Rect parent_rect
) : View { parent_rect },
	context_(context)
{
	set_style(&style_default);
	
	char debugtxt[21];	// DEBUG

	constexpr ui::Dim status_view_height = 16;
	
	add_child(&status_view);
	status_view.set_parent_rect({
		{ 0, 0 },
		{ parent_rect.width(), status_view_height }
	});
	status_view.on_back = [this]() {
		this->navigation_view.pop();
	};

	add_child(&navigation_view);
	navigation_view.set_parent_rect({
		{ 0, status_view_height },
		{ parent_rect.width(), static_cast<ui::Dim>(parent_rect.height() - status_view_height) }
	});
	navigation_view.on_view_changed = [this](const View& new_view) {
		this->status_view.set_back_enabled(!this->navigation_view.is_top());
		this->status_view.set_title(new_view.title());
	};

	// Initial view.
	// TODO: Restore from non-volatile memory?
	
	//if (persistent_memory::playing_dead() == 0x59)
	//	navigation_view.push(new PlayDeadView { navigation_view, true });
	//else
	//	navigation_view.push(new BMPView { navigation_view });
	
	if (portapack::persistent_memory::ui_config() & 1)
		navigation_view.push<BMPView>();
	else
		//navigation_view.push<SoundBoardView>();
		//navigation_view.push<HandWriteView>(debugtxt, 20);
		navigation_view.push<SystemMenuView>();
}

Context& SystemView::context() const {
	return context_;
}

/* HackRFFirmwareView ****************************************************/

HackRFFirmwareView::HackRFFirmwareView(NavigationView& nav) {
	button_yes.on_select = [&nav](Button&){
		m4_request_shutdown();
	};

	button_no.on_select = [&nav](Button&){
		nav.pop();
	};

	add_children({ {
		&text_title,
		&text_description_1,
		&text_description_2,
		&text_description_3,
		&text_description_4,
		&button_yes,
		&button_no,
	} });
}

/* ***********************************************************************/

void BMPView::focus() {
	button_done.focus();
}

BMPView::BMPView(NavigationView& nav) {
	add_children({ {
		&text_info,
		&button_done
	} });
	
	button_done.on_select = [this,&nav](Button&){
		nav.pop();
		nav.push<SystemMenuView>();
	};
}

void BMPView::paint(Painter& painter) {
	(void)painter;
	portapack::display.drawBMP({(240-185)/2, 0}, splash_bmp);
}

/* PlayDeadView **********************************************************/

void PlayDeadView::focus() {
	button_done.focus();
}

PlayDeadView::PlayDeadView(NavigationView& nav, bool booting) {
	_booting = booting;
	portapack::persistent_memory::set_playing_dead(0x59);
	
	add_children({ {
		&text_playdead1,
		&text_playdead2,
		&button_done,
	} });
	
	button_done.on_dir = [this,&nav](Button&, KeyEvent key){
		sequence = (sequence<<3) | static_cast<std::underlying_type<KeyEvent>::type>(key);
	};
	
	button_done.on_select = [this,&nav](Button&){
		if (sequence == portapack::persistent_memory::playdead_sequence()) {
			portapack::persistent_memory::set_playing_dead(0);
			if (_booting) {
				nav.pop();
				nav.push<SystemMenuView>();
			} else {
				nav.pop();
			}
		} else {
			sequence = 0;
		}
	};
}

void HackRFFirmwareView::focus() {
	button_no.focus();
}

/* NotImplementedView ****************************************************/

NotImplementedView::NotImplementedView(NavigationView& nav) {
	button_done.on_select = [&nav](Button&){
		nav.pop();
	};

	add_children({ {
		&text_title,
		&button_done,
	} });
}

void NotImplementedView::focus() {
	button_done.focus();
}

} /* namespace ui */
