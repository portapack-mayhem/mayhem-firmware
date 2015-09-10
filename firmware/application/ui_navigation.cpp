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

#include "receiver_model.hpp"
#include "transmitter_model.hpp"
#include "portapack_persistent_memory.hpp"

#include "splash.hpp"

#include "ui_setup.hpp"
#include "ui_debug.hpp"
#include "ui_receiver.hpp"
#include "ui_rds.hpp"
#include "ui_lcr.hpp"
#include "ui_whistle.hpp"

#include "portapack.hpp"
#include "m4_startup.hpp"
#include "spi_image.hpp"
using namespace portapack;

namespace ui {

/* SystemStatusView ******************************************************/

SystemStatusView::SystemStatusView() {
	add_children({ {
		&portapack,
	} });
}

/* Navigation ************************************************************/

NavigationView::NavigationView()
{
}

void NavigationView::push(View* new_view) {
	// TODO: Trap nullptr?
	// TODO: Trap push of object already on stack?
	view_stack.push_back(new_view);
	set_view(new_view);
}

void NavigationView::pop() {
	// Can't pop last item from stack.
	if( view_stack.size() > 1 ) {
		const auto old_view = view_stack.back();
		view_stack.pop_back();
		const auto new_view = view_stack.back();
		set_view(new_view);
		delete old_view;
	}
}

Widget* NavigationView::view() const {
	return children_.empty() ? nullptr : children_[0];
}

void NavigationView::set_view(Widget* const new_view) {
	const auto old_view = view();
	if( old_view ) {
		remove_child(old_view);
	}

	// TODO: Allow new_view == nullptr?!
	if( new_view ) {
		add_child(new_view);
		new_view->set_parent_rect({ {0, 0}, size() });
		focus();
	}

	set_dirty();
}

void NavigationView::focus() {
	if( view() ) {
		view()->focus();
	}
}

/* SystemMenuView ********************************************************/

SystemMenuView::SystemMenuView(NavigationView& nav) {
	add_items<10>({ {
		{ "Play dead", ui::Color::red(),  [&nav](){ nav.push(new PlayDeadView 		{ nav, false }); } },
		{ "Receiver", ui::Color::white(), [&nav](){ nav.push(new ReceiverView       { nav, receiver_model }); } },
		{ "Capture", ui::Color::white(),  [&nav](){ nav.push(new NotImplementedView { nav }); } },
		{ "Whistle", ui::Color::white(),  [&nav](){ nav.push(new WhistleView 		{ nav, transmitter_model }); } },
		{ "RDS TX", ui::Color::yellow(),  [&nav](){ nav.push(new RDSView            { nav, transmitter_model }); } },
		{ "LCR TX", ui::Color::orange(),  [&nav](){ nav.push(new LCRView            { nav, transmitter_model }); } },
		{ "Setup", ui::Color::white(),    [&nav](){ nav.push(new SetupMenuView      { nav }); } },
		{ "About", ui::Color::white(),    [&nav](){ nav.push(new AboutView          { nav }); } },
		{ "Debug", ui::Color::white(),    [&nav](){ nav.push(new DebugMenuView      { nav }); } },
		{ "HackRF", ui::Color::white(),   [&nav](){ nav.push(new HackRFFirmwareView { nav }); } },
	} });
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
	style_ = &style_default;

	constexpr ui::Dim status_view_height = 16;

	add_child(&status_view);
	status_view.set_parent_rect({
		{ 0, 0 },
		{ parent_rect.width(), status_view_height }
	});

	add_child(&navigation_view);
	navigation_view.set_parent_rect({
		{ 0, status_view_height },
		{ parent_rect.width(), static_cast<ui::Dim>(parent_rect.height() - status_view_height) }
	});

	// Initial view.
	// TODO: Restore from non-volatile memory?
	if (persistent_memory::playing_dead())
		navigation_view.push(new PlayDeadView { navigation_view, true });
	else
		navigation_view.push(new BMPView { navigation_view });
}

Context& SystemView::context() const {
	return context_;
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
		nav.push(new SystemMenuView { nav });
	};
}

void BMPView::paint(Painter& painter) {
	uint32_t pixel_data;
	uint8_t p, by, c, count;
	ui::Color linebuffer[185];
	ui::Coord px = 0, py = 302;
	ui::Color palette[16];
	
	// RLE_4 BMP loader with hardcoded size and no delta :(
	
	pixel_data = splash_bmp[0x0A];
	p = 0;
	for (c = 0x36; c < (0x36+(16*4)); c+=4) {
		palette[p++] = ui::Color(splash_bmp[c+2], splash_bmp[c+1], splash_bmp[c]);
	}
	
	do {
		by = splash_bmp[pixel_data++];
		if (by) {
			count = by;
			by = splash_bmp[pixel_data++];
			for (c = 0; c < count; c+=2) {
				linebuffer[px++] = palette[by >> 4];
				if (px < 185) linebuffer[px++] = palette[by & 15];
			}
			if (pixel_data & 1) pixel_data++;
		} else {
			by = splash_bmp[pixel_data++];
			if (by == 0) {
				portapack::display.render_line({27, py}, 185, linebuffer);
				py--;
				px = 0;
			} else if (by == 1) {
				break;
			} else if (by == 2) {
				// Delta
			} else {
				count = by;
				for (c = 0; c < count; c+=2) {
					by = splash_bmp[pixel_data++];
					linebuffer[px++] = palette[by >> 4];
					if (px < 185) linebuffer[px++] = palette[by & 15];
				}
				if (pixel_data & 1) pixel_data++;
			}
		}
	} while (1);
}

/* PlayDeadView **********************************************************/

void PlayDeadView::focus() {
	button_done.focus();
}

PlayDeadView::PlayDeadView(NavigationView& nav, bool booting) {
	_booting = booting;
	persistent_memory::set_playing_dead(1);
	
	add_children({ {
		&text_playdead1,
		&text_playdead2,
		&button_done,
	} });
	
	button_done.on_dir = [this,&nav](Button&, KeyEvent key){
		sequence = (sequence<<3) | static_cast<std::underlying_type<KeyEvent>::type>(key);
	};
	
	button_done.on_select = [this,&nav](Button&){
		if (sequence == persistent_memory::playdead_sequence()) {
			persistent_memory::set_playing_dead(0);
			if (_booting) {
				nav.pop();
				nav.push(new SystemMenuView { nav });
			} else {
				nav.pop();
			}
		} else {
			sequence = 0;
		}
	};
}

/* HackRFFirmwareView ****************************************************/

HackRFFirmwareView::HackRFFirmwareView(NavigationView& nav) {
	button_yes.on_select = [&nav](Button&){
		shutdown();

		m4_init(spi_flash::hackrf, reinterpret_cast<void*>(0x10000000));

		while(true) {
			__WFE();
		}
	};
	
	//377.6M: bouts de musique

	button_no.on_select = [&nav](Button&){
		nav.pop();
	};

	add_children({ {
		&text_title,
		&button_yes,
		&button_no,
	} });
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
