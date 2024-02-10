/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2023 Mark Thompson
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

#include "audio.hpp"

#include "portapack.hpp"
using portapack::clock_manager;

#include "portapack_hal.hpp"
#include "portapack_persistent_memory.hpp"

#include "i2s.hpp"
using namespace lpc43xx;

namespace audio {

namespace {

// "Master": I2S peripheral generates SCK/WS, transmits to audio codec.

constexpr i2s::ConfigTX i2s0_config_tx_master_base_clk{
    .dao = i2s::DAO{
        .wordwidth = i2s::WordWidth::Bits16,
        .mono = 0,
        .stop = 1,
        .reset = 0,
        .ws_sel = 0,  // Master
        .ws_halfperiod = 0x0f,
        .mute = 1,
    },
    .txrate = i2s::MCLKRate{
        .x_divider = 0,
        .y_divider = 0,
    },
    .txbitrate = i2s::BitRate{
        .bitrate = 7,
    },
    .txmode = i2s::Mode{
        .clksel = i2s::ClockSelect::BaseAudioClkOrExternalMCLK,
        .four_pin = 0,
        .mclk_out_en = 1,
    },
    .sck_in_sel = 1,
};

constexpr i2s::ConfigRX i2s0_config_rx_four_wire{
    .dai = i2s::DAI{
        .wordwidth = i2s::WordWidth::Bits16,
        .mono = 0,
        .stop = 1,
        .reset = 0,
        .ws_sel = 0,  // Master
        .ws_halfperiod = 0x0f,
    },
    .rxrate = i2s::MCLKRate{
        .x_divider = 0,
        .y_divider = 0,
    },
    .rxbitrate = i2s::BitRate{
        .bitrate = 7,
    },
    .rxmode = i2s::Mode{
        .clksel = i2s::ClockSelect::FractionalDivider,
        .four_pin = 1,
        .mclk_out_en = 0,
    },
    .sck_in_sel = 0,
};

// "Slave": I2S controlled by external SCK/WS, received from audio codec.

constexpr i2s::ConfigTX i2s0_config_tx_slave_base_clk{
    .dao = i2s::DAO{
        .wordwidth = i2s::WordWidth::Bits16,
        .mono = 0,
        .stop = 1,
        .reset = 0,
        .ws_sel = 1,
        .ws_halfperiod = 0x0f,
        .mute = 1,
    },
    .txrate = i2s::MCLKRate{
        .x_divider = 0,
        .y_divider = 0,
    },
    .txbitrate = i2s::BitRate{
        .bitrate = 0,
    },
    .txmode = i2s::Mode{
        .clksel = i2s::ClockSelect::FractionalDivider,
        .four_pin = 0,
        .mclk_out_en = 1,
    },
    .sck_in_sel = 1,
};

constexpr i2s::ConfigDMA i2s0_config_dma{
    .dma1 = i2s::DMA{
        .rx_enable = 1,
        .tx_enable = 0,
        .rx_depth = 4,
        .tx_depth = 0,
    },
    .dma2 = i2s::DMA{
        .rx_enable = 0,
        .tx_enable = 1,
        .rx_depth = 0,
        .tx_depth = 4,
    },
};

static audio::Codec* audio_codec = nullptr;

} /* namespace */

namespace output {

static bool cfg_speaker_disable = false;
static bool nav_requested_mute = false;
static bool app_requested_mute = false;

void start() {
    i2s::i2s0::tx_start();
    unmute();
}

void stop() {
    mute();
    i2s::i2s0::tx_stop();
}

void mute() {
    app_requested_mute = true;
    i2s::i2s0::tx_mute();
    audio_codec->headphone_disable();
    audio_codec->speaker_disable();
}

void unmute() {
    app_requested_mute = false;
    if (!nav_requested_mute) {
        i2s::i2s0::tx_unmute();
        audio_codec->headphone_enable();
        if (!cfg_speaker_disable) {
            audio_codec->speaker_enable();
        }
    }
}

// The following functions are used by the navigation-bar Speaker Mute only,
// and override all other audio mute/unmute requests from apps
void speaker_mute() {
    nav_requested_mute = true;
    i2s::i2s0::tx_mute();
    audio_codec->speaker_disable();
    audio_codec->headphone_disable();
}

void speaker_unmute() {
    nav_requested_mute = false;
    if (!app_requested_mute) {
        unmute();
    }
}

void update_audio_mute() {
    cfg_speaker_disable = portapack::persistent_memory::config_speaker_disable();
    if (cfg_speaker_disable) {
        audio_codec->speaker_disable();
    }

    if (portapack::persistent_memory::config_audio_mute())
        speaker_mute();
    else
        speaker_unmute();
}

} /* namespace output */

namespace input {

void start(int8_t alc_mode, bool mic_to_HP_enabled) {
    audio_codec->microphone_enable(alc_mode, mic_to_HP_enabled);  // added user-GUI selection for AK4951, ALC mode parameter. and the check box "Hear Mic"
    i2s::i2s0::rx_start();
}

void stop() {
    i2s::i2s0::rx_stop();
    audio_codec->microphone_disable();
}

void loopback_mic_to_hp_enable() {
    audio_codec->microphone_to_HP_enable();
}

void loopback_mic_to_hp_disable() {
    audio_codec->microphone_to_HP_disable();
}

} /* namespace input */

namespace headphone {

volume_range_t volume_range() {
    return audio_codec->headphone_gain_range();
}

void set_volume(const volume_t volume) {
    audio_codec->set_headphone_volume(volume);
}

} /* namespace headphone */

namespace debug {

size_t reg_count() {
    return audio_codec->reg_count();
}

uint32_t reg_read(const size_t register_number) {
    return audio_codec->reg_read(register_number);
}

void reg_write(const size_t register_number, uint32_t value) {
    audio_codec->reg_write(register_number, value);
}

std::string codec_name() {
    return audio_codec->name();
}

size_t reg_bits() {
    return audio_codec->reg_bits();
}

} /* namespace debug */

void init(audio::Codec* const codec) {
    clock_manager.start_audio_pll();

    // Configure I2S before activating codec interface.
    i2s::i2s0::configure(
        i2s0_config_tx_master_base_clk,
        i2s0_config_rx_four_wire,
        i2s0_config_dma);

    audio_codec = codec;
    audio_codec->init();

    // Set pin mode, since it's likely GPIO (as left after CPLD JTAG interactions).
    portapack::pin_i2s0_rx_sda.mode(3);
}

void shutdown() {
    audio_codec->reset();
    input::stop();
    output::stop();

    i2s::i2s0::shutdown();

    clock_manager.stop_audio_pll();
}

void set_rate(const Rate rate) {
    clock_manager.set_base_audio_clock_divider(toUType(rate));
}

bool speaker_disable_supported() {
    return audio_codec->speaker_disable_supported();
}

} /* namespace audio */
