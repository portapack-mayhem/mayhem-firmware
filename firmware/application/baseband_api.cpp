/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
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

#include "baseband_api.hpp"

#include "audio.hpp"
#include "tonesets.hpp"
#include "dsp_iir_config.hpp"

#include "portapack_shared_memory.hpp"
#include "portapack_persistent_memory.hpp"

#include "core_control.hpp"

/* Set true to enable additional checks to ensure
 * M4 and M0 are synchronized before passing messages. */
static constexpr bool enforce_core_sync = true;

/* Set true to enable check for baseband messages getting stuck.
 * This implies the baseband thread is not dequeuing and has probably stalled.
 * NB: This check adds a small amout of overhead to the message sending code
 * and may impact application perf if it is sending a lot of messages. */
static constexpr bool check_for_message_hang = true;

using namespace portapack;

namespace baseband {

static void send_message(const Message* const message) {
    // If message is only sent by this function via one thread, no need to check if
    // another message is present before setting new message.
    shared_memory.baseband_message = message;
    creg::m0apptxevent::assert_event();

    if constexpr (check_for_message_hang) {
        auto count = UINT32_MAX;
        while (shared_memory.baseband_message && --count)
            /* spin */;

        if (count == 0)
            chDbgPanic("Baseband Send Fail");
    } else {
        while (shared_memory.baseband_message)
            /* spin */;
    }
}

void AMConfig::apply() const {
    const AMConfigureMessage message{
        taps_6k0_decim_0,  // common FIR filter taps pre-decim_0 to all 5 x AM mod types.(AM-9K, AM-6K, USB, LSB, CW)
        taps_6k0_decim_1,  // common FIR filter taps pre-decim_1 to all 5 x AM mod. types.
        decim_2,           // var decim_2 FIR taps filter , variable values, depending selected  AM mod(AM  9k / 6k all rest AM modes)
        channel,           // var channel FIR taps filter , variable values, depending selected  AM mode, each one different  (DSB-9K, DSB-6K, USB-3K, LSB-3K,CW)
        modulation,        // var parameter .
        audio_12k_hpf_300hz_config};
    send_message(&message);
    audio::set_rate(audio::Rate::Hz_12000);
}

void NBFMConfig::apply(const uint8_t squelch_level) const {
    const NBFMConfigureMessage message{
        decim_0,
        decim_1,
        channel,
        2,
        deviation,
        audio_24k_hpf_300hz_config,
        audio_24k_deemph_300_6_config,
        squelch_level};
    send_message(&message);
    audio::set_rate(audio::Rate::Hz_24000);
}

void WFMConfig::apply() const {
    const WFMConfigureMessage message{
        decim_0,  // 	taps_200k_decim_0 , 	taps_180k_wfm_decim_0, taps_40k_wfm_decim_0
        decim_1,  // 	taps_200k_decim_1 or 	taps_180k_wfm_decim_1, taps_40k_wfm_decim_1
        taps_64_lp_156_198,
        75000,
        audio_48k_hpf_30hz_config,
        audio_48k_deemph_2122_6_config};
    send_message(&message);
    audio::set_rate(audio::Rate::Hz_48000);
}

void set_tone(const uint32_t index, const uint32_t delta, const uint32_t duration) {
    shared_memory.bb_data.tones_data.tone_defs[index].delta = delta;
    shared_memory.bb_data.tones_data.tone_defs[index].duration = duration;
}

void set_tones_config(const uint32_t bw, const uint32_t pre_silence, const uint16_t tone_count, const bool dual_tone, const bool audio_out) {
    const TonesConfigureMessage message{
        bw,
        pre_silence,
        tone_count,
        dual_tone,
        audio_out};
    send_message(&message);
}

void kill_tone() {
    const TonesConfigureMessage message{
        0,
        0,
        0,
        false,
        false};
    send_message(&message);
}

void set_sstv_data(const uint8_t vis_code, const uint32_t pixel_duration) {
    const SSTVConfigureMessage message{
        vis_code,
        pixel_duration};
    send_message(&message);
}

void set_afsk(const uint32_t baudrate, const uint32_t word_length, const uint32_t trigger_value, const bool trigger_word) {
    const AFSKRxConfigureMessage message{
        baudrate,
        word_length,
        trigger_value,
        trigger_word};
    send_message(&message);
}

void set_aprs(const uint32_t baudrate) {
    const APRSRxConfigureMessage message{
        baudrate};
    send_message(&message);
}

void set_btlerx(uint8_t channel_number) {
    const BTLERxConfigureMessage message{
        channel_number};
    send_message(&message);
}

void set_btletx(uint8_t channel_number, char* macAddress, char* advertisementData, uint8_t pduType) {
    const BTLETxConfigureMessage message{
        channel_number,
        macAddress,
        advertisementData,
        pduType};
    send_message(&message);
}

void set_nrf(const uint32_t baudrate, const uint32_t word_length, const uint32_t trigger_value, const bool trigger_word) {
    const NRFRxConfigureMessage message{
        baudrate,
        word_length,
        trigger_value,
        trigger_word};
    send_message(&message);
}

void set_fsk(const size_t deviation) {
    const FSKRxConfigureMessage message{
        taps_200k_decim_0,
        taps_16k0_decim_1,
        taps_11k0_channel,
        2,
        deviation};

    send_message(&message);
}

void set_afsk_data(const uint32_t afsk_samples_per_bit, const uint32_t afsk_phase_inc_mark, const uint32_t afsk_phase_inc_space, const uint8_t afsk_repeat, const uint32_t afsk_bw, const uint8_t symbol_count) {
    const AFSKTxConfigureMessage message{
        afsk_samples_per_bit,
        afsk_phase_inc_mark,
        afsk_phase_inc_space,
        afsk_repeat,
        afsk_bw,
        symbol_count};
    send_message(&message);
}

void kill_afsk() {
    const AFSKTxConfigureMessage message{
        0,
        0,
        0,
        0,
        0,
        false};
    send_message(&message);
}

void set_audiotx_config(
    const uint32_t divider,
    const float deviation_hz,
    const float audio_gain,
    uint8_t audio_shift_bits_s16,
    uint8_t bits_per_sample,
    const uint32_t tone_key_delta,
    const bool am_enabled,
    const bool dsb_enabled,
    const bool usb_enabled,
    const bool lsb_enabled) {
    const AudioTXConfigMessage message{
        divider,
        deviation_hz,
        audio_gain,
        audio_shift_bits_s16,
        bits_per_sample,
        tone_key_delta,
        (float)persistent_memory::tone_mix() / 100.0f,
        am_enabled,
        dsb_enabled,
        usb_enabled,
        lsb_enabled};
    send_message(&message);
}

void set_fifo_data(const int8_t* data) {
    const FIFODataMessage message{
        data};
    send_message(&message);
}

void set_pitch_rssi(int32_t avg, bool enabled) {
    const PitchRSSIConfigureMessage message{
        enabled,
        avg};
    send_message(&message);
}

void set_ook_data(const uint32_t stream_length, const uint32_t samples_per_bit, const uint8_t repeat, const uint32_t pause_symbols, const uint8_t de_bruijn_length) {
    const OOKConfigureMessage message{
        stream_length,
        samples_per_bit,
        repeat,
        pause_symbols,
        de_bruijn_length};
    send_message(&message);
}

void kill_ook() {
    const OOKConfigureMessage message{
        0,
        0,
        0,
        0,
        0};
    send_message(&message);
}

void set_fsk_data(const uint32_t stream_length, const uint32_t samples_per_bit, const uint32_t shift, const uint32_t progress_notice) {
    const FSKConfigureMessage message{
        stream_length,
        samples_per_bit,
        shift,
        progress_notice};
    send_message(&message);
}

void set_pocsag() {
    const POCSAGConfigureMessage message{};
    send_message(&message);
}

void set_adsb() {
    const ADSBConfigureMessage message{};
    send_message(&message);
}

void set_jammer(const bool run, const jammer::JammerType type, const uint32_t speed) {
    const JammerConfigureMessage message{
        run,
        type,
        speed};
    send_message(&message);
}

void set_rds_data(const uint16_t message_length) {
    const RDSConfigureMessage message{
        message_length};
    send_message(&message);
}

void set_spectrum(const size_t sampling_rate, const size_t trigger) {
    const WidebandSpectrumConfigMessage message{
        sampling_rate, trigger};
    send_message(&message);
}

void set_siggen_tone(const uint32_t tone) {
    const SigGenToneMessage message{
        TONES_F2D(tone, TONES_SAMPLERATE)};
    send_message(&message);
}

void set_siggen_config(const uint32_t bw, const uint32_t shape, const uint32_t duration) {
    const SigGenConfigMessage message{
        bw, shape, duration * TONES_SAMPLERATE};
    send_message(&message);
}

void set_spectrum_painter_config(const uint16_t width, const uint16_t height, bool update, int32_t bw) {
    SpectrumPainterBufferConfigureRequestMessage message{width, height, update, bw};
    send_message(&message);
}

void set_subghzd_config(uint8_t modulation = 0, uint32_t sampling_rate = 0) {
    const SubGhzFPRxConfigureMessage message{modulation, sampling_rate};
    send_message(&message);
}

static bool baseband_image_running = false;

void run_image(const spi_flash::image_tag_t image_tag) {
    if (baseband_image_running) {
        chDbgPanic("BBRunning");
    }

    creg::m4txevent::clear();
    shared_memory.clear_baseband_ready();

    m4_init(image_tag, memory::map::m4_code, false);
    baseband_image_running = true;

    creg::m4txevent::enable();

    if constexpr (enforce_core_sync) {
        // Wait up to 3 seconds for baseband to start handling events.
        auto count = 3'000u;
        while (!shared_memory.baseband_ready && --count)
            chThdSleepMilliseconds(1);

        if (count == 0)
            chDbgPanic("Baseband Sync Fail");
    }
}

void run_prepared_image(const uint32_t m4_code) {
    if (baseband_image_running) {
        chDbgPanic("BBRunning");
    }

    creg::m4txevent::clear();
    shared_memory.clear_baseband_ready();

    m4_init_prepared(m4_code, false);
    baseband_image_running = true;

    creg::m4txevent::enable();

    if constexpr (enforce_core_sync) {
        // Wait up to 3 seconds for baseband to start handling events.
        auto count = 3'000u;
        while (!shared_memory.baseband_ready && --count)
            chThdSleepMilliseconds(1);

        if (count == 0)
            chDbgPanic("Baseband Sync Fail");
    }
}

void shutdown() {
    if (!baseband_image_running) {
        return;
    }

    creg::m4txevent::disable();

    ShutdownMessage message;
    send_message(&message);

    shared_memory.application_queue.reset();

    baseband_image_running = false;
}

void spectrum_streaming_start() {
    SpectrumStreamingConfigMessage message{
        SpectrumStreamingConfigMessage::Mode::Running};
    send_message(&message);
}

void spectrum_streaming_stop() {
    SpectrumStreamingConfigMessage message{
        SpectrumStreamingConfigMessage::Mode::Stopped};
    send_message(&message);
}

void set_sample_rate(uint32_t sample_rate, OversampleRate oversample_rate) {
    SampleRateConfigMessage message{sample_rate, oversample_rate};
    send_message(&message);
}

void capture_start(CaptureConfig* const config) {
    CaptureConfigMessage message{config};
    send_message(&message);
}

void capture_stop() {
    CaptureConfigMessage message{nullptr};
    send_message(&message);
}

void replay_start(ReplayConfig* const config) {
    ReplayConfigMessage message{config};
    send_message(&message);
}

void replay_stop() {
    ReplayConfigMessage message{nullptr};
    send_message(&message);
}

void request_beep(RequestSignalMessage::Signal beep_type) {
    RequestSignalMessage message{beep_type};
    send_message(&message);
}

void request_roger_beep() {
    request_beep(RequestSignalMessage::Signal::RogerBeepRequest);
}

void request_rssi_beep() {
    request_beep(RequestSignalMessage::Signal::RSSIBeepRequest);
}

void request_beep_stop() {
    request_beep(RequestSignalMessage::Signal::BeepStopRequest);
}

void request_audio_beep(uint32_t freq, uint32_t sample_rate, uint32_t duration_ms) {
    AudioBeepMessage message{freq, sample_rate, duration_ms};
    send_message(&message);
}

} /* namespace baseband */
