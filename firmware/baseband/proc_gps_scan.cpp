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

#include "proc_gps_scan.hpp"
#include "portapack_shared_memory.hpp"
#include "event_m4.hpp"
#include "core_cm4.h"

#include <cstring>

using namespace gps_scan;

namespace {

// Static M4 SRAM buffers so linker enforces exact RAM fit.
static uint8_t s_snapshot[MAX_SNAPSHOT_MS][SAMPLES_PER_MS / 2];  // 16 KiB (16 ms @ 2-bit I/Q)

// Acquisition FFT buffers (36 KiB), Stage 2 Tracker (~7.2 KiB), and Stage 3 5-ch Nav Bank + Sim (~22.4 KiB)
// never run simultaneously! Union lives in .bss => 0 flash init data and minimal M4 RAM!
union WorkUnion {
    struct {
        c8_t cached_fft[4][SAMPLES_PER_MS];  // 16 KiB (4 coherent blocks)
        c8_t fft_code[SAMPLES_PER_MS];       //  4 KiB
        c16_t fft_work[SAMPLES_PER_MS];      //  8 KiB
        uint32_t accum[SAMPLES_PER_MS];      //  8 KiB
    } acq;
    struct {
        GPSTracker tracker;                  // ~1.7 KiB
        GPSTrackSim sim;                     // ~1.4 KiB
        int8_t sim_iq[SAMPLES_PER_MS * 2];   //  4 KiB
    } trk;
    struct {
        GPSNavBank bank;                     // ~17.3 KiB
        GPSNavSim sim;                       //  ~5.1 KiB
    } nav;
    WorkUnion() {}
};
static WorkUnion s_work;
static auto& s_cached_fft = s_work.acq.cached_fft;
static auto& s_fft_code = s_work.acq.fft_code;
static auto& s_fft_work = s_work.acq.fft_work;
static auto& s_accum = s_work.acq.accum;
static auto& s_tracker = s_work.trk.tracker;
static auto& s_track_sim = s_work.trk.sim;

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

    if (config_.mode == Mode::Track || config_.mode == Mode::TrackSelfTest) {
        start_tracking();
        return;
    }

    if (config_.mode == Mode::NavFix || config_.mode == Mode::NavFixSelfTest) {
        start_nav_fix();
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
    k_min_ = static_cast<int16_t>(-k_max_);

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
    cur_k_ = k_min_;
    need_sub_fft_ = true;
    need_prn_fft_ = true;
    last_reported_pct_ = 255;

    const uint32_t prn_cnt = static_cast<uint32_t>(prn_end_ - prn_start_ + 1);
    const uint32_t k_cnt = static_cast<uint32_t>(k_max_ - k_min_ + 1);
    total_steps_ = static_cast<uint32_t>(num_sub_) * prn_cnt * k_cnt;
    if (total_steps_ == 0) total_steps_ = 1;
    done_steps_ = 0;

    has_valid_snapshot_ = false;
    state_ = State::Calibrating;
    send_stats(ScanState::Capturing, 0);
}

void GPSScanProcessor::start_tracking() {
    track_prn_ = (config_.prn_filter >= 1 && config_.prn_filter <= NUM_PRNS) ? config_.prn_filter : 1;
    track_report_cnt_ = 0;
    track_adc_cnt_ = 0;
    idle_acc_.reset();

    if (config_.mode == Mode::TrackSelfTest) {
        // Synthetic real-time LNAV tracking self-test:
        // True PRN 1 at +1370 Hz, init chip 412.3; start tracker off by -120 Hz & -0.25 chip to exercise FLL+DLL!
        track_prn_ = 1;
        s_track_sim.init(1, 1370.0f, 412.3f);
        s_tracker.init(1, 1250.0f, 412.05f, true);
        track_drops_start_ = shared_memory.m4_buffer_missed;
        state_ = State::Tracking;
        send_track_status(TrackState::PullIn);
        return;
    }

    // Real RF tracking: fast single-PRN 2msx4 (8 ms) re-acquisition around track_dopp_hint_hz
    coh_ms_ = 2;
    num_blocks_ = 4;
    target_ms_ = 8;
    num_sub_ = 4;
    sub_step_hz_ = 250;

    int16_t k_center = static_cast<int16_t>((config_.track_dopp_hint_hz >= 0)
                                                ? ((config_.track_dopp_hint_hz + 500) / 1000)
                                                : ((config_.track_dopp_hint_hz - 500) / 1000));
    k_min_ = static_cast<int16_t>(k_center - 1);
    k_max_ = static_cast<int16_t>(k_center + 1);
    if (k_min_ < -40) k_min_ = -40;
    if (k_max_ > 40) k_max_ = 40;

    track_wide_retry_ = false;
    track_reacq_best_ = {};
    track_reacq_pom_ = 0;
    cur_sub_ = 0;
    cur_k_ = k_min_;
    need_sub_fft_ = true;

    // Precompute PRN code FFT once for this single tracked PRN
    generate_ca_2048(track_prn_, s_fft_work);
    fft_2048_q15(s_fft_work, false);
    compress_fft_c8(s_fft_work, s_fft_code, true);

    state_ = State::TrackReacqCal;
    send_track_status(TrackState::Reacq);
}

void GPSScanProcessor::start_nav_fix() {
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    nav_report_cnt_ = 0;
    nav_cpu_us_pack_ = 0;
    nav_cpu_us_track_ = 0;
    idle_acc_.reset();

    if (config_.mode == Mode::NavFixSelfTest) {
        nav_count_ = MAX_NAV_CH;
        s_work.nav.bank.init_tables();
        s_work.nav.sim.init();
        // Realistic initial acquisition offsets (±70..110 Hz, ±0.18..0.28 chips) to test full pull-in -> PVT!
        static constexpr float kInitDoppErr[MAX_NAV_CH] = {-85.0f, +95.0f, -70.0f, +80.0f, -90.0f};
        static constexpr float kInitChipErr[MAX_NAV_CH] = {-0.24f, +0.22f, -0.18f, +0.26f, -0.20f};
        for (size_t i = 0; i < MAX_NAV_CH; i++) {
            const auto& sp = kSimNavConstellation[i];
            s_work.nav.bank.init_channel(i, sp.prn, sp.fd0_hz + kInitDoppErr[i], sp.init_chip + kInitChipErr[i]);
        }
        nav_drops_start_ = shared_memory.m4_buffer_missed;
        nav_last_missed_ = nav_drops_start_;
        state_ = State::NavTracking;
        send_nav_obs(2);
        return;
    }

    // Real RF multi-channel fix: build target list (up to MAX_NAV_CH)
    nav_count_ = config_.nav_count;
    if (nav_count_ > MAX_NAV_CH) nav_count_ = MAX_NAV_CH;
    if (nav_count_ == 0) {
        state_ = State::Idle;
        send_nav_obs(0);
        return;
    }

    for (uint8_t i = 0; i < nav_count_; i++) {
        nav_ho_[i] = {};
        nav_ho_[i].prn = config_.nav_targets[i].prn;
        nav_ho_[i].dopp_hz = config_.nav_targets[i].dopp_hint_hz;
    }

    // Shared 2ms x 4 (8 ms) snapshot re-acquisition across all nav_count_ PRNs
    coh_ms_ = 2;
    num_blocks_ = 4;
    target_ms_ = 8;
    num_sub_ = 4;
    sub_step_hz_ = 250;
    cur_sub_ = 0;
    nav_cur_idx_ = 0;
    need_sub_fft_ = true;
    need_prn_fft_ = true;

    state_ = State::NavReacqCal;
    send_nav_obs(1);
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

    SignalStats st = (scan_state == ScanState::Idle || state_ == State::Tracking || state_ == State::NavTracking)
                         ? idle_acc_.to_signal_stats(0, static_cast<uint16_t>(dma_buffers_ & 0xFFFFU), 0,
                                                     (config_.mode == Mode::TrackSelfTest || config_.mode == Mode::NavFixSelfTest) ? 1 : 0)
                         : snapshot_stats_;
    st.dma_buffers = static_cast<uint16_t>(dma_buffers_ & 0xFFFFU);
    if (state_ == State::Tracking) {
        st.dma_drops = static_cast<uint16_t>(shared_memory.m4_buffer_missed - track_drops_start_);
    } else if (state_ == State::NavTracking) {
        st.dma_drops = static_cast<uint16_t>(shared_memory.m4_buffer_missed - nav_drops_start_);
    }

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

void GPSScanProcessor::send_track_status(TrackState override_state) {
    TrackStatus ts{};
    if (override_state == TrackState::Reacq) {
        ts.prn = track_prn_;
        ts.state = static_cast<uint8_t>(TrackState::Reacq);
        ts.doppler_hz_x10 = static_cast<int32_t>(config_.track_dopp_hint_hz) * 10;
    } else {
        s_tracker.fill_status(ts, static_cast<uint16_t>(shared_memory.m4_buffer_missed - track_drops_start_));
        if (override_state != TrackState::Off) {
            ts.state = static_cast<uint8_t>(override_state);
        }
    }
    const GPSScanTrackMessage msg{ts};
    shared_memory.application_queue.push(msg);
}

void GPSScanProcessor::send_nav_obs(uint8_t phase) {
    NavObsPacket pkt{};
    const uint8_t is_st = (config_.mode == Mode::NavFixSelfTest) ? 1 : 0;
    if (phase == 2) {
        const uint16_t td = static_cast<uint16_t>(shared_memory.m4_buffer_missed - nav_drops_start_);
        s_work.nav.bank.fill_obs_packet(pkt, 2, nav_count_, is_st, td, nav_cpu_us_pack_, nav_cpu_us_track_);
    } else {
        pkt.num_ch = nav_count_;
        pkt.phase = phase;
        pkt.reacq_idx = nav_cur_idx_;
        pkt.is_selftest = is_st;
        for (uint8_t i = 0; i < nav_count_ && i < MAX_NAV_CH; i++) {
            pkt.ch[i].prn = nav_ho_[i].prn;
            pkt.ch[i].state = static_cast<uint8_t>(phase == 1 ? TrackState::Reacq : TrackState::Off);
            pkt.ch[i].doppler_hz_x10 = static_cast<int32_t>(nav_ho_[i].dopp_hz) * 10;
        }
    }
    const GPSScanNavObsMessage msg{pkt};
    shared_memory.application_queue.push(msg);
}

void GPSScanProcessor::build_and_send_profile(uint8_t prn) {
    if (prn < 1 || prn > NUM_PRNS) return;
    const PRNResult& res = s_results[prn - 1];
    if (res.prn != prn) return;

    const uint8_t sub = s_best_sub[prn - 1];
    const int16_t k = s_best_k[prn - 1];

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
    if (need_sub_fft_) {
        for (uint8_t b = 0; b < num_blocks_; b++) {
            fold_coherent_block(s_snapshot, b * coh_ms_, coh_ms_, cur_sub_, s_fft_work);
            fft_2048_q15(s_fft_work, false);
            compress_fft_c8(s_fft_work, s_cached_fft[b], false);
        }
        need_sub_fft_ = false;
        cur_prn_ = prn_start_;
        need_prn_fft_ = true;
        return;
    }

    if (need_prn_fft_) {
        generate_ca_2048(cur_prn_, s_fft_work);
        fft_2048_q15(s_fft_work, false);
        compress_fft_c8(s_fft_work, s_fft_code, true);
        need_prn_fft_ = false;
        cur_k_ = k_min_;
    }

    const uint8_t k_budget = (num_blocks_ <= 1) ? 8 : 3;
    for (uint8_t step = 0; step < k_budget && cur_k_ <= k_max_; step++, cur_k_++) {
        const int16_t dopp_hz = static_cast<int16_t>(cur_k_ * 1000 + cur_sub_ * sub_step_hz_);
        done_steps_++;

        if (dopp_hz >= k_min_ * 1000 && dopp_hz <= k_max_ * 1000) {
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

    if (cur_k_ > k_max_) {
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

    const uint8_t pct = static_cast<uint8_t>((done_steps_ * 100U) / total_steps_);
    if (pct != last_reported_pct_ && (pct >= last_reported_pct_ + 2 || last_reported_pct_ == 255)) {
        last_reported_pct_ = pct;
        send_stats(ScanState::Scanning, pct);
    }
}

void GPSScanProcessor::step_track_reacq() {
    if (need_sub_fft_) {
        for (uint8_t b = 0; b < num_blocks_; b++) {
            fold_coherent_block(s_snapshot, b * coh_ms_, coh_ms_, cur_sub_, s_fft_work);
            fft_2048_q15(s_fft_work, false);
            compress_fft_c8(s_fft_work, s_cached_fft[b], false);
        }
        need_sub_fft_ = false;
        cur_k_ = k_min_;
        return;
    }

    const uint8_t k_budget = 3;
    for (uint8_t step = 0; step < k_budget && cur_k_ <= k_max_; step++, cur_k_++) {
        const int16_t dopp_hz = static_cast<int16_t>(cur_k_ * 1000 + cur_sub_ * sub_step_hz_);
        correlate_doppler_bin(s_cached_fft, num_blocks_, s_fft_code, cur_k_, s_fft_work, s_accum);

        const uint32_t pom = quick_peak_over_mean_q8(s_accum);
        if (pom > track_reacq_pom_) {
            track_reacq_pom_ = pom;
            analyze_correlation(s_accum, track_prn_, dopp_hz, coh_ms_, num_blocks_, track_reacq_best_);
        }
    }

    if (cur_k_ > k_max_) {
        cur_sub_++;
        if (cur_sub_ < num_sub_) {
            need_sub_fft_ = true;
        } else {
            // Re-acquisition finished!
            // If narrow window missed the satellite, widen once to ±doppler_khz
            if (track_reacq_best_.status == static_cast<uint8_t>(DetStatus::Noise) && !track_wide_retry_) {
                track_wide_retry_ = true;
                int16_t span = config_.doppler_khz;
                if (span < 5) span = 5;
                if (span > 35) span = 35;
                k_min_ = static_cast<int16_t>(-span);
                k_max_ = span;
                cur_sub_ = 0;
                cur_k_ = k_min_;
                need_sub_fft_ = true;
                return;
            }
            state_ = State::TrackHandover;
        }
    }
}

void GPSScanProcessor::step_nav_reacq() {
    if (need_sub_fft_) {
        for (uint8_t b = 0; b < num_blocks_; b++) {
            fold_coherent_block(s_snapshot, b * coh_ms_, coh_ms_, cur_sub_, s_fft_work);
            fft_2048_q15(s_fft_work, false);
            compress_fft_c8(s_fft_work, s_cached_fft[b], false);
        }
        need_sub_fft_ = false;
        nav_cur_idx_ = 0;
        need_prn_fft_ = true;
        return;
    }

    if (need_prn_fft_) {
        const uint8_t prn = config_.nav_targets[nav_cur_idx_].prn;
        const int16_t hint = config_.nav_targets[nav_cur_idx_].dopp_hint_hz;
        generate_ca_2048(prn, s_fft_work);
        fft_2048_q15(s_fft_work, false);
        compress_fft_c8(s_fft_work, s_fft_code, true);
        const int16_t k_center = static_cast<int16_t>((hint >= 0) ? ((hint + 500) / 1000) : ((hint - 500) / 1000));
        k_min_ = static_cast<int16_t>(k_center - 1);
        k_max_ = static_cast<int16_t>(k_center + 1);
        if (k_min_ < -40) k_min_ = -40;
        if (k_max_ > 40) k_max_ = 40;
        cur_k_ = k_min_;
        need_prn_fft_ = false;
    }

    const uint8_t prn = config_.nav_targets[nav_cur_idx_].prn;
    const uint8_t k_budget = 3;
    for (uint8_t step = 0; step < k_budget && cur_k_ <= k_max_; step++, cur_k_++) {
        const int16_t dopp_hz = static_cast<int16_t>(cur_k_ * 1000 + cur_sub_ * sub_step_hz_);
        correlate_doppler_bin(s_cached_fft, num_blocks_, s_fft_code, cur_k_, s_fft_work, s_accum);

        const uint32_t pom = quick_peak_over_mean_q8(s_accum);
        if (pom > nav_ho_[nav_cur_idx_].pom) {
            PRNResult tmp{};
            analyze_correlation(s_accum, prn, dopp_hz, coh_ms_, num_blocks_, tmp);
            nav_ho_[nav_cur_idx_].pom = pom;
            nav_ho_[nav_cur_idx_].prn = prn;
            nav_ho_[nav_cur_idx_].dopp_hz = tmp.doppler_hz;
            nav_ho_[nav_cur_idx_].code_phase = tmp.code_phase;
            nav_ho_[nav_cur_idx_].status = tmp.status;
        }
    }

    if (cur_k_ > k_max_) {
        nav_cur_idx_++;
        if (nav_cur_idx_ < nav_count_) {
            need_prn_fft_ = true;
        } else {
            cur_sub_++;
            if (cur_sub_ < num_sub_) {
                need_sub_fft_ = true;
            } else {
                state_ = State::NavHandover;
            }
        }
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

        case State::TrackReacqCal: {
            cal_acc_.reset();
            cal_acc_.measure_block(reinterpret_cast<const int8_t*>(buffer.p), buffer.count);
            cal_ = cal_acc_.derive_cal();
            cap_acc_.reset();
            captured_ms_ = 0;
            state_ = State::TrackReacqCap;
            break;
        }

        case State::TrackReacqCap: {
            if (captured_ms_ == 0) {
                // Exact monotonic DMA transfer index at start of s_snapshot[0]!
                track_snap_start_ms_ = dma_buffers_ + shared_memory.m4_buffer_missed;
            }
            if (captured_ms_ < target_ms_ && buffer.count == SAMPLES_PER_MS) {
                cap_acc_.pack_ms_2bit(reinterpret_cast<const int8_t*>(buffer.p), cal_, s_snapshot[captured_ms_]);
                captured_ms_++;
            }
            if (captured_ms_ >= target_ms_) {
                state_ = State::TrackReacqScan;
            }
            break;
        }

        case State::TrackReacqScan: {
            step_track_reacq();
            break;
        }

        case State::TrackHandover: {
            // Compute exact elapsed 1.000ms DMA buffers since s_snapshot[0], then process THIS buffer immediately!
            const uint32_t now_total_ms = dma_buffers_ + shared_memory.m4_buffer_missed;
            const uint32_t elapsed_ms = now_total_ms - track_snap_start_ms_;
            const float snap0_chip = static_cast<float>((2048U - track_reacq_best_.code_phase) & 2047U) * (1023.0f / 2048.0f);
            const float init_chip = snap0_chip + static_cast<float>(elapsed_ms) * static_cast<float>(track_reacq_best_.doppler_hz) * (1.0f / 1540000.0f);

            s_tracker.init(track_prn_, static_cast<float>(track_reacq_best_.doppler_hz), init_chip, false);
            track_drops_start_ = shared_memory.m4_buffer_missed;
            track_last_missed_ = track_drops_start_;
            track_report_cnt_ = 0;
            track_adc_cnt_ = 0;
            idle_acc_.reset();
            state_ = State::Tracking;

            // Process this very first buffer right away so zero ms are skipped!
            s_tracker.process_samples(reinterpret_cast<const int8_t*>(buffer.p), buffer.count);
            send_track_status(TrackState::PullIn);
            break;
        }

        case State::Tracking: {
            const int8_t* iq_ptr = reinterpret_cast<const int8_t*>(buffer.p);
            const uint16_t missed = shared_memory.m4_buffer_missed;
            const uint16_t gap = static_cast<uint16_t>(missed - track_last_missed_);
            track_last_missed_ = missed;
            if (config_.mode == Mode::TrackSelfTest) {
                s_track_sim.generate_ms(s_work.trk.sim_iq);
                iq_ptr = s_work.trk.sim_iq;
            } else if (gap != 0U) {
                // Real signal time elapsed in the missed buffers: coast NCOs / bit timing across it.
                s_tracker.skip_ms(gap);
            }

            s_tracker.process_samples(iq_ptr, buffer.count);

            track_report_cnt_++;
            if (track_report_cnt_ == 50) {
                // Lightweight 256-sample ADC check staggered away from track telemetry (~20 us)
                idle_acc_.measure_block(iq_ptr, 256);
                track_adc_cnt_++;
                if (track_adc_cnt_ >= 5) {  // 2 Hz ADC health strip update
                    track_adc_cnt_ = 0;
                    send_stats(ScanState::Scanning, 100);
                    idle_acc_.reset();
                }
            } else if (track_report_cnt_ >= 100) {  // 10 Hz live tracking telemetry!
                track_report_cnt_ = 0;
                send_track_status(TrackState::Off);
            }
            break;
        }

        case State::NavReacqCal: {
            cal_acc_.reset();
            cal_acc_.measure_block(reinterpret_cast<const int8_t*>(buffer.p), buffer.count);
            cal_ = cal_acc_.derive_cal();
            cap_acc_.reset();
            captured_ms_ = 0;
            state_ = State::NavReacqCap;
            break;
        }

        case State::NavReacqCap: {
            if (captured_ms_ == 0) {
                nav_snap_start_ms_ = dma_buffers_ + shared_memory.m4_buffer_missed;
            }
            if (captured_ms_ < target_ms_ && buffer.count == SAMPLES_PER_MS) {
                cap_acc_.pack_ms_2bit(reinterpret_cast<const int8_t*>(buffer.p), cal_, s_snapshot[captured_ms_]);
                captured_ms_++;
            }
            if (captured_ms_ >= target_ms_) {
                state_ = State::NavReacqScan;
            }
            break;
        }

        case State::NavReacqScan: {
            step_nav_reacq();
            break;
        }

        case State::NavHandover: {
            const uint32_t now_total_ms = dma_buffers_ + shared_memory.m4_buffer_missed;
            const uint32_t elapsed_ms = now_total_ms - nav_snap_start_ms_;

            // Switch WorkUnion from FFT buffers -> 5-channel GPSNavBank!
            s_work.nav.bank.init_tables();
            for (uint8_t i = 0; i < nav_count_; i++) {
                const float snap0_chip = static_cast<float>((2048U - nav_ho_[i].code_phase) & 2047U) * (1023.0f / 2048.0f);
                const float init_chip = snap0_chip +
                                        static_cast<float>(elapsed_ms) * static_cast<float>(nav_ho_[i].dopp_hz) * (1.0f / 1540000.0f) +
                                        0.25f;
                s_work.nav.bank.init_channel(i, nav_ho_[i].prn, static_cast<float>(nav_ho_[i].dopp_hz), init_chip);
            }

            nav_drops_start_ = shared_memory.m4_buffer_missed;
            nav_last_missed_ = nav_drops_start_;
            nav_report_cnt_ = 0;
            idle_acc_.reset();
            state_ = State::NavTracking;

            // Process this very first buffer immediately so zero ms are skipped!
            NavSubframe sfs[MAX_NAV_CH];
            s_work.nav.bank.pack_raw_2048(reinterpret_cast<const int8_t*>(buffer.p));
            const size_t nsf = s_work.nav.bank.process_bitplanes(sfs);
            for (size_t k = 0; k < nsf; k++) {
                const GPSScanSubframeMessage smsg{sfs[k]};
                shared_memory.application_queue.push(smsg);
            }
            send_nav_obs(2);
            break;
        }

        case State::NavTracking: {
            NavSubframe sfs[MAX_NAV_CH];
            const bool is_st = (config_.mode == Mode::NavFixSelfTest);

            // Buffers the M4 missed since the last call are real signal time that elapsed. Coast every
            // channel's NCOs / epoch / bit counters across them so code, carrier & 20 ms bit timing stay
            // aligned. (Self-test synthesizes per call, so its signal has no gap and must not be coasted.)
            const uint16_t missed = shared_memory.m4_buffer_missed;
            const uint16_t gap = static_cast<uint16_t>(missed - nav_last_missed_);
            nav_last_missed_ = missed;
            if (gap != 0U && !is_st) {
                const size_t nskip = s_work.nav.bank.skip_ms(gap, sfs);
                for (size_t k = 0; k < nskip; k++) {
                    const GPSScanSubframeMessage smsg{sfs[k]};
                    shared_memory.application_queue.push(smsg);
                }
            }

            const uint32_t cyc0 = DWT->CYCCNT;
            if (is_st) {
                s_work.nav.sim.generate_bitplanes_ms(s_work.nav.bank.bitplanes(), s_work.nav.bank.cs_tbl());
            } else {
                s_work.nav.bank.pack_raw_2048(reinterpret_cast<const int8_t*>(buffer.p));
            }
            const uint32_t cyc1 = DWT->CYCCNT;
            const size_t nsf = s_work.nav.bank.process_bitplanes(sfs);
            const uint32_t cyc2 = DWT->CYCCNT;

            // Report the worst case of each 500 ms window (a single >1 ms spike is what costs a buffer).
            const uint16_t us_pack = static_cast<uint16_t>((cyc1 - cyc0) / 204U);
            const uint16_t us_track = static_cast<uint16_t>((cyc2 - cyc1) / 204U);
            if (us_pack > nav_cpu_us_pack_) nav_cpu_us_pack_ = us_pack;
            if (us_track > nav_cpu_us_track_) nav_cpu_us_track_ = us_track;

            for (size_t k = 0; k < nsf; k++) {
                const GPSScanSubframeMessage smsg{sfs[k]};
                shared_memory.application_queue.push(smsg);
            }

            nav_report_cnt_++;
            if (nav_report_cnt_ == 250) {
                // Lightweight 256-sample ADC health check staggered 250 ms away from NavObs
                if (!is_st) {
                    idle_acc_.measure_block(reinterpret_cast<const int8_t*>(buffer.p), 256);
                }
                send_stats(ScanState::Scanning, 100);
                idle_acc_.reset();
            } else if (nav_report_cnt_ >= 500) {  // 2 Hz multi-channel observation telemetry
                nav_report_cnt_ = 0;
                send_nav_obs(2);
                nav_cpu_us_pack_ = 0;
                nav_cpu_us_track_ = 0;
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
