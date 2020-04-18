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

#ifndef __TRANSMITTER_MODEL_H__
#define __TRANSMITTER_MODEL_H__

#include <cstdint>
#include <cstddef>

#include "receiver_model.hpp"
#include "message.hpp"
#include "rf_path.hpp"
#include "max2837.hpp"
#include "volume.hpp"
#include "signal.hpp"

class TransmitterModel {
public:
	rf::Frequency tuning_frequency() const;
	void set_tuning_frequency(rf::Frequency f);

	void set_antenna_bias();
	
	bool rf_amp() const;
	void set_rf_amp(bool enabled);

	int32_t lna() const;
	void set_lna(int32_t v_db);

	uint32_t baseband_bandwidth() const;
	void set_baseband_bandwidth(uint32_t v);

	int32_t vga() const;
	void set_vga(int32_t v_db);
	
	int32_t tx_gain() const;
	void set_tx_gain(int32_t v_db);
	
	uint32_t channel_bandwidth() const;
	void set_channel_bandwidth(uint32_t v);

	uint32_t sampling_rate() const;
	void set_sampling_rate(uint32_t v);

	void enable();
	void disable();

private:
	bool enabled_ { false };
	bool rf_amp_ { false };
	int32_t lna_gain_db_ { 0 };
	uint32_t channel_bandwidth_ { 1 };
	uint32_t baseband_bandwidth_ { max2837::filter::bandwidth_minimum };
	int32_t vga_gain_db_ { 8 };
	int32_t tx_gain_db_ { 47 };
	uint32_t sampling_rate_ { 3072000 };
	SignalToken signal_token_tick_second { };

	void update_tuning_frequency();
	void update_antenna_bias();
	void update_rf_amp();
	void update_lna();
	void update_baseband_bandwidth();
	void update_vga();
	void update_tx_gain();
	void update_sampling_rate();
	void on_tick_second();
};

#endif/*__TRANSMITTER_MODEL_H__*/
