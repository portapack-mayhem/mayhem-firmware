// Author: berkeozkir (Berke Özkır)

/*
 * FPV RX — how frequency search and lock work
 *
 * 1. Power metric: We use channelized power (baseband IQ magnitude² in the
 *    capture bandwidth), not raw RF RSSI. Stats come from ChannelStatsCollector
 *    over filtered IQ, so we see power in the tuned channel, not wideband.
 *
 * 2. Scanning: We step through FPV bands/channels (A/B/E/F/R, 8 ch each). When
 *    power on the current channel exceeds the detect threshold, we enter
 *    "Candidate" and run verification — we do not lock on a single peak.
 *
 * 3. Verification (making sure the drone is on that freq):
 *    - Multiple samples: verify_hits / verify_misses over several updates.
 *    - Confidence score: combines average above threshold, peak margin,
 *      hit count, and crucially neighbor_margin = center power − max(adjacent ch).
 *    - Neighbor check: we only lock if the center channel is at least
 *      MIN_NEIGHBOR_MARGIN_FOR_LOCK_DB above both adjacent channels. That
 *      avoids locking on wide/noise that looks strong on many channels.
 *      During Candidate we re-sample left/right adjacent channels so the
 *      margin is based on current measurements, not stale scan data.
 *    - Lock requires: enough hits, confidence ≥ LOCK_CONFIDENCE_MIN, and
 *      center stronger than neighbors (strong lock path has same requirement).
 *
 * 4. Lock hold: Once locked, we only unlock when power stays below
 *    unlock_threshold_db() long enough to drain lock_hold_ (down from
 *    LOCK_HOLD_MAX), so brief fades don't drop lock.
 */

#include "ui_fpv_detect.hpp"

#include <cstdio>

#include "baseband_api.hpp"
#include "oversample.hpp"

using namespace portapack;

namespace ui::external_app::fpv_detect {

static constexpr uint8_t SCAN_DWELL_FRAMES = 1;
static constexpr uint8_t VERIFY_SAMPLE_TARGET = 5;
static constexpr uint8_t VERIFY_MIN_HITS = 4;
static constexpr uint8_t STRONG_LOCK_PEAK_MARGIN_DB = 8;
static constexpr uint8_t LOCK_HOLD_MAX = 12;
static constexpr uint8_t LOCK_CONFIDENCE_MIN = 72;
/** Center channel must be at least this many dB above adjacent channels to lock.
 *  Ensures we lock on a real FPV carrier (narrow peak) rather than wide/noise. */
static constexpr int32_t MIN_NEIGHBOR_MARGIN_FOR_LOCK_DB = 3;
static constexpr uint8_t BEEP_GUARD_FRAMES = 8;

int32_t FpvDetectView::clamp_value(int32_t value, int32_t low, int32_t high) {
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

int32_t FpvDetectView::map_value(int32_t value, int32_t from_low, int32_t from_high, int32_t to_low, int32_t to_high) {
    const auto clamped = clamp_value(value, from_low, from_high);
    return to_low + (clamped - from_low) * (to_high - to_low) / (from_high - from_low);
}

void FpvDetectView::focus() {
    field_lna.focus();
}

FpvDetectView::~FpvDetectView() {
    shared_memory.request_m4_performance_counter = 0;
    receiver_model.disable();
    audio::output::stop();
    baseband::shutdown();
}

uint8_t FpvDetectView::channel_index(uint8_t band, uint8_t ch) const {
    return static_cast<uint8_t>(band * FPV_CHANNELS_PER_BAND + ch);
}

uint8_t FpvDetectView::current_channel_index() const {
    return channel_index(scan_band, scan_ch);
}

int16_t FpvDetectView::detect_threshold_db() const {
    return static_cast<int16_t>(detect_threshold_db_);
}

int16_t FpvDetectView::unlock_threshold_db() const {
    return static_cast<int16_t>(detect_threshold_db() - 6);
}

int16_t FpvDetectView::candidate_average_db() const {
    if (!verify_samples_) {
        return -120;
    }
    return static_cast<int16_t>(verify_sum_db_ / verify_samples_);
}

int16_t FpvDetectView::neighbor_best_db() const {
    int16_t best = -120;

    if (candidate_ch_ > 0) {
        const auto& left = channel_memory_[channel_index(candidate_band_, static_cast<uint8_t>(candidate_ch_ - 1))];
        if (left.last_db > best) {
            best = left.last_db;
        }
    }

    if ((candidate_ch_ + 1) < FPV_CHANNELS_PER_BAND) {
        const auto& right = channel_memory_[channel_index(candidate_band_, static_cast<uint8_t>(candidate_ch_ + 1))];
        if (right.last_db > best) {
            best = right.last_db;
        }
    }

    return best;
}

void FpvDetectView::request_event_beep(uint32_t hz, uint32_t duration_ms) {
    if ((frame_counter_ - last_beep_frame_) < BEEP_GUARD_FRAMES) {
        return;
    }
    last_beep_frame_ = frame_counter_;
    baseband::request_audio_beep(hz, 24000, duration_ms);
}

void FpvDetectView::retune_to(uint8_t band, uint8_t ch) {
    scan_band = band;
    scan_ch = ch;
    freq_ = fpv_frequencies[band][ch];
    receiver_model.set_target_frequency(freq_);
    update_freq_display();
}

void FpvDetectView::step_scan() {
    if (band_mode < FPV_NUM_BANDS) {
        scan_band = band_mode;
        scan_ch = static_cast<uint8_t>((scan_ch + 1) % FPV_CHANNELS_PER_BAND);
    } else {
        scan_ch = static_cast<uint8_t>(scan_ch + 1);
        if (scan_ch >= FPV_CHANNELS_PER_BAND) {
            scan_ch = 0;
            scan_band = static_cast<uint8_t>(scan_band + 1);
            if (scan_band >= FPV_NUM_BANDS) {
                scan_band = 0;
            }
        }
    }

    retune_to(scan_band, scan_ch);
    scan_dwell_frames_ = SCAN_DWELL_FRAMES;
}

void FpvDetectView::reset_detector(bool retune_current) {
    detect_state_ = DetectState::Scanning;
    candidate_band_ = scan_band;
    candidate_ch_ = scan_ch;
    verify_samples_ = 0;
    verify_hits_ = 0;
    verify_misses_ = 0;
    verify_sum_db_ = 0;
    verify_peak_db_ = -120;
    candidate_confidence_ = 0;
    lock_hold_ = 0;
    neighbor_phase_ = 0;
    neighbors_fresh_ = false;
    scan_dwell_frames_ = 0;

    update_state_badge();
    update_confidence_text();
    update_status_text();
    update_detail_text();
    update_visual_state();

    if (retune_current) {
        retune_to(scan_band, scan_ch);
        scan_dwell_frames_ = SCAN_DWELL_FRAMES;
    }
}

bool FpvDetectView::is_possible_analog_carrier(const ChannelStatistics& statistics) const {
    return statistics.max_db >= detect_threshold_db();
}

uint8_t FpvDetectView::compute_candidate_confidence() const {
    if (!verify_samples_) {
        return 0;
    }

    const int32_t avg_db = candidate_average_db();
    const int32_t center_margin = avg_db - detect_threshold_db();
    const int32_t peak_margin = verify_peak_db_ - detect_threshold_db();
    const int32_t neighbor_margin = avg_db - neighbor_best_db();

    int32_t score = 18;
    score += center_margin * 7;
    score += peak_margin * 4;
    score += static_cast<int32_t>(verify_hits_) * 5;
    score -= static_cast<int32_t>(verify_misses_) * 10;

    if (neighbor_margin >= 6) {
        score += 12;
    } else if (neighbor_margin >= 3) {
        score += 6;
    } else if (neighbor_margin <= 0) {
        score -= 8;
    }

    const auto& mem = channel_memory_[channel_index(candidate_band_, candidate_ch_)];
    if (mem.hits >= 2) {
        score += 4;
    }

    return static_cast<uint8_t>(clamp_value(score, 0, 99));
}

void FpvDetectView::enter_candidate(const ChannelStatistics& statistics) {
    detect_state_ = DetectState::Candidate;
    candidate_band_ = scan_band;
    candidate_ch_ = scan_ch;
    verify_samples_ = 1;
    verify_hits_ = 1;
    verify_misses_ = 0;
    verify_sum_db_ = statistics.max_db;
    verify_peak_db_ = statistics.max_db;
    candidate_confidence_ = compute_candidate_confidence();
    neighbor_phase_ = 0;
    neighbors_fresh_ = false;

    retune_to(candidate_band_, candidate_ch_);

    update_state_badge();
    update_confidence_text();
    update_status_text();
    update_detail_text();
    update_visual_state();
    request_event_beep(1150, 60);
}

void FpvDetectView::evaluate_candidate_sample(const ChannelStatistics& statistics) {
    if (detect_state_ != DetectState::Candidate) {
        return;
    }

    ++verify_samples_;
    verify_sum_db_ += statistics.max_db;
    if (statistics.max_db > verify_peak_db_) {
        verify_peak_db_ = statistics.max_db;
    }

    if (statistics.max_db >= detect_threshold_db()) {
        ++verify_hits_;
    } else {
        ++verify_misses_;
    }

    candidate_confidence_ = compute_candidate_confidence();
    update_confidence_text();
    update_detail_text();

    /* Refresh adjacent channel power so neighbor_best_db() uses current
     * measurements instead of stale scan data. Do this before any lock decision. */
    if (verify_samples_ >= 2 && !neighbors_fresh_) {
        if (candidate_ch_ > 0) {
            neighbor_phase_ = 1;
            retune_to(candidate_band_, static_cast<uint8_t>(candidate_ch_ - 1));
        } else {
            neighbor_phase_ = 2;
            retune_to(candidate_band_, static_cast<uint8_t>(candidate_ch_ + 1));
        }
        return;
    }

    /* Lock decisions use neighbor margin; only run when we have fresh neighbor data. */
    if (!neighbors_fresh_) {
        return;
    }

    const bool strong_lock =
        verify_samples_ >= 3 &&
        verify_hits_ >= 3 &&
        verify_peak_db_ >= (detect_threshold_db() + STRONG_LOCK_PEAK_MARGIN_DB) &&
        candidate_confidence_ >= 80 &&
        (candidate_average_db() - neighbor_best_db()) >= MIN_NEIGHBOR_MARGIN_FOR_LOCK_DB;

    if (strong_lock) {
        enter_lock();
        return;
    }

    if (verify_samples_ < VERIFY_SAMPLE_TARGET) {
        return;
    }

    const bool center_stronger_than_neighbors =
        (candidate_average_db() - neighbor_best_db()) >= MIN_NEIGHBOR_MARGIN_FOR_LOCK_DB;
    if (verify_hits_ >= VERIFY_MIN_HITS && candidate_confidence_ >= LOCK_CONFIDENCE_MIN &&
        center_stronger_than_neighbors) {
        enter_lock();
        return;
    }

    detect_state_ = DetectState::Scanning;
    candidate_confidence_ = static_cast<uint8_t>(candidate_confidence_ / 2);
    verify_samples_ = 0;
    verify_hits_ = 0;
    verify_misses_ = 0;
    verify_sum_db_ = 0;
    verify_peak_db_ = -120;
    scan_dwell_frames_ = 0;

    update_state_badge();
    update_confidence_text();
    update_status_text();
    update_detail_text();
    update_visual_state();
}

void FpvDetectView::enter_lock() {
    detect_state_ = DetectState::Locked;
    lock_hold_ = LOCK_HOLD_MAX;
    candidate_confidence_ = static_cast<uint8_t>(clamp_value(candidate_confidence_, LOCK_CONFIDENCE_MIN, 99));

    retune_to(candidate_band_, candidate_ch_);

    update_state_badge();
    update_confidence_text();
    update_status_text();
    update_detail_text();
    update_visual_state();

    request_event_beep(1850, 180);
}

void FpvDetectView::update_lock(const ChannelStatistics& statistics) {
    if (detect_state_ != DetectState::Locked) {
        return;
    }

    if (statistics.max_db >= unlock_threshold_db()) {
        if (lock_hold_ < LOCK_HOLD_MAX) {
            ++lock_hold_;
        }
        if (candidate_confidence_ < 99) {
            ++candidate_confidence_;
        }
    } else {
        if (lock_hold_ > 0) {
            --lock_hold_;
        }
        if (candidate_confidence_ > 0) {
            --candidate_confidence_;
        }
    }

    update_confidence_text();
    update_detail_text();

    if (lock_hold_ > 0) {
        return;
    }

    detect_state_ = DetectState::Scanning;
    candidate_confidence_ = 0;
    verify_samples_ = 0;
    verify_hits_ = 0;
    verify_misses_ = 0;
    verify_sum_db_ = 0;
    verify_peak_db_ = -120;
    scan_dwell_frames_ = 0;

    update_state_badge();
    update_confidence_text();
    update_status_text();
    update_detail_text();
    update_visual_state();

    request_event_beep(650, 90);
}

void FpvDetectView::update_freq_display() {
    char buf[28];
    std::snprintf(buf, sizeof(buf), "%c%d  %ld MHz",
                  band_labels[scan_band],
                  static_cast<int>(scan_ch + 1),
                  static_cast<long>(freq_ / 1000000LL));
    text_freq.set(buf);
}

void FpvDetectView::update_state_badge() {
    switch (detect_state_) {
        case DetectState::Scanning:
            text_state.set("SCANNING");
            break;
        case DetectState::Candidate:
            text_state.set("VERIFY FPV");
            break;
        case DetectState::Locked:
            text_state.set("DRONE FOUND");
            break;
    }
}

void FpvDetectView::update_confidence_text() {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "Conf %u%%", static_cast<unsigned>(candidate_confidence_));
    text_confidence.set(buf);
}

void FpvDetectView::update_status_text() {
    char buf[40];

    switch (detect_state_) {
        case DetectState::Scanning:
            if (band_mode < FPV_NUM_BANDS) {
                std::snprintf(buf, sizeof(buf), "Scanning band %c for analog FPV", band_labels[band_mode]);
            } else {
                std::snprintf(buf, sizeof(buf), "Scanning all FPV bands");
            }
            break;

        case DetectState::Candidate:
            std::snprintf(buf, sizeof(buf), "VERIFYING %c%d  %ld MHz",
                          band_labels[candidate_band_],
                          static_cast<int>(candidate_ch_ + 1),
                          static_cast<long>(fpv_frequencies[candidate_band_][candidate_ch_] / 1000000LL));
            break;

        case DetectState::Locked:
            std::snprintf(buf, sizeof(buf), "!!! DRONE FOUND !!! %c%d  %ld",
                          band_labels[candidate_band_],
                          static_cast<int>(candidate_ch_ + 1),
                          static_cast<long>(fpv_frequencies[candidate_band_][candidate_ch_] / 1000000LL));
            break;
    }

    text_status.set(buf);
}

void FpvDetectView::update_detail_text() {
    char buf[40];

    switch (detect_state_) {
        case DetectState::Scanning:
            std::snprintf(buf, sizeof(buf), "Need >= %d dB, stable dwell", static_cast<int>(detect_threshold_db()));
            break;

        case DetectState::Candidate:
            std::snprintf(buf, sizeof(buf), "avg %d  peak %d  hits %u/%u",
                          static_cast<int>(candidate_average_db()),
                          static_cast<int>(verify_peak_db_),
                          static_cast<unsigned>(verify_hits_),
                          static_cast<unsigned>(verify_samples_));
            break;

        case DetectState::Locked:
            std::snprintf(buf, sizeof(buf), "LOCKED %c%d  conf %u%%  hold %u",
                          band_labels[candidate_band_],
                          static_cast<int>(candidate_ch_ + 1),
                          static_cast<unsigned>(candidate_confidence_),
                          static_cast<unsigned>(lock_hold_));
            break;
    }

    text_detail.set(buf);
}

void FpvDetectView::update_visual_state() {
    auto* theme = Theme::getInstance();

    /*
     * Conservative styling for cross-branch compatibility.
     * If your branch has explicit blue/red banner theme styles,
     * swap the styles in this function only.
     */
    auto normal_style = theme->bg_darkest;
    auto attention_style = theme->fg_red;

    switch (detect_state_) {
        case DetectState::Scanning:
            text_state.set_style(normal_style);
            text_status.set_style(normal_style);
            break;
        case DetectState::Candidate:
            text_state.set_style(normal_style);
            text_status.set_style(normal_style);
            break;
        case DetectState::Locked:
            text_state.set_style(attention_style);
            text_status.set_style(attention_style);
            break;
    }

    freq_stats_rssi.set_style(normal_style);
    freq_stats_db.set_style(normal_style);
    text_detail.set_style(normal_style);
    text_confidence.set_style(normal_style);
}

void FpvDetectView::update_stats_text(const ChannelStatistics& statistics) {
    rssi_graph.add_values(rssi.get_min(), rssi.get_avg(), rssi.get_max(), statistics.max_db);

    auto& mem = channel_memory_[current_channel_index()];
    mem.last_db = statistics.max_db;
    if (statistics.max_db > mem.peak_db) {
        mem.peak_db = statistics.max_db;
    }
    if (statistics.max_db >= detect_threshold_db()) {
        if (mem.hits < 255) {
            ++mem.hits;
        }
    }
    mem.confidence = candidate_confidence_;

    if (last_max_db_ != statistics.max_db) {
        last_max_db_ = statistics.max_db;
        char power_buf[20];
        std::snprintf(power_buf, sizeof(power_buf), "PWR %d dB", static_cast<int>(statistics.max_db));
        freq_stats_db.set(power_buf);
        rssi.set_db(statistics.max_db);
    }

    const uint8_t graph_min = rssi_graph.get_graph_min();
    const uint8_t graph_avg = rssi_graph.get_graph_avg();
    const uint8_t graph_max = rssi_graph.get_graph_max();

    if (last_min_rssi_ != graph_min || last_avg_rssi_ != graph_avg || last_max_rssi_ != graph_max) {
        last_min_rssi_ = graph_min;
        last_avg_rssi_ = graph_avg;
        last_max_rssi_ = graph_max;

        char rssi_buf[22];
        std::snprintf(rssi_buf, sizeof(rssi_buf), "RSSI %u/%u/%u",
                      static_cast<unsigned>(graph_min),
                      static_cast<unsigned>(graph_avg),
                      static_cast<unsigned>(graph_max));
        freq_stats_rssi.set(rssi_buf);
    }
}

void FpvDetectView::on_timer() {
    ++frame_counter_;

    if (detect_state_ != DetectState::Scanning) {
        return;
    }

    if (scan_dwell_frames_ > 0) {
        --scan_dwell_frames_;
        return;
    }

    step_scan();
}

FpvDetectView::FpvDetectView(NavigationView& nav)
    : nav_{nav} {
    add_children({
        &labels,
        &field_lna,
        &field_vga,
        &field_rf_amp,
        &field_volume,
        &field_band,
        &text_freq,
        &text_state,
        &text_confidence,
        &text_detect_label,
        &field_detect_threshold,
        &freq_stats_rssi,
        &freq_stats_db,
        &text_status,
        &text_detail,
        &rssi,
        &rssi_graph,
    });

    rssi.set_vertical_rssi(true);
    rssi.set_peak(true, 3000);
    rssi_graph.set_nb_columns(256);

    field_detect_threshold.set_value(detect_threshold_db_);
    field_detect_threshold.on_change = [this](int32_t v) {
        detect_threshold_db_ = v;
        update_detail_text();
    };

    field_band.on_change = [this](size_t, int32_t value) {
        band_mode = static_cast<uint8_t>(value);
        scan_band = (band_mode < FPV_NUM_BANDS) ? band_mode : 0;
        scan_ch = 0;
        reset_detector(true);
    };

    change_mode();
    field_band.set_selected_index(FPV_AUTO_SCAN_MODE);
    reset_detector(true);
}

void FpvDetectView::on_statistics_update(const ChannelStatistics& statistics) {
    const bool is_sampling_neighbor =
        (detect_state_ == DetectState::Candidate && (neighbor_phase_ == 1 || neighbor_phase_ == 2));
    if (!is_sampling_neighbor) {
        update_stats_text(statistics);
    }

    switch (detect_state_) {
        case DetectState::Scanning:
            if (is_possible_analog_carrier(statistics)) {
                enter_candidate(statistics);
            }
            break;

        case DetectState::Candidate:
            if (neighbor_phase_ == 1) {
                const auto idx = channel_index(candidate_band_, static_cast<uint8_t>(candidate_ch_ - 1));
                channel_memory_[idx].last_db = statistics.max_db;
                if (candidate_ch_ + 1 < FPV_CHANNELS_PER_BAND) {
                    neighbor_phase_ = 2;
                    retune_to(candidate_band_, static_cast<uint8_t>(candidate_ch_ + 1));
                } else {
                    neighbor_phase_ = 0;
                    neighbors_fresh_ = true;
                    retune_to(candidate_band_, candidate_ch_);
                }
                break;
            }
            if (neighbor_phase_ == 2) {
                const auto idx = channel_index(candidate_band_, static_cast<uint8_t>(candidate_ch_ + 1));
                channel_memory_[idx].last_db = statistics.max_db;
                neighbor_phase_ = 0;
                neighbors_fresh_ = true;
                retune_to(candidate_band_, candidate_ch_);
                break;
            }
            evaluate_candidate_sample(statistics);
            break;

        case DetectState::Locked:
            update_lock(statistics);
            break;
    }
}

size_t FpvDetectView::change_mode() {
    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();

    audio_sampling_rate = audio::Rate::Hz_24000;
    baseband::run_image(portapack::spi_flash::image_tag_capture);
    receiver_model.set_modulation(ReceiverModel::Mode::Capture);

    baseband::set_sample_rate(FPV_RX_BW, get_oversample_rate(FPV_RX_BW));
    auto actual_sampling_rate = get_actual_sample_rate(FPV_RX_BW);
    receiver_model.set_sampling_rate(actual_sampling_rate);
    receiver_model.set_baseband_bandwidth(filter_bandwidth_for_sampling_rate(actual_sampling_rate));

    audio::set_rate(audio_sampling_rate);
    audio::output::start();
    receiver_model.set_headphone_volume(receiver_model.headphone_volume());
    receiver_model.enable();

    return 0;
}

}  // namespace ui::external_app::fpv_detect
