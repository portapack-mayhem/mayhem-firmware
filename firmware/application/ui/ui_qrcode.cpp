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
#include "portapack_persistent_memory.hpp"

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

QRCodeImage::~QRCodeImage( )
{

}

QRCodeImage::QRCodeImage(const QRCodeImage&Image) : Widget { }
{
    (void)Image;
}

QRCodeImage & QRCodeImage::operator=(const QRCodeImage&Image)
{
    (void)Image;
    return *this;
}

void QRCodeImage::paint(Painter& painter) {

    (void)painter ;

	// The structure to manage the QR code
	QRCode qrcode;


	//Either small or large QR code can be shown..

	if(portapack::persistent_memory::show_bigger_qr_code()) { // show large QR code
		int qr_version =  2; 

		// Allocate a chunk of memory to store the QR code
		uint8_t qrcodeBytes[qrcode_getBufferSize(qr_version)];

		qrcode_initText(&qrcode, qrcodeBytes, qr_version, ECC_HIGH, qr_text_);


		display.fill_rectangle(Rect(10, 30, 220, 220), Color::white());

		for (uint8_t y = 0; y < qrcode.size; y++) {
    			for (uint8_t x = 0; x < qrcode.size; x++) {
        			if (qrcode_getModule(&qrcode, x, y)) {
            				display.fill_rectangle(Rect(20+(x*8), 40+(y*8), 8, 8), Color::black());

        			} 
    			}
		}

	}

	else { // show small QR code
		int qr_version =  10; 

		// Allocate a chunk of memory to store the QR code
		uint8_t qrcodeBytes[qrcode_getBufferSize(qr_version)];

		qrcode_initText(&qrcode, qrcodeBytes, qr_version, ECC_HIGH, qr_text_);


		display.fill_rectangle(Rect(92, 97, 63, 63), Color::white());

		for (uint8_t y = 0; y < qrcode.size; y++) {
    			for (uint8_t x = 0; x < qrcode.size; x++) {
        			if (qrcode_getModule(&qrcode, x, y)) {
            				display.draw_pixel(Point(95+x,100+y), Color::black());

        			} 
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
		&qr_code,
		&button_close});
        //text_qr.set(qr_text);	
	qr_code.set_text(qr_text);

	button_close.on_select =  [&nav](Button&){
		nav.pop();
	};
}



} /* namespace ui */
