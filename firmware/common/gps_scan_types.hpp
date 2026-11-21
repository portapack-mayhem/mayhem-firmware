/*
 * Copyright (C) 2026 PortaPack Mayhem Contributors
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

enum class Mode : uint8_t {
    Idle = 0,
    ScanSingle = 1,
    ScanAuto = 2,
    SelfTest = 3,
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

struct Config {
    Mode mode{Mode::Idle};
    Integration integration{Integration::Ms2x4};
    uint8_t doppler_khz{5};  // ±kHz: 5, 10, 20, or 35
    uint8_t prn_filter{0};   // 0 = All (1..32), 1..32 = single PRN
    uint8_t profile_prn{0};  // 0 = auto strongest, 1..32 = specific PRN
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

} /* namespace gps_scan */

#endif /* __GPS_SCAN_TYPES_H__ */
