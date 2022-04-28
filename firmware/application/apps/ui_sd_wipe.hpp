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

#ifndef __UI_SD_WIPE_H__
#define __UI_SD_WIPE_H__

#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "string_format.hpp"
#include "ff.h"

#include <cstdint>

namespace ui {

class WipeSDView : public View {
public:
	WipeSDView(NavigationView& nav);
	~WipeSDView();
	void focus() override;
	
	std::string title() const override { return "Wipe SD Card"; };	

private:
	NavigationView& nav_;
	
	bool confirmed = false;
	static Thread* thread;
	
	static msg_t static_fn(void* arg) {
		auto obj = static_cast<WipeSDView*>(arg);
		obj->run();
		return 0;
	}
	
	void run() {
		lfsr_word_t v = 1;
		//DIR d;
		const auto buffer = std::make_unique<std::array<uint8_t, 512>>();

		//f_opendir(&d, (TCHAR*)u"");

		uint32_t count = 512;	//sd_card::fs.n_fats * sd_card::fs.fsize;
		progress.set_max(count);

		for (uint32_t c = 0; c < count; c++) {
			progress.set_value(c);
			
			lfsr_fill(v,
				reinterpret_cast<lfsr_word_t*>(buffer->data()),
				sizeof(*buffer.get()) / sizeof(lfsr_word_t));
				
			if (disk_write(sd_card::fs.drv, buffer->data(), sd_card::fs.fatbase + c, 1) != RES_OK)
				break;
		}
		nav_.pop();
	}
	
	Text text_info {
		{ 10 * 8, 16 * 8, 10 * 8, 16 },
		"Working..."
	};
	
	ProgressBar progress {
		{ 2 * 8, 19 * 8, 26 * 8, 24 }
	};
	
	Button dummy {
		{ 240, 0, 0, 0 },
		""
	};
};

} /* namespace ui */

#endif/*__UI_SD_WIPE_H__*/
