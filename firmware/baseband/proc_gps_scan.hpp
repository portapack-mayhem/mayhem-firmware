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

#ifndef __PROC_GPS_SCAN_H__
#define __PROC_GPS_SCAN_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"
#include "gps_acq_dsp.hpp"

class GPSScanProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    enum class State : uint8_t {
        Idle = 0,
        Calibrating = 1,
        Capturing = 2,
        Scanning = 3,
        BuildProfile = 4,
    };

    void apply_pending_config();
    void start_new_scan();
    void run_selftest_capture();
    void step_scan();
    void build_and_send_profile(uint8_t prn);
    void send_stats(gps_scan::ScanState scan_state, uint8_t progress_pct);

    static constexpr uint32_t baseband_fs = gps_scan::SAMPLING_RATE_HZ;

    State state_{State::Idle};
    gps_scan::Config config_{};
    gps_scan::Config pending_config_{};
    volatile bool config_pending_{false};

    // Integration geometry for current scan
    uint8_t coh_ms_{2};
    uint8_t num_blocks_{4};
    uint8_t target_ms_{8};
    uint8_t num_sub_{4};
    int16_t sub_step_hz_{250};
    int16_t k_max_{5};
    uint8_t prn_start_{1};
    uint8_t prn_end_{32};

    // Capture state
    gps_scan::RawStatsAccum idle_acc_{};
    gps_scan::RawStatsAccum cal_acc_{};
    gps_scan::RawStatsAccum cap_acc_{};
    gps_scan::CaptureCal cal_{};
    gps_scan::SignalStats snapshot_stats_{};
    uint32_t dma_drops_start_{0};
    uint32_t dma_buffers_{0};
    uint16_t idle_buf_count_{0};
    uint8_t captured_ms_{0};
    bool has_valid_snapshot_{false};
    uint8_t last_profile_prn_{0};

    // Scan progress state
    uint32_t scan_id_{0};
    systime_t scan_start_time_{0};
    uint32_t ifft_count_{0};
    uint32_t total_steps_{1};
    uint32_t done_steps_{0};
    uint8_t cur_sub_{0};
    uint8_t cur_prn_{1};
    int16_t cur_k_{0};
    bool need_sub_fft_{false};
    bool need_prn_fft_{false};
    uint8_t detected_count_{0};
    uint8_t last_reported_pct_{255};

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};
};

#endif /* __PROC_GPS_SCAN_H__ */
