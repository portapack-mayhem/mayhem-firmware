/*
 * Copyright (C) 2023 Bernd Herzog
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

#pragma once

#include "ui.hpp"
#include "ui_widget.hpp"

#include "ui_navigation.hpp"
#include "ui_tabview.hpp"
#include "baseband_api.hpp"

namespace ui {

class SpectrumInputImageView : public View {
public:
	SpectrumInputImageView(NavigationView& nav);
	~SpectrumInputImageView();

	void focus() override;
	void paint(Painter&) override;

private:
	bool painted {false};
	
	Button button_start {
		{ 0 * 8, 17 * 8, 18 * 8, 28 },
		"Load Image ..."
	};
};

class SpectrumInputTextView : public View {
public:
	SpectrumInputTextView(NavigationView& nav);
	~SpectrumInputTextView();

	void focus() override;
	
private:
	Button button_start {
		{ 0 * 8, 11 * 8, 11 * 8, 28 },
		"s2"
	};
};

class SpectrumPainterView : public View {
public:
	SpectrumPainterView(NavigationView& nav);
	~SpectrumPainterView();

	SpectrumPainterView(const SpectrumPainterView&) = delete;
	SpectrumPainterView(SpectrumPainterView&&) = delete;
	SpectrumPainterView& operator=(const SpectrumPainterView&) = delete;
	SpectrumPainterView& operator=(SpectrumPainterView&&) = delete;

	void focus() override;
	
	std::string title() const override { return "Spec.Painter"; };

private:
	NavigationView& nav_;

	SpectrumInputImageView input_image { nav_ };
	SpectrumInputTextView input_text { nav_ };
	
	std::array<View*, 2> input_views { { &input_image, &input_text } };

	TabView tab_view {
		{ "Image", Color::white(), input_views[0] },
		{ "Text", Color::white(), input_views[1] }
	};

	Button button_exit {
		{ 20 * 8, 34 * 8, 10 * 8, 4 * 8 },
		"Paint"
	};
};

}
