/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#include "ui_qrcode.hpp"
#include "qrcodegen.hpp"
#include "portapack.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;

#include "string_format.hpp"
#include "complex.hpp"


namespace ui {

QRCodeImage::QRCodeImage(
	Rect parent_rect
) : Widget { parent_rect }
{
	
}

void QRCodeImage::paint(Painter& painter) {
	// The structure to manage the QR code
	QRCode qrcode;
	int qr_version = 2; // bigger versions aren't handled very well

	// Allocate a chunk of memory to store the QR code
	uint8_t qrcodeBytes[qrcode_getBufferSize(qr_version)];

	qrcode_initText(&qrcode, qrcodeBytes, qr_version, ECC_HIGH, qr_text_);


	display.fill_rectangle(Rect(20, 30, 220, 220), Color::white());

	for (uint8_t y = 0; y < qrcode.size; y++) {
    		for (uint8_t x = 0; x < qrcode.size; x++) {
        		if (qrcode_getModule(&qrcode, x, y)) {
                        display.fill_rectangle(Rect(30+(x*8), 40+(y*8), 8, 8), Color::black());
        		}
    		}
	}
}

void QRCodeView::focus() {
	button_close.focus();
}

QRCodeView::~QRCodeView() {
	if (on_close_)
		on_close_();
}

QRCodeView::QRCodeView(
	NavigationView& nav,
	const char * qr_text,
	const std::function<void(void)> on_close
) : nav_ (nav),
	on_close_(on_close)
{


        add_children({
		&text_qr,
		&qr_code,
		&button_close});
        text_qr.set(qr_text);	
	qr_code.set_text(qr_text);

	button_close.on_select =  [&nav](Button&){
		nav.pop();
	};
}



} /* namespace ui */
