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

#include "sd_card.hpp"

#include <hal.h>

#include "ff.h"

namespace sd_card {

FATFS fs;

namespace {

bool card_present = false;

Status status_ { Status::NotPresent };

FRESULT mount() {
	return f_mount(&fs, reinterpret_cast<const TCHAR*>(_T("")), 0);
}

} /* namespace */

Signal<Status> status_signal;

void poll_inserted() {
	const auto card_present_now = sdcIsCardInserted(&SDCD1);
	if( card_present_now != card_present ) {
		card_present = card_present_now;

		Status new_status { card_present ? Status::Present : Status::NotPresent };

		if( card_present ) {
			if( sdcConnect(&SDCD1) == CH_SUCCESS ) {
				if( mount() == FR_OK ) {
					new_status = Status::Mounted;
				} else {
					new_status = Status::MountError;
				}
			} else {
				new_status = Status::ConnectError;
			}
		} else {
			sdcDisconnect(&SDCD1);
		}

		status_ = new_status;
		status_signal.emit(status_);
	}
}

Status status() {
	return status_;
}

} /* namespace sd_card */
