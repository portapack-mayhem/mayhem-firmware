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

#include "proc_gps_scan.hpp"
#include "portapack_shared_memory.hpp"
#include "event_m4.hpp"

#include <cstring>

using namespace gps_scan;

namespace {

// Static M4 SRAM buffers (~54.5 KiB total) so linker enforces exact RAM fit.
static uint8_t s_snapshot[MAX_SNAPSHOT_MS][SAMPLES_PER_MS / 2];  // 16 KiB (16 ms @ 2-bit I/Q)
static c8_t s_cached_fft[4][SAMPLES_PER_MS];                     // 16 KiB (4 coherent blocks)
static c8_t s_fft_code[SAMPLES_PER_MS];                          //  4 KiB
static c16_t s_fft_work[SAMPLES_PER_MS];                         //  8 KiB
static uint32_t s_accum[SAMPLES_PER_MS];                         //  8 KiB
static PRNResult s_results[NUM_PRNS];                            // ~1.1 KiB
static uint32_t s_best_pom[NUM_PRNS];                            // 128 B
static uint8_t s_best_sub[NUM_PRNS];                             //  32 B
static int16_t s_best_k[NUM_PRNS];                               //  64 B

}  // namespace

void GPSScanProcessor::apply_pending_config() {
    config_pending_ = false;
    const Config next = pending_config_;

    // Allow clicking a different PRN in Idle to re-extract its 64-bin profile
    // from the already-captured snapshot without re-scanning.
    if (next.mode == Mode::Idle && config_.mode == Mode::Idle &&
        state_ == State::Idle && has_valid_snapshot_ &&
        next.profile_prn != 0 && next.profile_prn != last_profile_prn_ &&
        next.profile_prn <= NUM_PRNS && s_results[next.profile_prn - 1].prn == next.profile_prn) {
        config_.profile_prn = next.profile_prn;
        build_and_send_profile(next.profile_prn);
        return;
    }

    config_ = next;

    if (config_.mode == Mode::Idle) {
        state_ = State::Idle;
        idle_acc_.reset();
        idle_buf_count_ = 0;
        send_stats(ScanState::Idle, 0);
        return;
    }

    start_new_scan();
}

void GPSScanProcessor::start_new_scan() {
    switch (config_.integration) {
        case Integration::Ms1x1:
            coh_ms_ = 1;
            num_blocks_ = 1;
            break;
        case Integration::Ms1x4:
            coh_ms_ = 1;
            num_blocks_ = 4;
            break;
        case Integration::Ms2x4:
        default:
            coh_ms_ = 2;
            num_blocks_ = 4;
            break;
        case Integration::Ms4x4:
            coh_ms_ = 4;
            num_blocks_ = 4;
            break;
    }

    target_ms_ = static_cast<uint8_t>(coh_ms_ * num_blocks_);
    num_sub_ = static_cast<uint8_t>(coh_ms_ * 2);
    sub_step_hz_ = static_cast<int16_t>(1000 / num_sub_);

    k_max_ = config_.doppler_khz;
    if (k_max_ < 1) k_max_ = 1;
    if (k_max_ > 40) k_max_ = 40;

    if (config_.prn_filter >= 1 && config_.prn_filter <= NUM_PRNS) {
        prn_start_ = config_.prn_filter;
        prn_end_ = config_.prn_filter;
    } else {
        prn_start_ = 1;
        prn_end_ = NUM_PRNS;
    }

    std::memset(s_results, 0, sizeof(s_results));
    std::memset(s_best_pom, 0, sizeof(s_best_pom));
    std::memset(s_best_sub, 0, sizeof(s_best_sub));
    std::memset(s_best_k, 0, sizeof(s_best_k));

    scan_id_++;
    ifft_count_ = 0;
    detected_count_ = 0;
    cur_sub_ = 0;
    cur_prn_ = prn_start_;
    cur_k_ = static_cast<int16_t>(-k_max_);
    need_sub_fft_ = true;
    need_prn_fft_ = true;
    last_reported_pct_ = 255;

    const uint32_t prn_cnt = static_cast<uint32_t>(prn_end_ - prn_start_ + 1);
    const uint32_t k_cnt = static_cast<uint32_t>(2 * k_max_ + 1);
    total_steps_ = static_cast<uint32_t>(num_sub_) * prn_cnt * k_cnt;
    if (total_steps_ == 0) total_steps_ = 1;
    done_steps_ = 0;

    has_valid_snapshot_ = false;
    state_ = State::Calibrating;
    send_stats(ScanState::Capturing, 0);
}

void GPSScanProcessor::run_selftest_capture() {
    // Reuse s_fft_work (8 KiB) as temporary 4 KiB raw int8 I/Q buffer (zero stack bloat).
    int8_t* raw_iq = reinterpret_cast<int8_t*>(s_fft_work);
    uint32_t prng = 0x12345678U ^ (scan_id_ * 2654435761U);

    cal_acc_.reset();
    generate_selftest_ms(0, prng, raw_iq);
    cal_acc_.measure_block(raw_iq, SAMPLES_PER_MS);
    cal_ = cal_acc_.derive_cal();

    cap_acc_.reset();
    for (uint8_t m = 0; m < target_ms_; m++) {
        generate_selftest_ms(m + 1, prng, raw_iq);
        cap_acc_.pack_ms_2bit(raw_iq, cal_, s_snapshot[m]);
    }

    snapshot_stats_ = cap_acc_.to_signal_stats(0, static_cast<uint16_t>(dma_buffers_ & 0xFFFFU), target_ms_, 1);
    has_valid_snapshot_ = true;
    scan_start_time_ = chTimeNow();
    state_ = State::Scanning;
    send_stats(ScanState::Scanning, 0);
}

void GPSScanProcessor::send_stats(ScanState scan_state, uint8_t progress_pct) {
    uint16_t elapsed_ms = 0;
    if (scan_state == ScanState::Scanning || scan_state == ScanState::Done) {
        const systime_t dt = chTimeNow() - scan_start_time_;
        elapsed_ms = static_cast<uint16_t>(dt > 65535U ? 65535U : dt);
    }

    SignalStats st = (scan_state == ScanState::Idle)
                         ? idle_acc_.to_signal_stats(0, static_cast<uint16_t>(dma_buffers_ & 0xFFFFU), 0, 0)
                         : snapshot_stats_;
    st.dma_buffers = static_cast<uint16_t>(dma_buffers_ & 0xFFFFU);

    const GPSScanStatsMessage msg{
        st,
        scan_state,
        progress_pct,
        cur_prn_,
        detected_count_,
        elapsed_ms,
        ifft_count_,
        scan_id_};
    shared_memory.application_queue.push(msg);
}

void GPSScanProcessor::build_and_send_profile(uint8_t prn) {
    if (prn < 1 || prn > NUM_PRNS) return;
    const PRNResult& res = s_results[prn - 1];
    if (res.prn != prn) return;

    const uint8_t sub = s_best_sub[prn - 1];
    const int16_t k = s_best_k[prn - 1];

    // Recompute cached_fft for the winning sub-bin and correlate at winning k
    for (uint8_t b = 0; b < num_blocks_; b++) {
        fold_coherent_block(s_snapshot, b * coh_ms_, coh_ms_, sub, s_fft_work);
        fft_2048_q15(s_fft_work, false);
        compress_fft_c8(s_fft_work, s_cached_fft[b], false);
    }
    generate_ca_2048(prn, s_fft_work);
    fft_2048_q15(s_fft_work, false);
    compress_fft_c8(s_fft_work, s_fft_code, true);
    correlate_doppler_bin(s_cached_fft, num_blocks_, s_fft_code, k, s_fft_work, s_accum);

    CorrProfile prof{};
    extract_corr_profile(s_accum, res, scan_id_, prof);
    last_profile_prn_ = prn;

    const GPSScanProfileMessage msg{prof};
    shared_memory.application_queue.push(msg);
}

void GPSScanProcessor::step_scan() {
    // 1. If starting a new sub-bin, fold & FFT the num_blocks coherent signal blocks
    if (need_sub_fft_) {
        for (uint8_t b = 0; b < num_blocks_; b++) {
            fold_coherent_block(s_snapshot, b * coh_ms_, coh_ms_, cur_sub_, s_fft_work);
            fft_2048_q15(s_fft_work, false);
            compress_fft_c8(s_fft_work, s_cached_fft[b], false);
        }
        need_sub_fft_ = false;
        cur_prn_ = prn_start_;
        need_prn_fft_ = true;
        return;  // Yield back to wait_for_buffer() so EventDispatcher stays responsive
    }

    // 2. If starting a new PRN, compute conj(FFT(CA_code))
    if (need_prn_fft_) {
        generate_ca_2048(cur_prn_, s_fft_work);
        fft_2048_q15(s_fft_work, false);
        compress_fft_c8(s_fft_work, s_fft_code, true);
        need_prn_fft_ = false;
        cur_k_ = static_cast<int16_t>(-k_max_);
    }

    // 3. Process a small slice of Doppler bins (~3..12 ms total) per execute() call
    //    so M0 commands (Stop / Shutdown / Reconfig) are handled within ~15 ms.
    const uint8_t k_budget = (num_blocks_ <= 1) ? 8 : 3;
    for (uint8_t step = 0; step < k_budget && cur_k_ <= k_max_; step++, cur_k_++) {
        const int16_t dopp_hz = static_cast<int16_t>(cur_k_ * 1000 + cur_sub_ * sub_step_hz_);
        done_steps_++;

        if (dopp_hz >= -k_max_ * 1000 && dopp_hz <= k_max_ * 1000) {
            correlate_doppler_bin(s_cached_fft, num_blocks_, s_fft_code, cur_k_, s_fft_work, s_accum);
            ifft_count_ += num_blocks_;

            const uint32_t pom = quick_peak_over_mean_q8(s_accum);
            if (pom > s_best_pom[cur_prn_ - 1]) {
                s_best_pom[cur_prn_ - 1] = pom;
                s_best_sub[cur_prn_ - 1] = cur_sub_;
                s_best_k[cur_prn_ - 1] = cur_k_;
                analyze_correlation(s_accum, cur_prn_, dopp_hz, coh_ms_, num_blocks_, s_results[cur_prn_ - 1]);
            }
        }
    }

    // 4. Advance PRN / sub-bin if finished with cur_k_
    if (cur_k_ > k_max_) {
        // On final sub-bin, this PRN's search is 100% complete -> report to M0!
        if (cur_sub_ == num_sub_ - 1) {
            const PRNResult& r = s_results[cur_prn_ - 1];
            if (r.status == static_cast<uint8_t>(DetStatus::Lock)) {
                detected_count_++;
            }
            const bool is_final = (cur_prn_ == prn_end_);
            const GPSScanResultMessage msg{r, scan_id_, is_final};
            shared_memory.application_queue.push(msg);
        }

        cur_prn_++;
        if (cur_prn_ <= prn_end_) {
            need_prn_fft_ = true;
        } else {
            cur_sub_++;
            if (cur_sub_ < num_sub_) {
                need_sub_fft_ = true;
            } else {
                state_ = State::BuildProfile;
                return;
            }
        }
    }

    // 5. Report smooth progress updates
    const uint8_t pct = static_cast<uint8_t>((done_steps_ * 100U) / total_steps_);
    if (pct != last_reported_pct_ && (pct >= last_reported_pct_ + 2 || last_reported_pct_ == 255)) {
        last_reported_pct_ = pct;
        send_stats(ScanState::Scanning, pct);
    }
}

void GPSScanProcessor::execute(const buffer_c8_t& buffer) {
    dma_buffers_++;

    if (config_pending_) {
        apply_pending_config();
    }

    switch (state_) {
        case State::Idle: {
            idle_acc_.measure_block(reinterpret_cast<const int8_t*>(buffer.p), buffer.count);
            idle_buf_count_++;
            if (idle_buf_count_ >= 250) {  // ~250 ms at 2.048 Msps
                send_stats(ScanState::Idle, 0);
                idle_acc_.reset();
                idle_buf_count_ = 0;
            }
            break;
        }

        case State::Calibrating: {
            if (config_.mode == Mode::SelfTest) {
                run_selftest_capture();
            } else {
                cal_acc_.reset();
                cal_acc_.measure_block(reinterpret_cast<const int8_t*>(buffer.p), buffer.count);
                cal_ = cal_acc_.derive_cal();
                dma_drops_start_ = shared_memory.m4_buffer_missed;
                cap_acc_.reset();
                captured_ms_ = 0;
                state_ = State::Capturing;
            }
            break;
        }

        case State::Capturing: {
            if (captured_ms_ < target_ms_ && buffer.count == SAMPLES_PER_MS) {
                cap_acc_.pack_ms_2bit(reinterpret_cast<const int8_t*>(buffer.p), cal_, s_snapshot[captured_ms_]);
                captured_ms_++;
            }
            if (captured_ms_ >= target_ms_) {
                const uint32_t drops = shared_memory.m4_buffer_missed - dma_drops_start_;
                snapshot_stats_ = cap_acc_.to_signal_stats(
                    static_cast<uint16_t>(drops > 65535U ? 65535U : drops),
                    static_cast<uint16_t>(dma_buffers_ & 0xFFFFU),
                    target_ms_,
                    0);
                has_valid_snapshot_ = true;
                scan_start_time_ = chTimeNow();
                state_ = State::Scanning;
                send_stats(ScanState::Scanning, 0);
            }
            break;
        }

        case State::Scanning: {
            step_scan();
            break;
        }

        case State::BuildProfile: {
            uint8_t target_prn = config_.profile_prn;
            if (target_prn < prn_start_ || target_prn > prn_end_ || s_results[target_prn - 1].prn != target_prn) {
                // Pick scanned PRN with highest Z-score
                uint16_t best_z = 0;
                target_prn = prn_start_;
                for (uint8_t p = prn_start_; p <= prn_end_; p++) {
                    if (s_results[p - 1].z_score_x10 >= best_z) {
                        best_z = s_results[p - 1].z_score_x10;
                        target_prn = p;
                    }
                }
            }
            build_and_send_profile(target_prn);
            send_stats(ScanState::Done, 100);

            if (config_.mode == Mode::ScanAuto) {
                start_new_scan();
            } else {
                config_.mode = Mode::Idle;
                state_ = State::Idle;
                idle_acc_.reset();
                idle_buf_count_ = 0;
            }
            break;
        }
    }
}

void GPSScanProcessor::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::GPSScanConfig: {
            const auto& msg = *reinterpret_cast<const GPSScanConfigMessage*>(message);
            pending_config_ = msg.config;
            config_pending_ = true;
            break;
        }
        default:
            break;
    }
}

#ifndef _WIN32
int main() {
    EventDispatcher event_dispatcher{std::make_unique<GPSScanProcessor>()};
    event_dispatcher.run();
    return 0;
}
#endif
