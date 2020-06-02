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

#include "ui_sd_wipe.hpp"

namespace ui {

Thread* WipeSDView::thread { nullptr };

WipeSDView::WipeSDView(NavigationView& nav) : nav_ (nav) {
	add_children({
		&text_info,
		&progress,
		&dummy
	});
}

WipeSDView::~WipeSDView() {
	if (thread)
		chThdTerminate(thread);
}

void WipeSDView::focus() {
	BlockDeviceInfo block_device_info;
	
	dummy.focus();
	
	if (!confirmed) {
		nav_.push<ModalMessageView>("Warning !", "Wipe FAT of SD card?", YESCANCEL, [this](bool choice) {
				if (choice)
					confirmed = true;
			}
		);
	} else {
		if (sdcGetInfo(&SDCD1, &block_device_info) == CH_SUCCESS) {
			thread = chThdCreateFromHeap(NULL, 2048, NORMALPRIO, WipeSDView::static_fn, this);
		} else {
			nav_.pop();		// Just silently abort for now
		}
	}
}

} /* namespace ui */
