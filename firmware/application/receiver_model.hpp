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

#ifndef __RECEIVER_MODEL_H__
#define __RECEIVER_MODEL_H__

#include <cstdint>
#include <cstddef>

#include "clock_manager.hpp"
#include "message.hpp"
#include "rf_path.hpp"
#include "max2837.hpp"
#include "volume.hpp"

class ReceiverModel {
public:
	enum class Mode : int32_t {
		AMAudio = 0,
		NarrowbandFMAudio = 1,
		WidebandFMAudio = 2,
		AIS = 3,
		SpectrumAnalysis = 4,
		TPMS = 5,
		ERT = 6,
	};

	constexpr ReceiverModel(
		ClockManager& clock_manager
	) : clock_manager(clock_manager)
	{
	}

	rf::Frequency tuning_frequency() const;
	void set_tuning_frequency(rf::Frequency f);

	rf::Frequency frequency_step() const;
	void set_frequency_step(rf::Frequency f);

	int32_t reference_ppm_correction() const;
	void set_reference_ppm_correction(int32_t v);

	bool rf_amp() const;
	void set_rf_amp(bool enabled);

	int32_t lna() const;
	void set_lna(int32_t v_db);

	uint32_t baseband_bandwidth() const;
	void set_baseband_bandwidth(uint32_t v);

	int32_t vga() const;
	void set_vga(int32_t v_db);

	uint32_t sampling_rate() const;

	uint32_t modulation() const;

	volume_t headphone_volume() const;
	void set_headphone_volume(volume_t v);

	uint32_t baseband_oversampling() const;

	void enable();
	void disable();

	void set_baseband_configuration(const BasebandConfiguration config);

private:
	rf::Frequency frequency_step_ { 25000 };
	bool rf_amp_ { false };
	int32_t lna_gain_db_ { 32 };
	uint32_t baseband_bandwidth_ { max2837::filter::bandwidth_minimum };
	int32_t vga_gain_db_ { 32 };
	BasebandConfiguration baseband_configuration {
		.mode = 1,			/* TODO: Enum! */
		.sampling_rate = 3072000,
		.decimation_factor = 1,
	};
	volume_t headphone_volume_ { -43.0_dB };
	ClockManager& clock_manager;

	int32_t tuning_offset();

	void update_tuning_frequency();
	void update_rf_amp();
	void update_lna();
	void update_baseband_bandwidth();
	void update_vga();
	void update_baseband_configuration();
	void update_headphone_volume();

	void baseband_disable();
};

#endif/*__RECEIVER_MODEL_H__*/
