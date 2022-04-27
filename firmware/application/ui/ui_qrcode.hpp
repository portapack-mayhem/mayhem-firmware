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

#ifndef __QRCODE_H__
#define __QRCODE_H__

#include "ui.hpp"

#include "ui_navigation.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "qrcodegen.hpp"
#include "portapack.hpp"

namespace ui {

class QRCodeImage : public Widget {
public:

	QRCodeImage(Rect parent_rect);
	void set_text(const char * qr_text) {
		qr_text_ = qr_text;
	}
	void paint(Painter& painter) override;
                                                   // for -weffc++ to be killed
    ~QRCodeImage();                                // destructor 
    QRCodeImage(const QRCodeImage&Image);
    QRCodeImage & operator=(const QRCodeImage &Image); // assignment

private:
	const char * qr_text_ = NULL ;
};

class QRCodeView : public View {
public:
	QRCodeView(
		NavigationView& nav,
		const char * qr_text,		
		const std::function<void(void)> on_close = nullptr
	);
	~QRCodeView();
	
	QRCodeView(const QRCodeView&) = delete;
	QRCodeView(QRCodeView&&) = delete;
	QRCodeView& operator=(const QRCodeView&) = delete;
	QRCodeView& operator=(QRCodeView&&) = delete;
	
	std::string title() const override { return "QR code"; };
	void focus() override;

private:
	NavigationView& nav_;


	std::function<void(void)> on_close_ { nullptr };


	QRCodeImage qr_code {
		{ 50, 100, 100, 100  }
	};

	//Text text_qr {
	//	{ 0 * 8, 10 * 16, 32 * 8, 1 * 8 },
	//	"-"
	//};

	Button button_close {
		{ 9 * 8, 31 * 8, 12 * 8, 3 * 16 },
		"Back"
	};
};

} /* namespace ui */

#endif
