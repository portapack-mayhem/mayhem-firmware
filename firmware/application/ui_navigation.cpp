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

#include "ui_navigation.hpp"
//#include "ui_loadmodule.hpp"

#include "modules.h"

#include "portapack.hpp"
#include "event_m0.hpp"
#include "portapack_persistent_memory.hpp"
#include "bmp_splash.hpp"
#include "bmp_modal_warning.hpp"

#include "ui_about.hpp"
#include "ui_setup.hpp"
#include "ui_debug.hpp"

#include "ui_numbers.hpp"
#include "ui_whipcalc.hpp"
//#include "ui_closecall.hpp"		// DEBUG
#include "ui_freqman.hpp"
#include "ui_nuoptix.hpp"
#include "ui_soundboard.hpp"
#include "ui_encoders.hpp"
#include "ui_rds.hpp"
#include "ui_xylos.hpp"
#include "ui_epar.hpp"
#include "ui_lcr.hpp"
#include "analog_audio_app.hpp"
#include "ui_adsbtx.hpp"
#include "ui_jammer.hpp"
#include "ais_app.hpp"
#include "ert_app.hpp"
#include "tpms_app.hpp"
#include "pocsag_app.hpp"
#include "capture_app.hpp"

#include "ui_debug.hpp"
#include "core_control.hpp"

#include "file.hpp"
#include "png_writer.hpp"

namespace ui {

/* SystemStatusView ******************************************************/

SystemStatusView::SystemStatusView() {
	uint8_t cfg;
	
	add_children({ {
		&button_back,
		&title,
		&button_textentry,
		&button_camera,
		&button_sleep,
		&sd_card_status_view,
	} });
	
	cfg = portapack::persistent_memory::ui_config_textentry();
	
	if (!cfg)
		button_textentry.set_bitmap(&bitmap_keyboard);
	else
		button_textentry.set_bitmap(&bitmap_unistroke);

	button_back.on_select = [this](Button&){
		if( this->on_back ) {
			this->on_back();
		}
	};
	
	button_textentry.on_select = [this](ImageButton&) {
		this->on_textentry();
	};

	button_camera.on_select = [this](ImageButton&) {
		this->on_camera();
	};

	button_sleep.on_select = [this](ImageButton&) {
		DisplaySleepMessage message;
		EventDispatcher::send_message(message);
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

void SystemStatusView::on_textentry() {
	uint8_t cfg;
	
	cfg = portapack::persistent_memory::ui_config_textentry();
	portapack::persistent_memory::set_config_textentry(cfg ^ 1);
	
	if (!cfg)
		button_textentry.set_bitmap(&bitmap_unistroke);
	else
		button_textentry.set_bitmap(&bitmap_keyboard);
}

void SystemStatusView::on_camera() {
	const auto filename_stem = next_filename_stem_matching_pattern("SCR_????");
	if( filename_stem.empty() ) {
		return;
	}

	PNGWriter png;
	auto create_error = png.create(filename_stem + ".PNG");
	if( create_error.is_valid() ) {
		return;
	}

	for (int i=0; i<320; i++) {
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
	if( view() == modal_view ) {
		modal_view = nullptr;
	}

	// Can't pop last item from stack.
	if( view_stack.size() > 1 ) {
		free_view();

		view_stack.pop_back();

		update_view();
	}
}

void NavigationView::pop_modal() {
	if( view() == modal_view ) {
		modal_view = nullptr;
	}

	// Pop modal view and underlying app view
	if( view_stack.size() > 2 ) {
		free_view();
		view_stack.pop_back();
		free_view();
		view_stack.pop_back();

		update_view();
	}
}

void NavigationView::display_modal(
	const std::string& title,
	const std::string& message
) {
	display_modal(title, message, INFO, nullptr);
}

void NavigationView::display_modal(
	const std::string& title,
	const std::string& message,
	const modal_t type,
	const std::function<void(bool)> on_choice
) {
	/* If a modal view is already visible, don't display another */
	if( !modal_view ) {
		modal_view = push<ModalMessageView>(title, message, type, on_choice);
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
	add_items<4>({ {
		{ "ADS-B: Planes", 			ui::Color::grey(),		[&nav](){ nav.push<NotImplementedView>(); } },
		{ "AIS:   Boats", 			ui::Color::white(),		[&nav](){ nav.push<AISAppView>(); } },
		{ "ERT:   Utility Meters", 	ui::Color::white(), 	[&nav](){ nav.push<ERTAppView>(); } },
		{ "TPMS:  Cars", 			ui::Color::white(),		[&nav](){ nav.push<TPMSAppView>(); } },
	} });
	on_left = [&nav](){ nav.pop(); };
}

/* ReceiverMenuView ******************************************************/

ReceiverMenuView::ReceiverMenuView(NavigationView& nav) {
	add_items<6>({ {
	//	{ "AFSK", 					ui::Color::grey(),  	[&nav](){ nav.push<NotImplementedView>(); } }, // AFSKRXView
		{ "Audio", 					ui::Color::green(),		[&nav](){ nav.push<AnalogAudioView>(); } },
		{ "CCIR", 					ui::Color::grey(),  	[&nav](){ nav.push<NotImplementedView>(); } }, // XylosRXView
		{ "Nordic/BTLE", 			ui::Color::grey(),		[&nav](){ nav.push<NotImplementedView>(); } },
		{ "POCSAG 1200", 			ui::Color::cyan(),		[&nav](){ nav.push<POCSAGAppView>(); } },
		{ "SIGFOX", 				ui::Color::grey(),  	[&nav](){ nav.push<NotImplementedView>(); } }, // SIGFRXView
		{ "Transponders", 			ui::Color::green(),		[&nav](){ nav.push<TranspondersMenuView>(); } },
	} });
	on_left = [&nav](){ nav.pop(); };
}

/* TransmitterCodedMenuView ******************************************************/

TransmitterCodedMenuView::TransmitterCodedMenuView(NavigationView& nav) {
	add_items<7>({ {
		{ "ADS-B Mode S", 			ui::Color::yellow(),  	[&nav](){ nav.push<ADSBTxView>(); } },
		{ "BHT Epar", 				ui::Color::grey(),  	[&nav](){ nav.push<NotImplementedView>(); } },
		{ "BHT Xylos", 				ui::Color::yellow(),  	[&nav](){ nav.push<XylosView>(); } },
		{ "Nuoptix DTMF timecode", 	ui::Color::green(),		[&nav](){ nav.push<NuoptixView>(); } },
		{ "OOK remote encoders", 	ui::Color::green(),		[&nav](){ nav.push<EncodersView>(); } },
		{ "RDS",					ui::Color::orange(),	[&nav](){ nav.push<RDSView>(); } },
		{ "TEDI/LCR AFSK", 			ui::Color::green(),  	[&nav](){ nav.push<LCRView>(); } },
	} });
	on_left = [&nav](){ nav.pop(); };
}

/* TransmitterAudioMenuView ******************************************************/

TransmitterAudioMenuView::TransmitterAudioMenuView(NavigationView& nav) {
	add_items<4>({ {
		{ "Soundboard", 			ui::Color::green(),  	[&nav](){ nav.push<SoundBoardView>(); } },
		{ "Numbers station",		ui::Color::yellow(),	[&nav](){ nav.push<NumbersStationView>(); } },
		{ "Microphone", 			ui::Color::grey(),  	[&nav](){ nav.push<NotImplementedView>(); } },
		{ "Whistle", 				ui::Color::grey(),  	[&nav](){ nav.push<NotImplementedView>(); } },
	} });
	on_left = [&nav](){ nav.pop(); };
}

/* UtilitiesView *****************************************************************/

UtilitiesView::UtilitiesView(NavigationView& nav) {
	add_items<2>({ {
		{ "Whip antenna calculator",	ui::Color::green(), [&nav](){ nav.push<WhipCalcView>(); } },
		{ "Notepad",					ui::Color::grey(),	[&nav](){ nav.push<NotImplementedView>(); } },
	} });
	on_left = [&nav](){ nav.pop(); };
}
/* SystemMenuView ********************************************************/

SystemMenuView::SystemMenuView(NavigationView& nav) {
	add_items<11>({ {
		{ "Play dead",				ui::Color::red(),  		[&nav](){ nav.push<PlayDeadView>(false); } },
		{ "Receivers", 				ui::Color::cyan(),		[&nav](){ nav.push<ReceiverMenuView>(); } },
		{ "Capture",				ui::Color::cyan(),		[&nav](){ nav.push<CaptureAppView>(); } },
		{ "Code transmitters", 		ui::Color::green(),		[&nav](){ nav.push<TransmitterCodedMenuView>(); } },
		{ "Audio transmitters", 	ui::Color::green(),		[&nav](){ nav.push<TransmitterAudioMenuView>(); } },
		//{ "Close Call                RX",	ui::Color::cyan(),		[&nav](){ nav.push<CloseCallView>(); } },
		{ "Jammer", 				ui::Color::orange(),  	[&nav](){ nav.push<JammerView>(); } },
		{ "Frequency manager", 		ui::Color::white(),  	[&nav](){ nav.push<FreqManView>(); } },
		{ "Utilities",				ui::Color::purple(),	[&nav](){ nav.push<UtilitiesView>(); } },
		//{ "Analyze", 		ui::Color::white(),  	[&nav](){ nav.push<NotImplementedView>(); } },
		{ "Setup", 					ui::Color::white(),    	[&nav](){ nav.push<SetupMenuView>(); } },
		//{ "Debug", 					ui::Color::white(),    	[&nav](){ nav.push<DebugMenuView>(); } },
		{ "HackRF", 				ui::Color::white(),	   	[&nav](){ nav.push<HackRFFirmwareView>(); } },
		{ "About", 					ui::Color::white(),    	[&nav](){ nav.push<AboutView>(); } }
	} });
}

/* SystemView ************************************************************/

static constexpr ui::Style style_default {
	.font = ui::font::fixed_8x16,
	.background = ui::Color::black(),
	.foreground = ui::Color::white()
};

SystemView::SystemView(
	Context& context,
	const Rect parent_rect
) : View { parent_rect },
	context_(context)
{
	set_style(&style_default);

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
		//navigation_view.push<CloseCallView>();
		//navigation_view.push<HandWriteView>(debugtxt, 20);
		
		navigation_view.push<SystemMenuView>();
}

Context& SystemView::context() const {
	return context_;
}

/* HackRFFirmwareView ****************************************************/

HackRFFirmwareView::HackRFFirmwareView(NavigationView& nav) {
	button_yes.on_select = [](Button&){
		EventDispatcher::request_stop();
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
	portapack::display.drawBMP({(240-185)/2, 0}, splash_bmp, false);
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

/* ModalMessageView ******************************************************/
ModalMessageView::ModalMessageView(
	NavigationView& nav,
	const std::string& title,
	const std::string& message,
	const modal_t type,
	const std::function<void(bool)> on_choice
) : title_ { title },
	type_ { type },
	on_choice_ { on_choice }
{

	add_child(&text_message);

	if (type == INFO) {
		add_child(&button_ok);
		
		button_ok.on_select = [&nav](Button&){
			nav.pop();
		};
	} else if (type == YESNO) {
		add_children({ {
			&button_yes,
			&button_no
		} });
		
		button_yes.on_select = [this, &nav](Button&){
			if (on_choice_) on_choice_(true);
			nav.pop();
		};
		button_no.on_select = [this, &nav](Button&){
			if (on_choice_) on_choice_(false);
			nav.pop();
		};
	} else {
		add_child(&button_ok);
		
		button_ok.on_select = [this, &nav](Button&){
			if (on_choice_) on_choice_(true);
			nav.pop_modal();
		};
	}
	
	const int text_message_width = message.size() * 8;
 	text_message.set_parent_rect({
 		(240 - text_message_width) / 2, 8 * 16,
 		text_message_width, 16
 	});
	text_message.set(message);
}

void ModalMessageView::paint(Painter& painter) {
	(void)painter;
	portapack::display.drawBMP({96, 64}, modal_warning_bmp, false);
}

void ModalMessageView::focus() {
	if (type_ == YESNO) {
		button_yes.focus();
	} else {
		button_ok.focus();
	}
}

} /* namespace ui */
