/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2023 Mark Thompson
 * Copyright (C) 2025 Speedster04 (TPMS EU M4 extensions: BMW G4/5, G2/3, Porsche)
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

#ifndef __PROC_TPMS_EU_H__
#define __PROC_TPMS_EU_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"

#include "channel_decimator.hpp"
#include "matched_filter.hpp"

#include "clock_recovery.hpp"
#include "symbol_coding.hpp"
#include "packet_builder.hpp"
#include "baseband_packet.hpp"

#include "ook.hpp"

#include "message.hpp"
#include "portapack_shared_memory.hpp"

#include <cstdint>
#include <cstddef>
#include <bitset>

// ---------------------------------------------------------------------------
// Matched filter taps — 307.2 kHz input, 38400 Hz center frequency
//
// rect_taps_307k2_38k4_1t_19k2_p  — 16 taps, decimation=8, output at 38400 Hz
//   Used for: FSK 19k2 (FLM, Schrader, Ford, Citroen, Renault, BMW G2/3, Porsche)
//   Symbol period: 1/19200 s = 2 output samples
//
// This is the SAME filter as in proc_tpms.hpp. It also serves BMW Gen4/5
// (~40kbps raw) with a second ClockRecovery targeting ~40000 bps:
//   The 16-tap filter is 2× the BMW G4/5 symbol period (2 × 1/38400 s = 2/38400 s
//   = 1/19200 s). This over-smooths by one symbol but the clock recovery
//   compensates for the ~4% rate difference (38400 vs ~40000 bps).
// ---------------------------------------------------------------------------
constexpr std::array<std::complex<float>, 16> rect_taps_307k2_38k4_1t_19k2_p{{
    {6.2500000000e-02f,  0.0000000000e+00f},
    {4.4194173824e-02f,  4.4194173824e-02f},
    {0.0000000000e+00f,  6.2500000000e-02f},
    {-4.4194173824e-02f, 4.4194173824e-02f},
    {-6.2500000000e-02f, 0.0000000000e+00f},
    {-4.4194173824e-02f,-4.4194173824e-02f},
    {0.0000000000e+00f, -6.2500000000e-02f},
    {4.4194173824e-02f, -4.4194173824e-02f},
    {6.2500000000e-02f,  0.0000000000e+00f},
    {4.4194173824e-02f,  4.4194173824e-02f},
    {0.0000000000e+00f,  6.2500000000e-02f},
    {-4.4194173824e-02f, 4.4194173824e-02f},
    {-6.2500000000e-02f, 0.0000000000e+00f},
    {-4.4194173824e-02f,-4.4194173824e-02f},
    {0.0000000000e+00f, -6.2500000000e-02f},
    {4.4194173824e-02f, -4.4194173824e-02f},
}};

class TPMSEUProcessor : public BasebandProcessor {
   public:
    TPMSEUProcessor();

    void execute(const buffer_c8_t& buffer) override;

   private:
    static constexpr size_t baseband_fs = 2457600;

    std::array<complex16_t, 512> dst{};
    const buffer_c16_t dst_buffer{dst.data(), dst.size()};

    dsp::decimate::FIRC8xR16x24FS4Decim4 decim_0{};
    dsp::decimate::FIRC16xR16x16Decim2   decim_1{};

    // Matched filter: 307.2 kHz → 38400 Hz output (decimation=8)
    // Serves both 19200 bps FSK path and ~40000 bps BMW G4/5 path
    dsp::matched_filter::MatchedFilter mf_38k4_1t_19k2{rect_taps_307k2_38k4_1t_19k2_p, 8};

    // -----------------------------------------------------------------------
    // FSK 19200 bps path
    // ClockRecovery: input 38400 Hz, target 19200 bps → 2 samples/symbol
    // Drives: Schrader/FLM/Ford/Citroen/Renault (original) +
    //         BMW Gen2/3 + Porsche (new preambles on same clock)
    // -----------------------------------------------------------------------
    clock_recovery::ClockRecovery<clock_recovery::FixedErrorFilter> clock_recovery_fsk_19k2{
        38400,
        19200,
        {0.0555f},
        [this](const float raw_symbol) {
            const uint_fast8_t sliced = (raw_symbol >= 0.0f) ? 1 : 0;
            this->packet_builder_fsk_19k2_schrader.execute(sliced);
            this->packet_builder_fsk_19k2_bmw_g23.execute(sliced);
            this->packet_builder_fsk_19k2_porsche.execute(sliced);
        }};

    // Preamble: 0x55 0x55 0x56 (30 bits trailing)
    // Protocols: FLM_64/72/80, Schrader, GMC_96, Ford, Citroen/PSA, Renault
    PacketBuilder<BitPattern, NeverMatch, FixedLength> packet_builder_fsk_19k2_schrader{
        {0b010101010101010101010101010110, 30, 1},
        {},
        {160},
        [this](const baseband::Packet& packet) {
            const TPMSPacketMessage message{tpms::SignalType::FSK_19k2_Schrader, packet};
            shared_memory.application_queue.push(message);
        }};

    // Preamble: 0xCCCD (16 bits) — BMW Gen2/3 differential Manchester (NRZI)
    // Payload: 90 raw NRZI bits = 90 decoded bits (11 bytes Gen3, 10 bytes Gen2)
    PacketBuilder<BitPattern, NeverMatch, FixedLength> packet_builder_fsk_19k2_bmw_g23{
        {0b1100110011001101, 16, 1},
        {},
        {90},
        [](const baseband::Packet& packet) {
            const TPMSPacketMessage message{tpms::SignalType::FSK_19k2_BMW_G23, packet};
            shared_memory.application_queue.push(message);
        }};

    // Preamble: 0x33 0x33 0x20 top 20 bits = 0b00110011001100110010
    // Porsche 987 Boxster/Cayman — NRZI, 80 raw bits = 10 decoded bytes
    PacketBuilder<BitPattern, NeverMatch, FixedLength> packet_builder_fsk_19k2_porsche{
        {0b00110011001100110010, 20, 1},
        {},
        {80},
        [](const baseband::Packet& packet) {
            const TPMSPacketMessage message{tpms::SignalType::FSK_19k2_Porsche, packet};
            shared_memory.application_queue.push(message);
        }};

    // -----------------------------------------------------------------------
    // FSK ~40000 bps path — BMW Gen4/5, Audi Pressure Alert
    //
    // Reuses the same mf_38k4_1t_19k2 matched filter output (38400 Hz).
    // ClockRecovery targets 40000 bps (GardnerTED × 2 → 80000 Hz resample target).
    // The 4% rate difference (38400 vs 40000) is within the resampler's tracking
    // capability, and the preamble + CRC-8 provide validation.
    //
    // Preamble: 0xAA59 (16 bits) — after matched filter output, raw bit sequence
    // Payload: 176 raw Manchester bits = 88 decoded bytes (11 BMW or 8 Audi)
    // -----------------------------------------------------------------------
    clock_recovery::ClockRecovery<clock_recovery::FixedErrorFilter> clock_recovery_fsk_38k4_bmw{
        38400.0f,   // matched filter output rate
        40000.0f,   // BMW Gen4/5 actual symbol rate (~40 kbps raw)
        {0.0555f},
        [this](const float raw_symbol) {
            const uint_fast8_t sliced = (raw_symbol >= 0.0f) ? 1 : 0;
            this->packet_builder_fsk_38k4_bmw_g45.execute(sliced);
        }};

    // Preamble: 0xAA59 (16 bits) — BMW Gen4/5 Manchester preamble
    // Payload: 176 raw Manchester bits → 88 decoded bits (11 bytes BMW / 8 bytes Audi)
    PacketBuilder<BitPattern, NeverMatch, FixedLength> packet_builder_fsk_38k4_bmw_g45{
        {0b1010101001011001, 16, 1},
        {},
        {176},
        [](const baseband::Packet& packet) {
            const TPMSPacketMessage message{tpms::SignalType::FSK_38k4_BMW_G45, packet};
            shared_memory.application_queue.push(message);
        }};

    // -----------------------------------------------------------------------
    // OOK paths (identical to proc_tpms — included for compatibility)
    // -----------------------------------------------------------------------
    static constexpr float channel_rate_in   = 307200.0f;
    static constexpr size_t channel_decimation = 2;
    static constexpr float channel_sample_rate = channel_rate_in / channel_decimation;
    OOKSlicerMagSquaredInt ook_slicer_5sps{channel_sample_rate / 8400 + 1};
    uint32_t slicer_history{0};

    OOKClockRecovery clock_recovery_ook_8k192{channel_sample_rate / 8192.0f};

    PacketBuilder<BitPattern, NeverMatch, FixedLength> packet_builder_ook_8k192_schrader{
        {0b010101010101010101011110, 24, 0},
        {},
        {37 * 2},
        [](const baseband::Packet& packet) {
            const TPMSPacketMessage message{tpms::SignalType::OOK_8k192_Schrader, packet};
            shared_memory.application_queue.push(message);
        }};

    OOKClockRecovery clock_recovery_ook_8k4{channel_sample_rate / 8400.0f};

    PacketBuilder<BitPattern, NeverMatch, FixedLength> packet_builder_ook_8k4_schrader{
        {0b01010101010101010101010101100101, 32, 0},
        {},
        {76 * 2},
        [](const baseband::Packet& packet) {
            const TPMSPacketMessage message{tpms::SignalType::OOK_8k4_Schrader, packet};
            shared_memory.application_queue.push(message);
        }};

    void on_message(const Message* const message);
    void on_beep_message(const AudioBeepMessage& message);

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{
        baseband_fs, this, baseband::Direction::Receive, /*auto_start*/ false};
    RSSIThread rssi_thread{};
};

#endif /*__PROC_TPMS_EU_H__*/
