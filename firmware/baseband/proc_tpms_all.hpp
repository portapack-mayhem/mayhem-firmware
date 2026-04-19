/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2023 Mark Thompson
 * Copyright (C) 2026 Speedster04
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

#ifndef __PROC_TPMS_ALL_H__
#define __PROC_TPMS_ALL_H__

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

// Matched filter: 307.2 kHz -> 38400 Hz output (decimation=8)
// Serves all FSK paths: 19200 bps standard + NRZI + ~40000 bps BMW
constexpr std::array<std::complex<float>, 16> rect_taps_307k2_38k4_all{{
    {6.2500000000e-02f, 0.0000000000e+00f},
    {4.4194173824e-02f, 4.4194173824e-02f},
    {0.0000000000e+00f, 6.2500000000e-02f},
    {-4.4194173824e-02f, 4.4194173824e-02f},
    {-6.2500000000e-02f, 0.0000000000e+00f},
    {-4.4194173824e-02f, -4.4194173824e-02f},
    {0.0000000000e+00f, -6.2500000000e-02f},
    {4.4194173824e-02f, -4.4194173824e-02f},
    {6.2500000000e-02f, 0.0000000000e+00f},
    {4.4194173824e-02f, 4.4194173824e-02f},
    {0.0000000000e+00f, 6.2500000000e-02f},
    {-4.4194173824e-02f, 4.4194173824e-02f},
    {-6.2500000000e-02f, 0.0000000000e+00f},
    {-4.4194173824e-02f, -4.4194173824e-02f},
    {0.0000000000e+00f, -6.2500000000e-02f},
    {4.4194173824e-02f, -4.4194173824e-02f},
}};

class TPMSAllProcessor : public BasebandProcessor {
   public:
    TPMSAllProcessor();
    void execute(const buffer_c8_t& buffer) override;

   private:
    static constexpr size_t baseband_fs = 2457600;

    std::array<complex16_t, 512> dst{};
    const buffer_c16_t dst_buffer{dst.data(), dst.size()};

    dsp::decimate::FIRC8xR16x24FS4Decim4 decim_0{};
    dsp::decimate::FIRC16xR16x16Decim2 decim_1{};

    dsp::matched_filter::MatchedFilter mf{rect_taps_307k2_38k4_all, 8};

    // -----------------------------------------------------------------------
    // FSK 19200 bps -> drives Schrader preamble + Elantra2012 preamble
    // Schrader/FLM/Citroen/Renault/Jansite/HyundaiVDO/Abarth/Ren0435R/TruckSolar/Nissan
    // all share the Schrader preamble (0x55..56); Elantra2012 uses its own
    // preamble (0x7155) on the same clock recovery path.
    // -----------------------------------------------------------------------
    clock_recovery::ClockRecovery<clock_recovery::FixedErrorFilter> clock_recovery_19k2{
        38400,
        19200,
        {0.0555f},
        [this](const float raw_symbol) {
            const uint_fast8_t s = (raw_symbol >= 0.0f) ? 1 : 0;
            this->pb_schrader.execute(s);
            this->pb_elantra2012.execute(s);
        }};

    // Preamble 0x55 0x56 (30 bits) -> FLM, Schrader, GMC, Citroen, Renault,
    //                                  Jansite TY02S, HyundaiVDO, Abarth,
    //                                  Renault_0435R, TruckSolar, Nissan
    PacketBuilder<BitPattern, NeverMatch, FixedLength> pb_schrader{
        {0b010101010101010101010101010110, 30, 1},
        {},
        {160},
        [this](const baseband::Packet& packet) {
            shared_memory.application_queue.push(
                TPMSPacketMessage{tpms::SignalType::FSK_19k2_Schrader, packet});
        }};

    // Preamble 0x7155 (16 bits) -> Elantra 2012 / TRW, payload 128 Manchester
    // bits = 64 data bits = 8 bytes. The preamble is NOT Manchester-encoded --
    // it's the raw sync word 0111 0001 0101 0101, so we match 16 raw bits.
    // After the preamble, the reader sees 8 bytes Manchester-decoded.
    // Reference: rtl_433/src/devices/tpms_elantra2012.c
    PacketBuilder<BitPattern, NeverMatch, FixedLength> pb_elantra2012{
        {0b0111000101010101, 16, 0},
        {},
        {128},
        [](const baseband::Packet& packet) {
            shared_memory.application_queue.push(
                TPMSPacketMessage{tpms::SignalType::FSK_19k2_Elantra2012, packet});
        }};

    // -----------------------------------------------------------------------
    // FSK 19200 bps -> Jansite Solar (separate clock, distinct preamble)
    // -----------------------------------------------------------------------
    clock_recovery::ClockRecovery<clock_recovery::FixedErrorFilter> clock_recovery_jansite{
        38400,
        19200,
        {0.0555f},
        [this](const float raw_symbol) {
            const uint_fast8_t s = (raw_symbol >= 0.0f) ? 1 : 0;
            this->pb_jansite_solar.execute(s);
        }};

    // Preamble 0xa6a65a5a (32 raw bits) -> decoded 0xDD33 sync word.
    // The 4 most-significant preamble bits (decoded 0xD) are what
    // bitbuffer_search in rtl_433 effectively locks onto; we extend to the
    // full 2-byte sync so the reader starts aligned on the data portion.
    // Payload 144 raw bits = 9 decoded bytes:
    //   II II II 00 TT PP 00 CC CC
    PacketBuilder<BitPattern, NeverMatch, FixedLength> pb_jansite_solar{
        {0b10100110101001100101101001011010, 32, 1},
        {},
        {144},
        [](const baseband::Packet& packet) {
            shared_memory.application_queue.push(
                TPMSPacketMessage{tpms::SignalType::FSK_19k2_JansiteSolar, packet});
        }};

    // -----------------------------------------------------------------------
    // OOK paths (original -> Schrader OOK + GMC_96)
    // -----------------------------------------------------------------------
    static constexpr float channel_rate_in = 307200.0f;
    static constexpr size_t channel_decimation = 2;
    static constexpr float channel_sample_rate = channel_rate_in / channel_decimation;
    OOKSlicerMagSquaredInt ook_slicer_5sps{channel_sample_rate / 8400 + 1};
    uint32_t slicer_history{0};

    OOKClockRecovery clock_recovery_ook_8k192{channel_sample_rate / 8192.0f};
    PacketBuilder<BitPattern, NeverMatch, FixedLength> pb_ook_8k192{
        /* Preamble: 11*2, 01*14, 11, 10
         * Payload: 37 Manchester-encoded bits
         * Bit rate: 4096 Hz
         */
        {0b010101010101010101011110, 24, 0},
        {},
        {37 * 2},
        [](const baseband::Packet& packet) {
            shared_memory.application_queue.push(
                TPMSPacketMessage{tpms::SignalType::OOK_8k192_Schrader, packet});
        }};

    OOKClockRecovery clock_recovery_ook_8k4{channel_sample_rate / 8400.0f};
    PacketBuilder<BitPattern, NeverMatch, FixedLength> pb_ook_8k4{
        /* Preamble: 01*40, 01, 10, 01, 01
         * Payload: 76 Manchester-encoded bits
         * Bit rate: 4200 Hz
         */
        {0b01010101010101010101010101100101, 32, 0},
        {},
        {76 * 2},
        [](const baseband::Packet& packet) {
            shared_memory.application_queue.push(
                TPMSPacketMessage{tpms::SignalType::OOK_8k4_Schrader, packet});
        }};

    // Schrader SMD3MA4 / 3039 (Subaru, Renault Koleos, Nissan 370Z, Infiniti)
    // Preamble 0xF5555555E (36 raw bits, sense=1 for inverted decode).
    // Payload: 37 Manchester-decoded bits = 74 raw bits.
    // Shares clock_recovery_ook_8k4 with pb_ook_8k4 (both at ~8400 symbols/s);
    // wired together in proc_tpms_all.cpp callback.
    // Reference: rtl_433/src/devices/schraeder.c (SMD3MA4 branch)
    PacketBuilder<BitPattern, NeverMatch, FixedLength> pb_smd3ma4{
        {0b111101010101010101010101010101011110, 36, 1},
        {},
        {37 * 2},
        [](const baseband::Packet& packet) {
            shared_memory.application_queue.push(
                TPMSPacketMessage{tpms::SignalType::OOK_8k4_SMD3MA4, packet});
        }};

    void on_message(const Message* const message);
    void on_beep_message(const AudioBeepMessage& message);

    BasebandThread baseband_thread{
        baseband_fs, this, baseband::Direction::Receive, false};
    RSSIThread rssi_thread{};
};

#endif /*__PROC_TPMS_ALL_H__*/
