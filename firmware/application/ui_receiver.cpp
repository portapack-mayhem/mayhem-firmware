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

#include "ui_receiver.hpp"

#include "ui_spectrum.hpp"
#include "ui_console.hpp"

#include "portapack.hpp"
using namespace portapack;

#include "ais_baseband.hpp"

#include "sd_card.hpp"
#include "ff.h"

namespace ui {

/* BasebandBandwidthField ************************************************/

BasebandBandwidthField::BasebandBandwidthField(
	Point parent_pos
) : OptionsField {
		parent_pos,
		4,
		{
			{ " 1M8",  1750000 },
			{ " 2M5",  2500000 },
			{ " 3M5",  3500000 },
			{ " 5M ",  5000000 },
			{ " 5M5",  5500000 },
			{ " 6M ",  6000000 },
			{ " 7M ",  7000000 },
			{ " 8M ",  8000000 },
			{ " 9M ",  9000000 },
			{ "10M ", 10000000 },
			{ "12M ", 12000000 },
			{ "14M ", 14000000 },
			{ "15M ", 15000000 },
			{ "20M ", 20000000 },
			{ "24M ", 24000000 },
			{ "28M ", 28000000 },
		}
	}
{
}

/* FrequencyField ********************************************************/

FrequencyField::FrequencyField(
	const Point parent_pos
) : Widget { { parent_pos, { 8 * 10, 16 } } },
	length_ { 11 },
	range(rf::tuning_range)
{
	flags.focusable = true;
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
	const auto mhz = to_string_dec_int(value_ / 1000000, 4);
	const auto hz100 = to_string_dec_int((value_ / 100) % 10000, 4, '0');

	const auto paint_style = has_focus() ? style().invert() : style();

	painter.draw_string(
		screen_pos(),
		paint_style,
		mhz
	);
	painter.draw_string(
		screen_pos() + Point { 4 * 8, 0 },
		paint_style,
		"."
	);
	painter.draw_string(
		screen_pos() + Point { 5 * 8, 0 },
		paint_style,
		hz100
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
/*
bool FrequencyField::on_key(const ui::KeyEvent event) override {
	if( event == ui::KeyEvent::Select ) {

		// NOTE: For testing sampling rate / decimation combinations
		turbo = !turbo;
		if( turbo ) {
			clock_manager.set_sampling_frequency(18432000);
			radio::set_baseband_decimation_by(6);
		} else {
			clock_manager.set_sampling_frequency(12288000);
			radio::set_baseband_decimation_by(4);
		}
		return true;
	}

	return false;
}
*/
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
	if( value > range.max ) {
		value = range.max;
	}
	if( value < range.min ) {
		value = range.min;
	}
	return value;
}

/* FrequencyKeypadView ***************************************************/

FrequencyKeypadView::FrequencyKeypadView(
	NavigationView& nav,
	const rf::Frequency value
) {
	add_child(&text_value);

	const auto button_fn = [this](Button& button) {
		this->on_button(button);
	};

	const char* const key_caps = "123456789<0.";

	size_t n = 0;
	for(auto& button : buttons) {
		add_child(&button);
		const std::string label {
			key_caps[n]
		};
		button.on_select = button_fn;
		button.set_parent_rect({
			static_cast<Coord>((n % 3) * button_w),
			static_cast<Coord>((n / 3) * button_h + button_h),
			button_w, button_h
		});
		button.set_text(label);
		n++;
	}

	add_child(&button_close);
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

	options_step.on_change = [this](size_t n, OptionsField::value_t v) {
		(void)n;
		this->on_step_changed(v);
	};

	field_ppm.on_change = [this](int32_t v) {
		this->on_reference_ppm_correction_changed(v);
	};

	add_children({ {
		&text_step,
		&options_step,
		&text_correction,
		&field_ppm,
		&text_ppm,
	} });
}

void FrequencyOptionsView::set_step(rf::Frequency f) {
	options_step.set_by_value(f);
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

/* RadioGainOptionsView **************************************************/

RadioGainOptionsView::RadioGainOptionsView(
	const Rect parent_rect,
	const Style* const style
) : View { parent_rect }
{
	set_style(style);

	add_children({ {
		&label_rf_amp,
		&field_rf_amp,
		//&label_agc,
		//&field_agc
	} });

	field_rf_amp.on_change = [this](int32_t v) {
		this->on_rf_amp_changed(v);
	};
	/*
	field_agc.set_value(receiver_model.agc());
	field_agc.on_change = [this](int32_t v) {
		this->on_agc_changed(v);
	};
	*/
}

void RadioGainOptionsView::set_rf_amp(int32_t v_db) {
	field_rf_amp.set_value(v_db);
}

void RadioGainOptionsView::on_rf_amp_changed(bool enable) {
	if( on_change_rf_amp ) {
		on_change_rf_amp(enable);
	}
}
/*
void RadioGainOptionsView::on_agc_changed(bool v) {
	receiver_model.set_agc(v);
}
*/

/* LNAGainField **********************************************************/

LNAGainField::LNAGainField(
	Point parent_pos
) : NumberField {
		{ parent_pos }, 2,
		{ max2837::lna::gain_db_min, max2837::lna::gain_db_max },
		max2837::lna::gain_db_step,
		' ',
	}
{
}

void LNAGainField::on_focus() {
	//Widget::on_focus();
	if( on_show_options ) {
		on_show_options();
	}
}

/* ReceiverView **********************************************************/

ReceiverView::ReceiverView(
	NavigationView& nav,
	ReceiverModel& receiver_model
) : receiver_model(receiver_model)
{
	add_children({ {
		&rssi,
		&channel,
		&audio,
		&button_done,
		&field_frequency,
		&field_lna,
		//&options_baseband_bandwidth,
		&field_vga,
		&options_modulation,
		//&options_baseband_oversampling,
		&field_volume,
		&view_frequency_options,
		&view_rf_gain_options,
	} });

	button_done.on_select = [&nav](Button&){
		nav.pop();
	};

	field_frequency.set_value(receiver_model.tuning_frequency());
	field_frequency.set_step(receiver_model.frequency_step());
	field_frequency.on_change = [this](rf::Frequency f) {
		this->on_tuning_frequency_changed(f);
	};
	field_frequency.on_edit = [this, &nav]() {
		// TODO: Provide separate modal method/scheme?
		auto new_view = new FrequencyKeypadView { nav, this->receiver_model.tuning_frequency() };
		new_view->on_changed = [this](rf::Frequency f) {
			this->on_tuning_frequency_changed(f);
			this->field_frequency.set_value(f);
		};
		nav.push(new_view);
	};
	field_frequency.on_show_options = [this]() {
		this->on_show_options_frequency();
	};

	field_lna.set_value(receiver_model.lna());
	field_lna.on_change = [this](int32_t v) {
		this->on_lna_changed(v);
	};
	field_lna.on_show_options = [this]() {
		this->on_show_options_rf_gain();
	};
	/*
	options_baseband_bandwidth.set_by_value(receiver_model.baseband_bandwidth());
	options_baseband_bandwidth.on_change = [this](size_t n, OptionsField::value_t v) {
		(void)n;
		this->on_baseband_bandwidth_changed(v);
	};
	*/
	field_vga.set_value(receiver_model.vga());
	field_vga.on_change = [this](int32_t v_db) {
		this->on_vga_changed(v_db);
	};

	options_modulation.set_by_value(receiver_model.modulation());
	options_modulation.on_change = [this](size_t n, OptionsField::value_t v) {
		(void)n;
		this->on_modulation_changed(v);
	};
/*
	options_baseband_oversampling.set_by_value(receiver_model.baseband_oversampling());
	options_baseband_oversampling.on_change = [this](size_t n, OptionsField::value_t v) {
		(void)n;
		this->on_baseband_oversampling_changed(v);
	};
*/
	field_volume.set_value((receiver_model.headphone_volume() - wolfson::wm8731::headphone_gain_range.max).decibel() + 99);
	field_volume.on_change = [this](int32_t v) {
		this->on_headphone_volume_changed(v);
	};

	view_frequency_options.hidden(true);
	view_frequency_options.set_step(receiver_model.frequency_step());
	view_frequency_options.on_change_step = [this](rf::Frequency f) {
		this->on_frequency_step_changed(f);
	};
	view_frequency_options.set_reference_ppm_correction(receiver_model.reference_ppm_correction());
	view_frequency_options.on_change_reference_ppm_correction = [this](int32_t v) {
		this->on_reference_ppm_correction_changed(v);
	};

	view_rf_gain_options.hidden(true);
	view_rf_gain_options.set_rf_amp(receiver_model.rf_amp());
	view_rf_gain_options.on_change_rf_amp = [this](bool enable) {
		this->on_rf_amp_changed(enable);
	};

	receiver_model.enable();
}

ReceiverView::~ReceiverView() {

	// TODO: Manipulating audio codec here, and in ui_receiver.cpp. Good to do
	// both?
	audio_codec.headphone_mute();

	receiver_model.disable();
}

void ReceiverView::on_show() {
	auto& message_map = context().message_map();
	message_map.register_handler(Message::ID::AISPacket,
		[this](Message* const p) {
			const auto message = static_cast<const AISPacketMessage*>(p);
			this->on_packet_ais(*message);
		}
	);
	message_map.register_handler(Message::ID::TPMSPacket,
		[this](Message* const p) {
			const auto message = static_cast<const TPMSPacketMessage*>(p);
			this->on_packet_tpms(*message);
		}
	);
	message_map.register_handler(Message::ID::ERTPacket,
		[this](Message* const p) {
			const auto message = static_cast<const ERTPacketMessage*>(p);
			this->on_packet_ert(*message);
		}
	);

	sd_card_status_signal_token = sd_card::status_signal += [this](const sd_card::Status status) {
		this->on_sd_card_status(status);
	};
}

void ReceiverView::on_hide() {
	sd_card::status_signal -= sd_card_status_signal_token;

	auto& message_map = context().message_map();
	message_map.unregister_handler(Message::ID::ERTPacket);
	message_map.unregister_handler(Message::ID::TPMSPacket);
	message_map.unregister_handler(Message::ID::AISPacket);
}

void ReceiverView::on_packet_ais(const AISPacketMessage& message) {
	const auto result = baseband::ais::packet_decode(message.packet.payload, message.packet.bits_received);

	auto console = reinterpret_cast<Console*>(widget_content.get());
	if( result.first == "OK" ) {
		console->writeln(result.second);
	}
}

static FIL fil_tpms;

static std::pair<std::string, std::string> format_manchester(
	const uint_fast8_t sense,
	const std::bitset<1024>& payload,
	const size_t payload_length
) {
	const size_t payload_length_decoded = payload_length / 2;
	const size_t payload_length_bytes = (payload_length_decoded + 7) / 8;
	const size_t payload_length_symbols_rounded = payload_length_bytes * 8 * 2;

	std::string hex_data;
	std::string hex_error;
	uint8_t byte_data = 0;
	uint8_t byte_error = 0;
	for(size_t i=0; i<payload_length_symbols_rounded; i+=2) {
		const auto bit_data = payload[i+sense];
		const auto bit_error = (payload[i+0] == payload[i+1]);

		byte_data <<= 1;
		byte_data |= bit_data ? 1 : 0;

		byte_error <<= 1;
		byte_error |= bit_error ? 1 : 0;

		if( ((i >> 1) & 7) == 7 ) {
			hex_data += to_string_hex(byte_data, 2);
			hex_error += to_string_hex(byte_error, 2);
		}
	}

	return { hex_data, hex_error };
}

void ReceiverView::on_packet_tpms(const TPMSPacketMessage& message) {
	auto payload = message.packet.payload;
	auto payload_length = message.packet.bits_received;

	const auto hex_formatted = format_manchester(1, payload, payload_length);

	auto console = reinterpret_cast<Console*>(widget_content.get());
	console->writeln(hex_formatted.first.substr(0, 240 / 8));

	if( !f_error(&fil_tpms) ) {
		rtc::RTC datetime;
		rtcGetTime(&RTCD1, &datetime);
		std::string timestamp = 
			to_string_dec_uint(datetime.year(), 4) +
			to_string_dec_uint(datetime.month(), 2, '0') +
			to_string_dec_uint(datetime.day(), 2, '0') +
			to_string_dec_uint(datetime.hour(), 2, '0') +
			to_string_dec_uint(datetime.minute(), 2, '0') +
			to_string_dec_uint(datetime.second(), 2, '0');

		const auto tuning_frequency = receiver_model.tuning_frequency();
		// TODO: function doesn't take uint64_t, so when >= 1<<32, weirdness will ensue!
		const auto tuning_frequency_str = to_string_dec_uint(tuning_frequency, 10);

		std::string log = timestamp + " " + tuning_frequency_str + " FSK 38.4 19.2 " + hex_formatted.first + "/" + hex_formatted.second + "\r\n";
		f_puts(log.c_str(), &fil_tpms);
		f_sync(&fil_tpms);
	}
}

static std::bitset<512> manchester_decode(
	const size_t sense,
	const std::bitset<1024>& encoded,
	const size_t count
) {
	std::bitset<512> result;
	for(size_t i=0; i<count; i+=2) {
		result[i >> 1] = encoded[i+sense];
	}
	return result;
}

static std::bitset<512> manchester_errors(
	const std::bitset<1024>& encoded,
	const size_t count
) {
	std::bitset<512> result;
	for(size_t i=0; i<count; i+=2) {
		result[i >> 1] = (encoded[i+0] == encoded[i+1]);
	}
	return result;
}

void ReceiverView::on_packet_ert(const ERTPacketMessage& message) {
	auto console = reinterpret_cast<Console*>(widget_content.get());

	if( message.packet.preamble == 0x555516a3 ) {
		console->writeln("IDM");
	}
	if( message.packet.preamble == 0x1f2a60 ) {
		console->writeln("SCM");
	}

	const auto hex_formatted = format_manchester(0, message.packet.payload, message.packet.bits_received);
	console->writeln(hex_formatted.first);
	console->writeln(hex_formatted.second);
}

void ReceiverView::on_sd_card_status(const sd_card::Status status) {
	if( status == sd_card::Status::Mounted ) {
		const auto open_result = f_open(&fil_tpms, "tpms.txt", FA_WRITE | FA_OPEN_ALWAYS);
		if( open_result == FR_OK ) {
			const auto fil_size = f_size(&fil_tpms);
			const auto seek_result = f_lseek(&fil_tpms, fil_size);
			if( seek_result != FR_OK ) {
				f_close(&fil_tpms);
				// TODO: Error, indicate somehow.
			} else {
				// TODO: Indicate success.
			}
		} else {
			// TODO: Error, indicate somehow.
		}
	} else {
		// TODO: Indicate unmounted.
	}
}

void ReceiverView::focus() {
	button_done.focus();
}

void ReceiverView::on_tuning_frequency_changed(rf::Frequency f) {
	receiver_model.set_tuning_frequency(f);
}

void ReceiverView::on_baseband_bandwidth_changed(uint32_t bandwidth_hz) {
	receiver_model.set_baseband_bandwidth(bandwidth_hz);
}

void ReceiverView::on_rf_amp_changed(bool v) {
	receiver_model.set_rf_amp(v);
}

void ReceiverView::on_lna_changed(int32_t v_db) {
	receiver_model.set_lna(v_db);
}

void ReceiverView::on_vga_changed(int32_t v_db) {
	receiver_model.set_vga(v_db);
}

void ReceiverView::on_modulation_changed(int32_t modulation) {
	/* TODO: This is TERRIBLE!!! */
	switch(modulation) {
	case 3:
	case 5:
		receiver_model.set_baseband_configuration({
			.mode = modulation,
			.sampling_rate = 2457600,
			.decimation_factor = 4,
		});
		receiver_model.set_baseband_bandwidth(1750000);
		break;

	case 6:
		receiver_model.set_baseband_configuration({
			.mode = modulation,
			.sampling_rate = 4194304,
			.decimation_factor = 1,
		});
		receiver_model.set_baseband_bandwidth(1750000);
		break;

	case 4:
		receiver_model.set_baseband_configuration({
			.mode = modulation,
			.sampling_rate = 20000000,
			.decimation_factor = 1,
		});
		receiver_model.set_baseband_bandwidth(12000000);
		break;

	default:
		receiver_model.set_baseband_configuration({
			.mode = modulation,
			.sampling_rate = 3072000,
			.decimation_factor = 4,
		});
		receiver_model.set_baseband_bandwidth(1750000);
		break;
	}

	remove_child(widget_content.get());
	widget_content.reset();
	
	switch(modulation) {
	case 3:
	case 5:
	case 6:
		widget_content = std::make_unique<Console>();
		add_child(widget_content.get());
		break;

	default:
		widget_content = std::make_unique<spectrum::WaterfallWidget>();
		add_child(widget_content.get());
		break;
	}
	
	if( widget_content ) {
		const ui::Dim header_height = 3 * 16;
		const ui::Rect rect { 0, header_height, parent_rect.width(), static_cast<ui::Dim>(parent_rect.height() - header_height) };
		widget_content->set_parent_rect(rect);
	}
}

void ReceiverView::on_show_options_frequency() {
	view_rf_gain_options.hidden(true);
	field_lna.set_style(nullptr);

	view_frequency_options.hidden(false);
	field_frequency.set_style(&view_frequency_options.style());
}

void ReceiverView::on_show_options_rf_gain() {
	view_frequency_options.hidden(true);
	field_frequency.set_style(nullptr);

	view_rf_gain_options.hidden(false);
	field_lna.set_style(&view_frequency_options.style());
}

void ReceiverView::on_frequency_step_changed(rf::Frequency f) {
	receiver_model.set_frequency_step(f);
	field_frequency.set_step(f);
}

void ReceiverView::on_reference_ppm_correction_changed(int32_t v) {
	receiver_model.set_reference_ppm_correction(v);
}

void ReceiverView::on_headphone_volume_changed(int32_t v) {
	const auto new_volume = volume_t::decibel(v - 99) + wolfson::wm8731::headphone_gain_range.max;
	receiver_model.set_headphone_volume(new_volume);
}

// void ReceiverView::on_baseband_oversampling_changed(int32_t v) {
// 	receiver_model.set_baseband_oversampling(v);
// }

} /* namespace ui */
