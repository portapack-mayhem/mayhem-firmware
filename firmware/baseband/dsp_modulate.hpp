/*
 * Copyright (C) 2020 Belousov Oleg
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

#ifndef __DSP_MODULATE_H__
#define __DSP_MODULATE_H__

#include "dsp_types.hpp"
#include "dsp_hilbert.hpp"
#include "tone_gen.hpp"
#include "baseband_processor.hpp"

namespace dsp {
namespace modulate {

enum class Mode {
	None,
	AM,
	DSB,
	LSB,
	USB,
	FM
};

///

class Modulator {
public:
	virtual void execute(const buffer_s16_t& audio, const buffer_c8_t& buffer,bool& configured_in, uint32_t& new_beep_index, uint32_t& new_beep_timer, TXProgressMessage& new_txprogress_message, AudioLevelReportMessage& new_level_message, uint32_t& new_power_acc_count, uint32_t& new_divider   ) = 0;
	virtual ~Modulator();

	Mode get_mode();
	void set_mode(Mode new_mode);

    void set_over(uint32_t new_over);
	void set_gain_vumeter_beep(float new_audio_gain , bool new_play_beep );
	int32_t apply_beep(int32_t sample_in, bool& configured_in, uint32_t& new_beep_index, uint32_t& new_beep_timer, TXProgressMessage& new_txprogress_message );
	float audio_gain { };
	bool play_beep { false };
	uint32_t power_acc_count { 0 };		// this var it is initialized from Proc_mictx.cpp
	uint32_t divider { };				// this var it is initialized from Proc_mictx.cpp
    uint64_t power_acc { 0 };	// it is aux Accumulated sum (Absolute sample signal) , initialitzed to zero. 
	AudioLevelReportMessage level_message { };	

private: 
	static constexpr size_t baseband_fs = 1536000U;
	TXProgressMessage txprogress_message { };
   	ToneGen  beep_gen { };
	uint32_t beep_index { }, beep_timer { };


protected:
    uint32_t    over = 1;
	Mode	    mode = Mode::None;
};

///

class SSB : public Modulator {
public:
	SSB();

	virtual void execute(const buffer_s16_t& audio, const buffer_c8_t& buffer, bool& configured_in, uint32_t& new_beep_index, uint32_t& new_beep_timer, TXProgressMessage& new_txprogress_message, AudioLevelReportMessage& new_level_message, uint32_t& new_power_acc_count, uint32_t& new_divider  );

private:
	dsp::HilbertTransform	hilbert;
};

///

class FM : public Modulator {
public:
	FM();

	virtual void execute(const buffer_s16_t& audio, const buffer_c8_t& buffer, bool& configured_in,  uint32_t& new_beep_index, uint32_t& new_beep_timer, TXProgressMessage& new_txprogress_message, AudioLevelReportMessage& new_level_message, uint32_t& new_power_acc_count, uint32_t& new_divider  ) ;
	void set_fm_delta(uint32_t new_delta);
	void set_tone_gen_configure(const uint32_t delta, const float tone_mix_weight); 

///

private:
	uint32_t	fm_delta { 0 };
	uint32_t	phase { 0 }, sphase { 0 };
	int32_t		sample { 0 }, delta { };
	ToneGen 	tone_gen { };
	

};

class AM : public Modulator {
public:
        AM();

        virtual void execute(const buffer_s16_t& audio, const buffer_c8_t& buffer, bool& configured_in, uint32_t& new_beep_index, uint32_t& new_beep_timer, TXProgressMessage& new_txprogress_message, AudioLevelReportMessage& new_level_message, uint32_t& new_power_acc_count, uint32_t& new_divider  );
};

} /* namespace modulate */
} /* namespace dsp */

#endif/*__DSP_MODULATE_H__*/
