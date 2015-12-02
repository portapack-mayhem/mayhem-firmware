/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __UI_RECEIVER_H__
#define __UI_RECEIVER_H__

#include "ui.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "ui_navigation.hpp"
#include "ui_painter.hpp"
#include "ui_widget.hpp"

#include "utility.hpp"

#include "max2837.hpp"
#include "rf_path.hpp"
#include "volume.hpp"
#include "wm8731.hpp"

#include "receiver_model.hpp"

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <functional>

namespace ui {

class BasebandBandwidthField : public OptionsField {
public:
	BasebandBandwidthField(Point parent_pos);
};

class FrequencyField : public Widget {
public:
	std::function<void(rf::Frequency)> on_change;
	std::function<void(void)> on_edit;
	std::function<void(void)> on_show_options;

	using range_t = rf::FrequencyRange;

	FrequencyField(const Point parent_pos);

	rf::Frequency value() const;

	void set_value(rf::Frequency new_value);
	void set_step(rf::Frequency new_value);

	void paint(Painter& painter) override;

	bool on_key(const ui::KeyEvent event) override;
	bool on_encoder(const EncoderEvent delta) override;
	bool on_touch(const TouchEvent event) override;
	void on_focus() override;

private:
	const size_t length_;
	const range_t range;
	rf::Frequency value_;
	rf::Frequency step { 25000 };

	//bool turbo { false };

	rf::Frequency clamp_value(rf::Frequency value);
};

template<size_t N>
class FieldString {
public:
	enum Justify {
		Right,
		Left,
	};

	constexpr FieldString(
		Justify justify
	) : justify { justify }
	{
	}

	uint32_t as_int() const {
		uint32_t value = 0;
		for(const auto c : s) {
			const uint_fast8_t digit = (c == ' ') ? 0 : c - '0';
			value = (value * 10) + digit;
		}
		return value;
	}

	void set(uint32_t value) {
		std::generate(s.rbegin(), s.rend(), [&value]() {
			const char digit = (value % 10) + '0';
			value /= 10;
			return digit;
		});
	}

	void clear() {
		s.fill(' ');
	}

	void add_digit(const char c) {
		/*
		if( justify == Justify::Right ) {
			push_right(c);
		} else {
			insert_right(c);
		}
		*/
		insert_right(c);
	}

	void delete_digit() {
		if( justify == Justify::Right ) {
			shift_right();
			s.front() = ' ';
		} else {
			auto first_digit = std::find_if(s.rbegin(), s.rend(), [](const char& a) {
				return a != ' ';
			});
			if( first_digit != s.rend() ) {
				*first_digit = ' ';
			}
		}
	}

	std::string as_string() const {
		return { s.data(), s.size() };
	}

	void remove_leading_zeros() {
		remove_zeros(s.begin(), s.end());
	}

	void remove_trailing_zeros() {
		remove_zeros(s.rbegin(), s.rend());
	}

private:
	using array_type = std::array<char, N>;

	array_type s;
	Justify justify;

	template<typename Iterator>
	void remove_zeros(Iterator begin, Iterator end) {
		const auto first_significant_digit =
			std::find_if(begin, end, [](const char& a) {
				return a != '0';
			});
		std::fill(begin, first_significant_digit, ' ');
	}

	void insert_right(const char c) {
		auto insert_point = s.end() - 1;

		if( justify == Justify::Left ) {
			insert_point = std::find_if(s.begin(), s.end(), [](const char& a) {
				return a == ' ';
			});
		}

		if( *insert_point != ' ' ) {
			insert_point = shift_left();
		}

		*insert_point = c;
	}

	typename array_type::iterator shift_left() {
		return std::move(s.begin() + 1, s.end(), s.begin());
	}

	typename array_type::iterator shift_right() {
		return std::move_backward(s.begin(), s.end() - 1, s.end());
	}
};

class FrequencyKeypadView : public View {
public:
	std::function<void(rf::Frequency)> on_changed;

	FrequencyKeypadView(
		NavigationView& nav,
		const rf::Frequency value
	);

	void focus() override;

	rf::Frequency value() const;
	void set_value(const rf::Frequency new_value);

private:
	static constexpr size_t button_w = 240 / 3;
	static constexpr size_t button_h = 48;

	static constexpr size_t mhz_digits = 4;
	static constexpr size_t submhz_digits = 4;

	static constexpr size_t mhz_mod = pow(10, mhz_digits);
	static constexpr size_t submhz_base = pow(10, 6 - submhz_digits);
	//static constexpr size_t submhz_mod = pow(10, submhz_digits);
	static constexpr size_t text_digits = mhz_digits + 1 + submhz_digits;

	Text text_value {
		{ 0, 0, text_digits * button_w, button_h }
	};

	std::array<Button, 12> buttons;

	Button button_close {
		{ 0, button_h * 4 + button_h, button_w * 3, button_h },
		"Done"
	};

	/* TODO: Template arg required in enum?! */
	FieldString<mhz_digits> mhz { FieldString<4>::Justify::Right };
	FieldString<submhz_digits> submhz { FieldString<4>::Justify::Left };

	enum State {
		DigitMHz,
		DigitSubMHz
	};

	State state { DigitMHz };
	bool clear_field_if_digits_entered { true };

	void on_button(Button& button);

	void digit_add(const char c);
	void digit_delete();

	void field_toggle();
	void update_text();
};

class FrequencyOptionsView : public View {
public:
	std::function<void(rf::Frequency)> on_change_step;
	std::function<void(int32_t)> on_change_reference_ppm_correction;

	FrequencyOptionsView(const Rect parent_rect, const Style* const style);

	void set_step(rf::Frequency f);
	void set_reference_ppm_correction(int32_t v);

private:
	Text text_step {
		{ 0 * 8, 0 * 16, 4 * 8, 1 * 16 },
		"Step"
	};

	OptionsField options_step {
		{ 5 * 8, 0 * 16 },
		5,
		{
			{ "  100",      100 },
			{ "  1k ",     1000 },
			{ " 10k ",    10000 },
			{ " 12k5",    12500 },
			{ " 25k ",    25000 },
			{ "100k ",   100000 },
			{ "  1M ",  1000000 },
			{ " 10M ", 10000000 },
		}
	};

	void on_step_changed(rf::Frequency v);
	void on_reference_ppm_correction_changed(int32_t v);

	Text text_correction {
		{ 17 * 8, 0 * 16, 5 * 8, 16 },
		"Corr.",
	};

	NumberField field_ppm {
		{ 23 * 8, 0 * 16 },
		3,
		{ -99, 99 },
		1,
		'0',
	};

	Text text_ppm {
		{ 27 * 8, 0 * 16, 3 * 8, 16 },
		"PPM",
	};
};

class RadioGainOptionsView : public View {
public:
	std::function<void(bool)> on_change_rf_amp;

	RadioGainOptionsView(const Rect parent_rect, const Style* const style);

	void set_rf_amp(int32_t v_db);

private:
	Text label_rf_amp {
		{ 0 * 8, 0 * 16, 3 * 8, 1 * 16 },
		"Amp"
	};

	NumberField field_rf_amp {
		{ 4 * 8, 0 * 16},
		1,
		{ 0, 1 },
		1,
		' ',
	};
	/*
	Text label_agc {
		{ 6 * 8, 0 * 16, 3 * 8, 1 * 16 },
		"AGC"
	};

	NumberField field_agc {
		{ 10 * 8, 0 * 16},
		1,
		{ 0, 1 }
	};
	*/

	void on_rf_amp_changed(bool enable);
};

class LNAGainField : public NumberField {
public:
	std::function<void(void)> on_show_options;

	LNAGainField(Point parent_pos);

	void on_focus() override;
};

constexpr Style style_options_group {
	.font = font::fixed_8x16,
	.background = Color::blue(),
	.foreground = Color::white(),
};

class ReceiverView : public View {
public:
	ReceiverView(NavigationView& nav, ReceiverModel& receiver_model);
	~ReceiverView();

	void focus() override;

private:
	ReceiverModel& receiver_model;

	RSSI rssi {
		{ 19 * 8, 0, 11 * 8, 4 },
	};

	Channel channel {
		{ 19 * 8, 5, 11 * 8, 4 },
	};

	Audio audio {
		{ 19 * 8, 10, 11 * 8, 4 },
	};

	Button button_done {
		{ 0 * 8, 0 * 16, 3 * 8, 16 },
		" < ",
	};

	FrequencyField field_frequency {
		{ 0 * 8, 1 * 16 },
	};

	LNAGainField field_lna {
		{ 13 * 8, 1 * 16 }
	};
	/*
	BasebandBandwidthField options_baseband_bandwidth {
		{ 15 * 8, 1 * 16 },
	};
	*/
	NumberField field_vga {
		{ 16 * 8, 1 * 16},
		2,
		{ max2837::vga::gain_db_min, max2837::vga::gain_db_max },
		max2837::vga::gain_db_step,
		' ',
	};

	OptionsField options_modulation {
		{ 19 * 8, 1 * 16 },
		4,
		{
			{ " AM ", toUType(ReceiverModel::Mode::AMAudio) },
			{ "NFM ", toUType(ReceiverModel::Mode::NarrowbandFMAudio) },
			{ "WFM ", toUType(ReceiverModel::Mode::WidebandFMAudio) },
			{ "AIS ", toUType(ReceiverModel::Mode::AIS) },
			{ "TPMS", toUType(ReceiverModel::Mode::TPMS) },
			{ "ERT",  toUType(ReceiverModel::Mode::ERT) },
			{ "SPEC", toUType(ReceiverModel::Mode::SpectrumAnalysis) },
		}
	};
/*
	OptionsField options_baseband_oversampling {
		{ 24 * 8, 1 * 16 },
		1,
		{
			{ "4", 4 },
			{ "6", 6 },
			{ "8", 8 },
		}
	};
*/
	NumberField field_vregmode {
		{ 24 * 8, 1 * 16 },
		1,
		{ 0, 1 },
		1,
		' ',
	};

	NumberField field_volume {
		{ 28 * 8, 1 * 16 },
		2,
		{ 0, 99 },
		1,
		' ',
	};

	FrequencyOptionsView view_frequency_options {
		{ 0 * 8, 2 * 16, 30 * 8, 1 * 16 },
		&style_options_group
	};

	RadioGainOptionsView view_rf_gain_options {
		{ 0 * 8, 2 * 16, 30 * 8, 1 * 16 },
		&style_options_group
	};

	std::unique_ptr<Widget> widget_content;

	void on_tuning_frequency_changed(rf::Frequency f);
	void on_baseband_bandwidth_changed(uint32_t bandwidth_hz);
	void on_rf_amp_changed(bool v);
	void on_lna_changed(int32_t v_db);
	void on_vga_changed(int32_t v_db);
	void on_modulation_changed(ReceiverModel::Mode mode);
	void on_show_options_frequency();
	void on_show_options_rf_gain();
	void on_frequency_step_changed(rf::Frequency f);
	void on_reference_ppm_correction_changed(int32_t v);
	void on_headphone_volume_changed(int32_t v);
//	void on_baseband_oversampling_changed(int32_t v);
	void on_edit_frequency();

	void on_packet_ais(const AISPacketMessage& message);
	void on_packet_tpms(const TPMSPacketMessage& message);
	void on_packet_ert(const ERTPacketMessage& message);
};

} /* namespace ui */

#endif/*__UI_RECEIVER_H__*/
