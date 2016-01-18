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

#ifndef __TPMS_APP_H__
#define __TPMS_APP_H__

#include "ui_console.hpp"
#include "message.hpp"

#include "manchester.hpp"
#include "log_file.hpp"

namespace tpms {

using Packet = ManchesterDecoder;

} /* namespace tpms */

class TPMSLogger {
public:
	void on_packet(const Timestamp& timestamp, const tpms::Packet& packet);

private:
	LogFile log_file { "tpms.txt" };
};

namespace ui {

class TPMSAppView : public View {
public:
	TPMSAppView();
	~TPMSAppView();

	void set_parent_rect(const Rect new_parent_rect) override;

private:
	TPMSLogger logger;

	Console console;

	void on_packet(const baseband::Packet& packet);
	void draw(const tpms::Packet& packet);
};

} /* namespace ui */

#endif/*__TPMS_APP_H__*/
