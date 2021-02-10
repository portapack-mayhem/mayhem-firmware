/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#include "ui_receiver.hpp"
#include "ui_freqman.hpp"

#include "portapack.hpp"
using namespace portapack;

#include "string_format.hpp"

#include "max2837.hpp"

namespace ui {

/* FrequencyField ********************************************************/

FrequencyField::FrequencyField(
	const Point parent_pos
) : Widget { { parent_pos, { 8 * 10, 16 } } },
	length_ { 11 },
	range(rf::tuning_range)
{
	set_focusable(true);
}

rf::Frequency FrequencyField::value() const {
	return value_;
}

void FrequencyField::set_value(rf::Frequency new_value) {
	new_value = clamp_value(new_value);

	if( new_value != value_ ) {
		value_ = new_value;
		if( on_change ) {
			on_change(value_);
		}
		set_dirty();
	}
}

void FrequencyField::set_step(rf::Frequency new_value) {
	step = new_value;
	// TODO: Quantize current frequency to a step of the new size?
}

void FrequencyField::paint(Painter& painter) {
	const std::string str_value = to_string_short_freq(value_);

	const auto paint_style = has_focus() ? style().invert() : style();

	painter.draw_string(
		screen_pos(),
		paint_style,
		str_value
	);
}

bool FrequencyField::on_key(const ui::KeyEvent event) {
	if( event == ui::KeyEvent::Select ) {
		if( on_edit ) {
			on_edit();
			return true;
		}
	}
	return false;
}

bool FrequencyField::on_encoder(const EncoderEvent delta) {
	set_value(value() + (delta * step));
	return true;
}

bool FrequencyField::on_touch(const TouchEvent event) {
	if( event.type == TouchEvent::Type::Start ) {
		focus();
	}
	return true;
}

void FrequencyField::on_focus() {
	if( on_show_options ) {
		on_show_options();
	}
}

rf::Frequency FrequencyField::clamp_value(rf::Frequency value) {
	return range.clip(value);
}

/* FrequencyKeypadView ***************************************************/

bool FrequencyKeypadView::on_encoder(const EncoderEvent delta) {
	focused_button += delta;
	if (focused_button < 0) {
		focused_button = buttons.size() - 1;
	}
	else if (focused_button >= (int16_t)buttons.size()) {
		focused_button = 0;
	}
	buttons[focused_button].focus();
	return true;
}

FrequencyKeypadView::FrequencyKeypadView(
	NavigationView& nav,
	const rf::Frequency value
) {
	add_child(&text_value);

	const auto button_fn = [this](Button& button) {
		this->on_button(button);
	};

	const char* const key_caps = "123456789<0.";

	int n = 0;
	for(auto& button : buttons) {
		add_child(&button);
		const std::string label {
			key_caps[n]
		};
		button.id = n;
		button.on_highlight = [this](Button& button) {
			focused_button = button.id;
		};
		button.on_select = button_fn;
		button.set_parent_rect({
			(n % 3) * button_w,
			(n / 3) * button_h + 24,
			button_w, button_h
		});
		button.set_text(label);
		n++;
	}
	
	add_children({
		&button_save,
		&button_load,
		&button_close
	});
	
	button_save.on_select = [this, &nav](Button&) {
		nav.push<FrequencySaveView>(this->value());
	};
	button_load.on_select = [this, &nav](Button&) {
		auto load_view = nav.push<FrequencyLoadView>();
		load_view->on_frequency_loaded = [this](rf::Frequency value) {
			set_value(value);
		};
	};

	button_close.on_select = [this, &nav](Button&) {
		if( on_changed ) {
			on_changed(this->value());
		}
		nav.pop();
	};

	set_value(value);
}

void FrequencyKeypadView::focus() {
	button_close.focus();
}

rf::Frequency FrequencyKeypadView::value() const {
	return mhz.as_int() * 1000000ULL + submhz.as_int() * submhz_base;
}

void FrequencyKeypadView::set_value(const rf::Frequency new_value) {
	mhz.set(new_value / 1000000);
	mhz.remove_leading_zeros();

	submhz.set((new_value % 1000000) / submhz_base);
	submhz.remove_trailing_zeros();

	update_text();
}

void FrequencyKeypadView::on_button(Button& button) {
	const auto s = button.text();
	if( s == "." ) {
		field_toggle();
	} else if( s == "<" ) {
		digit_delete();
	} else {
		digit_add(s[0]);
	}
	update_text();
}

void FrequencyKeypadView::digit_add(const char c) {
	if( state == State::DigitMHz ) {
		if( clear_field_if_digits_entered ) {
			mhz.clear();
		}
		mhz.add_digit(c);
	} else {
		submhz.add_digit(c);
	}
	clear_field_if_digits_entered = false;
}

void FrequencyKeypadView::digit_delete() {
	if( state == State::DigitMHz ) {
		mhz.delete_digit();
	} else {
		submhz.delete_digit();
	}
}

void FrequencyKeypadView::field_toggle() {
	if( state == State::DigitMHz ) {
		state = State::DigitSubMHz;
		submhz.clear();
	} else {
		state = State::DigitMHz;
		clear_field_if_digits_entered = true;
	}
}

void FrequencyKeypadView::update_text() {
	const auto s = mhz.as_string() + "." + submhz.as_string();
	text_value.set(s);
}

/* FrequencyOptionsView **************************************************/

FrequencyOptionsView::FrequencyOptionsView(
	const Rect parent_rect,
	const Style* const style
) : View { parent_rect }
{
	set_style(style);

	field_step.on_change = [this](size_t n, OptionsField::value_t v) {
		(void)n;
		this->on_step_changed(v);
	};

	field_ppm.on_change = [this](int32_t v) {
		this->on_reference_ppm_correction_changed(v);
	};

	add_children({
		&text_step,
		&field_step,
	});

	if( portapack::clock_manager.get_reference().source == ClockManager::ReferenceSource::Xtal ) {
		add_child(&field_ppm);
		add_child(&text_ppm);
	}
}

void FrequencyOptionsView::set_step(rf::Frequency f) {
	field_step.set_by_value(f);
}

void FrequencyOptionsView::set_reference_ppm_correction(int32_t v) {
	field_ppm.set_value(v);
}

void FrequencyOptionsView::on_step_changed(rf::Frequency v) {
	if( on_change_step ) {
		on_change_step(v);
	}
}

void FrequencyOptionsView::on_reference_ppm_correction_changed(int32_t v) {
	if( on_change_reference_ppm_correction ) {
		on_change_reference_ppm_correction(v);
	}
}

/* RFAmpField ************************************************************/

RFAmpField::RFAmpField(
	Point parent_pos
) : NumberField {
		parent_pos,
		1,
		{ 0, 1 },
		1,
		' ',
	}
{
	set_value(receiver_model.rf_amp());

	on_change = [](int32_t v) {
		receiver_model.set_rf_amp(v);
	};
}

/* RadioGainOptionsView **************************************************/

RadioGainOptionsView::RadioGainOptionsView(
	const Rect parent_rect,
	const Style* const style
) : View { parent_rect }
{
	set_style(style);

	add_children({
		&label_rf_amp,
		&field_rf_amp,
	});
}

/* LNAGainField **********************************************************/

LNAGainField::LNAGainField(
	Point parent_pos
) : NumberField {
		parent_pos, 2,
		{ max2837::lna::gain_db_range.minimum, max2837::lna::gain_db_range.maximum },
		max2837::lna::gain_db_step,
		' ',
	}
{
	set_value(receiver_model.lna());

	on_change = [](int32_t v) {
		receiver_model.set_lna(v);
	};
}

void LNAGainField::on_focus() {
	//Widget::on_focus();
	if( on_show_options ) {
		on_show_options();
	}
}

/* VGAGainField **********************************************************/

VGAGainField::VGAGainField(
	Point parent_pos
) : NumberField {
		parent_pos, 2,
		{ max2837::vga::gain_db_range.minimum, max2837::vga::gain_db_range.maximum },
		max2837::vga::gain_db_step,
		' ',
	}
{
	set_value(receiver_model.vga());

	on_change = [](int32_t v) {
		receiver_model.set_vga(v);
	};
}

void VGAGainField::on_focus() {
	//Widget::on_focus();
	if( on_show_options ) {
		on_show_options();
	}
}

} /* namespace ui */
