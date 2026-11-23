/*
 * Copyright (C) 2026 zxkmm
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
 * the Free Software Foundation; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __GPS_SCAN_TYPES_H__
#define __GPS_SCAN_TYPES_H__

#include <cstdint>
#include <cstddef>

namespace gps_scan {

static constexpr uint32_t SAMPLING_RATE_HZ = 2'048'000;
static constexpr uint32_t BASEBAND_BW_HZ = 2'500'000;
static constexpr uint64_t GPS_L1_FREQ_HZ = 1'575'420'000ULL;
static constexpr size_t SAMPLES_PER_MS = 2048;
static constexpr size_t MAX_SNAPSHOT_MS = 16;
static constexpr size_t NUM_PRNS = 32;
static constexpr size_t PROFILE_BINS = 64;
static constexpr size_t TRACK_IQ_HISTORY = 32;
static constexpr size_t MAX_NAV_CH = 5;

enum class Mode : uint8_t {
    Idle = 0,
    ScanSingle = 1,
    ScanAuto = 2,
    SelfTest = 3,
    Track = 4,
    TrackSelfTest = 5,
    NavFix = 6,
    NavFixSelfTest = 7,
};

enum class TrackState : uint8_t {
    Off = 0,
    Reacq = 1,
    PullIn = 2,
    WidePLL = 3,
    Locked = 4,
    Lost = 5,
};

enum class Integration : uint8_t {
    Ms1x1 = 0,  // 1 ms coherent x 1 non-coherent =  1 ms total, 500 Hz step
    Ms1x4 = 1,  // 1 ms coherent x 4 non-coherent =  4 ms total, 500 Hz step
    Ms2x4 = 2,  // 2 ms coherent x 4 non-coherent =  8 ms total, 250 Hz step
    Ms4x4 = 3,  // 4 ms coherent x 4 non-coherent = 16 ms total, 125 Hz step
};

enum class ScanState : uint8_t {
    Idle = 0,
    Capturing = 1,
    Scanning = 2,
    Done = 3,
};

enum class DetStatus : uint8_t {
    Noise = 0,
    Weak = 1,
    Lock = 2,
};

struct NavTarget {
    uint8_t prn{0};
    int16_t dopp_hint_hz{0};
};

struct Config {
    Mode mode{Mode::Idle};
    Integration integration{Integration::Ms2x4};
    uint8_t doppler_khz{5};        // ±kHz: 5, 10, 20, or 35
    uint8_t prn_filter{0};         // 0 = All (1..32), 1..32 = single PRN (or tracked PRN)
    uint8_t profile_prn{0};        // 0 = auto strongest, 1..32 = specific PRN
    int16_t track_dopp_hint_hz{0}; // Initial Doppler hint (Hz) when entering Mode::Track
    uint8_t nav_count{0};          // Number of targets in nav_targets (1..MAX_NAV_CH)
    NavTarget nav_targets[MAX_NAV_CH]{};
};

struct SignalStats {
    int16_t mean_i_x10{0};
    int16_t mean_q_x10{0};
    uint16_t rms_i_x10{0};
    uint16_t rms_q_x10{0};
    uint8_t peak_abs_i{0};
    uint8_t peak_abs_q{0};
    uint16_t clip_pm{0};  // per-mille (0..1000 => 0.0..100.0%)
    uint8_t q2_pct[4]{0, 0, 0, 0};
    uint16_t dma_drops{0};
    uint16_t dma_buffers{0};
    uint8_t snapshot_ms{0};
    uint8_t is_selftest{0};
};

struct PRNResult {
    uint8_t prn{0};            // 1..32
    uint8_t status{0};         // DetStatus
    int16_t doppler_hz{0};     // Best Doppler offset in Hz
    uint16_t code_phase{0};    // Sample offset 0..2047 (chip = code_phase * 1023 / 2048.0)
    uint32_t peak_power{0};    // Scaled correlation power at peak
    uint32_t second_peak{0};   // Highest peak outside ±4 samples (±2 chips)
    uint32_t noise_mean{0};    // Noise mean outside ±4 samples
    uint32_t noise_std_x10{0}; // Noise std dev * 10
    uint16_t ratio_x100{0};    // peak / second_peak * 100
    uint16_t z_score_x10{0};   // (peak - mean) / std * 10
    int16_t cn0_dbhz_x10{0};   // Estimated C/N0 * 10 in dB-Hz
};

struct CorrProfile {
    PRNResult result{};
    uint8_t bins[PROFILE_BINS]{};
    uint8_t mean_level{0};
    uint8_t second_level{0};
    uint32_t scan_id{0};
};

struct TrackIQ {
    int8_t i;
    int8_t q;
};

struct TrackStatus {
    uint8_t prn{0};
    uint8_t state{0};           // TrackState
    uint8_t bit_synced{0};      // 1 if 20ms nav bit edge locked
    uint8_t frame_synced{0};    // 0=none, 1=TLM 0x8B norm, 2=TLM 0x74 inv
    int32_t doppler_hz_x10{0};  // Carrier Doppler * 10
    uint32_t code_chip_x100{0}; // Code phase * 100 (0..102299)
    int16_t cn0_dbhz_x10{0};    // Live C/N0 * 10 (dB-Hz)
    int16_t pli_x100{0};        // Carrier Phase Lock Indicator * 100 (-100..+100)
    uint16_t early_mag{0};      // Early correlator magnitude (scaled)
    uint16_t prompt_mag{0};     // Prompt correlator magnitude (scaled)
    uint16_t late_mag{0};       // Late correlator magnitude (scaled)
    uint16_t track_sec{0};      // Seconds tracked
    uint16_t dma_drops{0};      // DMA drops during tracking
    uint32_t total_bits{0};     // Total 50 bps bits decoded
    uint64_t recent_bits{0};    // Last 64 decoded 50 bps bits (LSB = newest)

    // Subframe telemetry (IS-GPS-200 LNAV)
    uint32_t tow_sec{0};        // GPS Time of Week in seconds (0..604794)
    uint16_t gps_week{0};       // Full GPS week (2048 + 10-bit WN) from SF1
    uint8_t sf_mask{0};         // Bitmask of received subframes (bits 0..4 => SF1..SF5)
    uint8_t last_sf_id{0};      // Last subframe ID (1..5)
    uint8_t last_parity_ok{0};  // Words passing parity in last subframe (0..10)
    uint8_t sv_health{0xFF};    // SF1 SV Health (0 = OK, 0xFF = unknown)
    uint8_t ura_idx{0xFF};      // SF1 URA index
    uint8_t last_iode{0};       // SF2/SF3 IODE
    uint8_t alert_as{0};        // Bit 0 = Alert, Bit 1 = Anti-Spoof
    uint8_t is_selftest{0};     // 1 if synthetic tracking self-test
    uint16_t frames_ok{0};      // Subframes passing 10/10 parity
    uint16_t frames_err{0};     // Subframes with parity error

    TrackIQ iq_hist[TRACK_IQ_HISTORY]{};
};

// Verified 10-word LNAV subframe (24-bit recovered data words d1..d24) sent M4 -> M0
struct NavSubframe {
    uint8_t prn{0};
    uint8_t sf_id{0};        // 1..5
    uint32_t words[10]{};    // Recovered 24-bit data words (parity stripped)
};

// Per-channel simultaneous observation at a common receiver DMA instant
struct NavChanObs {
    uint8_t prn{0};
    uint8_t state{0};           // TrackState
    uint8_t bit_synced{0};
    uint8_t frame_synced{0};
    uint8_t sf_mask{0};         // Bits 0..4 => SF1..SF5 decoded with 10/10 parity
    uint8_t ms_in_bit{0};       // 0..19
    uint16_t bit_in_sf{0};      // 0..299
    int16_t cn0_dbhz_x10{0};
    int16_t pli_x100{0};
    int32_t doppler_hz_x10{0};
    uint32_t sf_start_tow{0};   // Exact GPS TOW (seconds) of bit 0 of current subframe
    uint32_t chip_q20{0};       // Exact fractional C/A chip phase * 2^20 (0 .. 1023*2^20 - 1)
};

struct NavObsPacket {
    uint8_t num_ch{0};
    uint8_t phase{0};           // 0=Idle, 1=Reacq, 2=Tracking
    uint8_t reacq_idx{0};       // 0..num_ch while re-acquiring
    uint8_t is_selftest{0};
    uint16_t track_sec{0};
    uint16_t dma_drops{0};
    uint16_t cpu_us_pack{0};    // Peak M4 us/ms (last 500 ms) for pack_from_raw (or GPSNavSim)
    uint16_t cpu_us_nav{0};     // Peak M4 us/ms (last 500 ms) for 5-ch GPSNavBank tracking
    NavChanObs ch[MAX_NAV_CH]{};
};

} /* namespace gps_scan */

#endif /* __GPS_SCAN_TYPES_H__ */
