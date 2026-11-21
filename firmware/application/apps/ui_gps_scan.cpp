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
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "ui_gps_scan.hpp"
#include "baseband_api.hpp"
#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"
#include "string_format.hpp"

#include <cstdio>
#include <cstring>

using namespace portapack;
using namespace gps_scan;

namespace {

static constexpr uint8_t kDoppKhzChoices[4] = {5, 10, 20, 35};
static constexpr const char* kDoppLabels[4] = {"+-5k", "+-10k", "+-20k", "+-35k"};

// Integer-only dBFS * 10 approximation for rms_x10 (full scale = 1280 => 0.0 dBFS).
int16_t rms_x10_to_dbfs_x10(uint16_t rms_x10) {
    if (rms_x10 <= 1) return -620;
    uint32_t v = rms_x10;
    int32_t msb = 0;
    while ((v >> (msb + 1)) != 0) msb++;
    const int32_t frac = (msb >= 4)
                             ? static_cast<int32_t>((v >> (msb - 4)) & 0x0FU)
                             : static_cast<int32_t>((v << (4 - msb)) & 0x0FU);
    const int32_t log2_diff_x16 = (msb * 16 + frac) - 165;
    return static_cast<int16_t>((log2_diff_x16 * 3853) / 1024);
}

const char* integ_label(Integration integ) {
    switch (integ) {
        case Integration::Ms1x1:
            return "1msx1";
        case Integration::Ms1x4:
            return "1msx4";
        case Integration::Ms2x4:
            return "2msx4";
        case Integration::Ms4x4:
            return "4msx4";
        default:
            return "2msx4";
    }
}

const char* status_tag(uint8_t st) {
    switch (static_cast<DetStatus>(st)) {
        case DetStatus::Lock:
            return "LOCK";
        case DetStatus::Weak:
            return "WEAK";
        default:
            return "NOIS";
    }
}

}  // namespace

namespace ui {

void GPSScanPanel::paint(Painter& painter) {
    const auto r = screen_rect();
    painter.fill_rectangle(r, Theme::getInstance()->bg_darkest->background);

    if (model_.diag_mode) {
        paint_diagnostics(painter, r);
    } else {
        paint_list_and_plot(painter, r);
    }
}

void GPSScanPanel::paint_list_and_plot(Painter& painter, const Rect& r) {
    char buf[34];

    // Column header (y = r.top())
    painter.draw_string({r.left(), r.top()}, *Theme::getInstance()->fg_cyan, "PRN  C/N0  DoppHz   Chip  Rat");

    // Top 7 PRN rows (y = r.top() + 16 .. r.top() + 127)
    static constexpr size_t kVisibleRows = 7;
    for (size_t i = 0; i < kVisibleRows; i++) {
        const Coord y = r.top() + 16 + static_cast<Coord>(i * 16);
        if (i >= model_.result_count) {
            if (i == 0 && model_.result_count == 0) {
                painter.draw_string({r.left(), y}, *Theme::getInstance()->fg_medium, "Press Scan or SelfTst...");
            }
            continue;
        }

        const uint8_t prn = model_.sorted_prns[i];
        const auto& res = model_.results[prn - 1];
        const bool is_sel = (prn == model_.selected_prn) || (model_.selected_prn == 0 && i == 0);

        const uint32_t chip_x10 = (static_cast<uint32_t>(res.code_phase) * 10230U + 1024U) >> 11;
        const int16_t cn0 = res.cn0_dbhz_x10;
        const int16_t cn0_abs = (cn0 < 0) ? -cn0 : cn0;

        std::snprintf(
            buf, sizeof(buf),
            "%cG%02u %s%2d.%1d %+5dHz %4lu.%1lu %2u.%1u",
            is_sel ? '>' : ' ',
            static_cast<unsigned>(res.prn),
            (cn0 < 0) ? "-" : "",
            static_cast<int>(cn0_abs / 10),
            static_cast<int>(cn0_abs % 10),
            static_cast<int>(res.doppler_hz),
            static_cast<unsigned long>(chip_x10 / 10),
            static_cast<unsigned long>(chip_x10 % 10),
            static_cast<unsigned>(res.ratio_x100 / 100),
            static_cast<unsigned>((res.ratio_x100 % 100) / 10));

        const Style* st = Theme::getInstance()->fg_medium;
        if (res.status == static_cast<uint8_t>(DetStatus::Lock)) {
            st = Theme::getInstance()->fg_green;
        } else if (res.status == static_cast<uint8_t>(DetStatus::Weak)) {
            st = Theme::getInstance()->fg_yellow;
        } else if (is_sel) {
            st = Theme::getInstance()->fg_light;
        }
        painter.draw_string({r.left(), y}, *st, buf);
    }

    // Correlation profile caption (y = r.top() + 130)
    const Coord cap_y = r.top() + 130;
    if (model_.has_profile) {
        const auto& pr = model_.profile.result;
        std::snprintf(buf, sizeof(buf), "G%02u %s Z=%u.%u R=%u.%02u %+d",
                      static_cast<unsigned>(pr.prn),
                      status_tag(pr.status),
                      static_cast<unsigned>(pr.z_score_x10 / 10),
                      static_cast<unsigned>(pr.z_score_x10 % 10),
                      static_cast<unsigned>(pr.ratio_x100 / 100),
                      static_cast<unsigned>(pr.ratio_x100 % 100),
                      static_cast<int>(pr.doppler_hz));
        const Style* cap_st = Theme::getInstance()->fg_cyan;
        if (pr.status == static_cast<uint8_t>(DetStatus::Lock))
            cap_st = Theme::getInstance()->fg_green;
        else if (pr.status == static_cast<uint8_t>(DetStatus::Weak))
            cap_st = Theme::getInstance()->fg_yellow;
        painter.draw_string({r.left(), cap_y}, *cap_st, buf);
    } else {
        painter.draw_string({r.left(), cap_y}, *Theme::getInstance()->fg_cyan, "Corr Peak (+-16 chips): --");
    }

    // Correlation Spike Plot box (y = r.top() + 146 .. r.bottom())
    const Rect plot_box{r.left(), r.top() + 146, r.width(), static_cast<Dim>(r.height() - 146)};
    painter.draw_rectangle(plot_box, Theme::getInstance()->fg_dark->foreground);

    if (!model_.has_profile) {
        painter.draw_string(
            {plot_box.left() + 28, plot_box.top() + 22},
            *Theme::getInstance()->fg_medium,
            "No correlation data yet");
        return;
    }

    const Coord plot_x = plot_box.left() + 24;
    const Coord plot_y = plot_box.top() + 4;
    const Dim plot_h = plot_box.height() - 14;
    const Dim bar_w = 3;  // 64 * 3 = 192 px

    const Coord center_x = plot_x + 32 * bar_w + 1;
    painter.fill_rectangle({center_x, plot_y, 1, plot_h}, Theme::getInstance()->fg_dark->foreground);

    Color bar_color = Theme::getInstance()->fg_medium->foreground;
    if (model_.profile.result.status == static_cast<uint8_t>(DetStatus::Lock)) {
        bar_color = Theme::getInstance()->fg_green->foreground;
    } else if (model_.profile.result.status == static_cast<uint8_t>(DetStatus::Weak)) {
        bar_color = Theme::getInstance()->fg_yellow->foreground;
    }

    for (size_t i = 0; i < PROFILE_BINS; i++) {
        const uint8_t v = model_.profile.bins[i];
        Dim h = static_cast<Dim>((static_cast<uint32_t>(v) * plot_h) / 255U);
        if (h == 0 && v > 0) h = 1;
        if (h > plot_h) h = plot_h;
        const Coord bx = plot_x + static_cast<Coord>(i * bar_w);
        const Coord by = plot_y + static_cast<Coord>(plot_h - h);
        const Color c = (i == PROFILE_BINS / 2) ? Color::white() : bar_color;
        painter.fill_rectangle({bx, by, bar_w - 1, h}, c);
    }

    const Dim mean_h = static_cast<Dim>((static_cast<uint32_t>(model_.profile.mean_level) * plot_h) / 255U);
    const Coord mean_y = plot_y + static_cast<Coord>(plot_h - mean_h);
    painter.fill_rectangle({plot_x, mean_y, static_cast<Dim>(PROFILE_BINS * bar_w), 1}, Theme::getInstance()->fg_cyan->foreground);

    const Dim sec_h = static_cast<Dim>((static_cast<uint32_t>(model_.profile.second_level) * plot_h) / 255U);
    const Coord sec_y = plot_y + static_cast<Coord>(plot_h - sec_h);
    painter.fill_rectangle({plot_box.left() + 4, sec_y, 16, 1}, Theme::getInstance()->fg_yellow->foreground);
    painter.fill_rectangle({plot_x + static_cast<Coord>(PROFILE_BINS * bar_w) + 4, sec_y, 16, 1}, Theme::getInstance()->fg_yellow->foreground);

    const Style& small_style = *Theme::getInstance()->bg_darkest_small;
    painter.draw_string({plot_x, plot_box.bottom() - 9}, small_style, "-16c");
    painter.draw_string({center_x - 5, plot_box.bottom() - 9}, small_style, "0c");
    painter.draw_string({plot_x + static_cast<Coord>(PROFILE_BINS * bar_w) - 20, plot_box.bottom() - 9}, small_style, "+16c");
}

void GPSScanPanel::paint_diagnostics(Painter& painter, const Rect& r) {
    char buf[34];
    Coord y = r.top();
    auto draw = [&](const Style* st, const char* s) {
        painter.draw_string({r.left(), y}, *st, s);
        y += 16;
    };

    const auto& st = model_.stats;

    draw(Theme::getInstance()->fg_cyan, "-- RF & HARDWARE --");

    const uint64_t f_hz = receiver_model.target_frequency();
    const uint32_t f_mhz = static_cast<uint32_t>(f_hz / 1'000'000ULL);
    const uint32_t f_rem = static_cast<uint32_t>((f_hz % 1'000'000ULL) / 100U);
    std::snprintf(buf, sizeof(buf), "F:%lu.%04luM Fs:2.048M",
                  static_cast<unsigned long>(f_mhz), static_cast<unsigned long>(f_rem));
    draw(Theme::getInstance()->fg_light, buf);

    std::snprintf(buf, sizeof(buf), "BW:2.5M ANT:%s AMP:%u L%u V%u",
                  portapack::get_antenna_bias() ? "ON " : "OFF",
                  receiver_model.rf_amp() ? 1U : 0U,
                  static_cast<unsigned>(receiver_model.lna()),
                  static_cast<unsigned>(receiver_model.vga()));
    draw(Theme::getInstance()->fg_light, buf);

    draw(Theme::getInstance()->fg_cyan, "-- ADC & SNAPSHOT --");

    const int16_t mi = st.mean_i_x10;
    const int16_t mq = st.mean_q_x10;
    std::snprintf(buf, sizeof(buf), "DC:%c%d.%d/%c%d.%d Pk:%u/%u",
                  mi < 0 ? '-' : '+', static_cast<int>((mi < 0 ? -mi : mi) / 10), static_cast<int>((mi < 0 ? -mi : mi) % 10),
                  mq < 0 ? '-' : '+', static_cast<int>((mq < 0 ? -mq : mq) / 10), static_cast<int>((mq < 0 ? -mq : mq) % 10),
                  static_cast<unsigned>(st.peak_abs_i), static_cast<unsigned>(st.peak_abs_q));
    draw(Theme::getInstance()->fg_light, buf);

    const uint16_t rms_avg = static_cast<uint16_t>((st.rms_i_x10 + st.rms_q_x10) / 2);
    const int16_t dbfs_x10 = rms_x10_to_dbfs_x10(rms_avg);
    const int16_t dbfs_abs = (dbfs_x10 < 0) ? -dbfs_x10 : dbfs_x10;
    std::snprintf(buf, sizeof(buf), "RMS:%u.%u/%u.%u (-%d.%ddBFS)",
                  static_cast<unsigned>(st.rms_i_x10 / 10), static_cast<unsigned>(st.rms_i_x10 % 10),
                  static_cast<unsigned>(st.rms_q_x10 / 10), static_cast<unsigned>(st.rms_q_x10 % 10),
                  static_cast<int>(dbfs_abs / 10), static_cast<int>(dbfs_abs % 10));
    draw(Theme::getInstance()->fg_light, buf);

    std::snprintf(buf, sizeof(buf), "Q2:%02u/%02u/%02u/%02u%% (16/34/34/16)",
                  static_cast<unsigned>(st.q2_pct[0]), static_cast<unsigned>(st.q2_pct[1]),
                  static_cast<unsigned>(st.q2_pct[2]), static_cast<unsigned>(st.q2_pct[3]));
    draw(Theme::getInstance()->fg_light, buf);

    std::snprintf(buf, sizeof(buf), "Clp:%u.%u%% Drp:%u Snap:%ums",
                  static_cast<unsigned>(st.clip_pm / 10), static_cast<unsigned>(st.clip_pm % 10),
                  static_cast<unsigned>(st.dma_drops), static_cast<unsigned>(st.snapshot_ms));
    draw(st.dma_drops > 0 ? Theme::getInstance()->fg_red : Theme::getInstance()->fg_green, buf);

    draw(Theme::getInstance()->fg_cyan, "-- SCAN & SELECTED PRN --");

    std::snprintf(buf, sizeof(buf), "#%lu %u.%us %lu IFFT Buf:%u",
                  static_cast<unsigned long>(model_.scan_id),
                  static_cast<unsigned>(model_.scan_ms / 1000),
                  static_cast<unsigned>((model_.scan_ms % 1000) / 100),
                  static_cast<unsigned long>(model_.ifft_count),
                  static_cast<unsigned>(st.dma_buffers));
    draw(Theme::getInstance()->fg_light, buf);

    if (model_.result_count > 0 && model_.selected_rank < model_.result_count) {
        const auto& res = model_.results[model_.sorted_prns[model_.selected_rank] - 1];
        const uint32_t chip_x10 = (static_cast<uint32_t>(res.code_phase) * 10230U + 1024U) >> 11;
        const int16_t cn0 = res.cn0_dbhz_x10;
        const int16_t cn0_abs = (cn0 < 0) ? -cn0 : cn0;

        std::snprintf(buf, sizeof(buf), "G%02u %s %s%d.%ddB %+dHz %lu.%u",
                      static_cast<unsigned>(res.prn),
                      status_tag(res.status),
                      cn0 < 0 ? "-" : "",
                      static_cast<int>(cn0_abs / 10), static_cast<int>(cn0_abs % 10),
                      static_cast<int>(res.doppler_hz),
                      static_cast<unsigned long>(chip_x10 / 10),
                      static_cast<unsigned>(chip_x10 % 10));
        const Style* rst = Theme::getInstance()->fg_light;
        if (res.status == static_cast<uint8_t>(DetStatus::Lock))
            rst = Theme::getInstance()->fg_green;
        else if (res.status == static_cast<uint8_t>(DetStatus::Weak))
            rst = Theme::getInstance()->fg_yellow;
        draw(rst, buf);

        std::snprintf(buf, sizeof(buf), "Pk:%lu 2nd:%lu R=%u.%02u Z=%u.%u",
                      static_cast<unsigned long>(res.peak_power),
                      static_cast<unsigned long>(res.second_peak),
                      static_cast<unsigned>(res.ratio_x100 / 100),
                      static_cast<unsigned>(res.ratio_x100 % 100),
                      static_cast<unsigned>(res.z_score_x10 / 10),
                      static_cast<unsigned>(res.z_score_x10 % 10));
        draw(Theme::getInstance()->fg_light, buf);
    } else {
        draw(Theme::getInstance()->fg_medium, "No PRN scanned yet");
    }
}

GPSScanView::GPSScanView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_gps_scan);

    add_children({
        &field_frequency,
        &labels_rf,
        &field_rf_amp,
        &field_lna,
        &field_vga,
        &rssi,
        &button_bias,
        &button_integ,
        &button_dopp,
        &field_prn,
        &check_auto,
        &button_scan,
        &button_test,
        &button_view,
        &button_prev,
        &button_next,
        &text_status,
        &progress_bar,
        &text_adc_strip,
        &panel,
    });

    // GPS L1 C/A defaults: 1575.42 MHz, 1 kHz step, LNA 40, VGA 32, AMP 0
    receiver_model.set_lna(40);
    receiver_model.set_vga(32);
    receiver_model.set_rf_amp(false);
    field_frequency.set_step(1000);
    field_prn.set_value(0);
    progress_bar.set_max(100);
    progress_bar.set_value(0);

    update_bias_button();

    button_bias.on_select = [this](Button&) {
        toggle_bias_tee();
    };

    button_integ.on_select = [this](Button&) {
        cycle_integ();
    };

    button_dopp.on_select = [this](Button&) {
        cycle_dopp();
    };

    check_auto.on_select = [this](Checkbox&, bool v) {
        auto_rescan_ = v;
    };

    button_scan.on_select = [this](Button&) {
        if (scanning_) {
            scanning_ = false;
            selftest_active_ = false;
            button_scan.set_text("Scan");
            send_config(Mode::Idle);
        } else {
            scanning_ = true;
            selftest_active_ = false;
            model_.result_count = 0;
            model_.selected_rank = 0;
            model_.results.fill({});
            button_scan.set_text("Stop");
            panel.set_dirty();
            send_config(auto_rescan_ ? Mode::ScanAuto : Mode::ScanSingle);
        }
    };

    button_test.on_select = [this](Button&) {
        scanning_ = true;
        selftest_active_ = true;
        model_.result_count = 0;
        model_.selected_rank = 0;
        model_.results.fill({});
        button_scan.set_text("Stop");
        panel.set_dirty();
        send_config(Mode::SelfTest);
    };

    button_view.on_select = [this](Button&) {
        model_.diag_mode = !model_.diag_mode;
        button_view.set_text(model_.diag_mode ? "List" : "Diag");
        panel.set_dirty();
    };

    button_prev.on_select = [this](Button&) {
        select_rank(-1);
    };

    button_next.on_select = [this](Button&) {
        select_rank(+1);
    };

    receiver_model.enable();
    send_config(Mode::Idle);
}

GPSScanView::~GPSScanView() {
    send_config(Mode::Idle);
    receiver_model.disable();
    baseband::shutdown();
}

void GPSScanView::focus() {
    button_scan.focus();
}

void GPSScanView::toggle_bias_tee() {
    const bool next = !portapack::get_antenna_bias();
    portapack::set_antenna_bias(next);
    receiver_model.set_antenna_bias();
    update_bias_button();
    StatusRefreshMessage message{};
    EventDispatcher::send_message(message);
    if (model_.diag_mode) panel.set_dirty();
}

void GPSScanView::update_bias_button() {
    const bool on = portapack::get_antenna_bias();
    button_bias.set_text(on ? "ANT:ON" : "ANT:OFF");
    button_bias.set_style(on ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_light);
}

void GPSScanView::cycle_integ() {
    const uint8_t next = (static_cast<uint8_t>(integ_) + 1U) & 3U;
    integ_ = static_cast<Integration>(next);
    button_integ.set_text(integ_label(integ_));
}

void GPSScanView::cycle_dopp() {
    dopp_idx_ = (dopp_idx_ + 1U) & 3U;
    button_dopp.set_text(kDoppLabels[dopp_idx_]);
}

void GPSScanView::send_config(Mode mode) {
    Config cfg{};
    cfg.mode = mode;
    cfg.integration = integ_;
    cfg.doppler_khz = kDoppKhzChoices[dopp_idx_];
    cfg.prn_filter = static_cast<uint8_t>(field_prn.value());
    cfg.profile_prn = model_.selected_prn;
    baseband::set_gps_scan_config(cfg);
}

void GPSScanView::on_stats(const GPSScanStatsMessage& msg) {
    model_.stats = msg.stats;
    model_.scan_ms = msg.scan_ms;
    model_.ifft_count = msg.ifft_count;
    model_.scan_id = msg.scan_id;

    char buf[34];
    switch (msg.scan_state) {
        case ScanState::Idle:
            if (scanning_) {
                scanning_ = false;
                button_scan.set_text("Scan");
            }
            std::snprintf(buf, sizeof(buf), "IDLE  DMA:%u  Ready",
                          static_cast<unsigned>(msg.stats.dma_buffers));
            text_status.set_style(Theme::getInstance()->fg_light);
            break;

        case ScanState::Capturing:
            std::snprintf(buf, sizeof(buf), "CAP #%lu (%s)...",
                          static_cast<unsigned long>(msg.scan_id),
                          selftest_active_ ? "TEST" : integ_label(integ_));
            text_status.set_style(Theme::getInstance()->fg_yellow);
            progress_bar.set_value(0);
            break;

        case ScanState::Scanning:
            std::snprintf(buf, sizeof(buf), "%s G%02u %3u%% %u.%us Lk:%u",
                          selftest_active_ ? "TST" : "SCN",
                          static_cast<unsigned>(msg.current_prn),
                          static_cast<unsigned>(msg.progress_pct),
                          static_cast<unsigned>(msg.scan_ms / 1000),
                          static_cast<unsigned>((msg.scan_ms % 1000) / 100),
                          static_cast<unsigned>(msg.detected_count));
            text_status.set_style(Theme::getInstance()->fg_yellow);
            progress_bar.set_value(msg.progress_pct);
            break;

        case ScanState::Done:
            if (!auto_rescan_ || selftest_active_) {
                scanning_ = false;
                button_scan.set_text("Scan");
            }
            std::snprintf(buf, sizeof(buf), "%s#%lu %u.%us Lk:%u %s",
                          selftest_active_ ? "TST" : "OK ",
                          static_cast<unsigned long>(msg.scan_id),
                          static_cast<unsigned>(msg.scan_ms / 1000),
                          static_cast<unsigned>((msg.scan_ms % 1000) / 100),
                          static_cast<unsigned>(msg.detected_count),
                          msg.stats.is_selftest ? "[SIM]" : "");
            text_status.set_style(msg.detected_count > 0 ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_cyan);
            progress_bar.set_value(100);
            break;
    }
    text_status.set(buf);

    refresh_adc_strip();
    if (model_.diag_mode) {
        panel.set_dirty();
    }
}

void GPSScanView::on_result(const GPSScanResultMessage& msg) {
    const auto& r = msg.result;
    if (r.prn < 1 || r.prn > NUM_PRNS) return;

    model_.scan_id = msg.scan_id;
    model_.results[r.prn - 1] = r;
    sort_results();
    panel.set_dirty();
}

void GPSScanView::on_profile(const GPSScanProfileMessage& msg) {
    model_.profile = msg.profile;
    model_.has_profile = true;
    model_.selected_prn = msg.profile.result.prn;

    for (size_t i = 0; i < model_.result_count; i++) {
        if (model_.sorted_prns[i] == model_.selected_prn) {
            model_.selected_rank = i;
            break;
        }
    }
    panel.set_dirty();
}

void GPSScanView::sort_results() {
    model_.result_count = 0;
    for (uint8_t p = 1; p <= NUM_PRNS; p++) {
        if (model_.results[p - 1].prn == p) {
            model_.sorted_prns[model_.result_count++] = p;
        }
    }

    for (size_t i = 1; i < model_.result_count; i++) {
        const uint8_t key = model_.sorted_prns[i];
        const auto& rk = model_.results[key - 1];
        size_t j = i;
        while (j > 0) {
            const auto& rprev = model_.results[model_.sorted_prns[j - 1] - 1];
            const bool better =
                (rk.status > rprev.status) ||
                (rk.status == rprev.status && rk.z_score_x10 > rprev.z_score_x10) ||
                (rk.status == rprev.status && rk.z_score_x10 == rprev.z_score_x10 && rk.ratio_x100 > rprev.ratio_x100);
            if (!better) break;
            model_.sorted_prns[j] = model_.sorted_prns[j - 1];
            j--;
        }
        model_.sorted_prns[j] = key;
    }

    if (model_.selected_rank >= model_.result_count) {
        model_.selected_rank = 0;
    }
}

void GPSScanView::refresh_adc_strip() {
    const auto& st = model_.stats;
    const uint16_t rms_avg = static_cast<uint16_t>((st.rms_i_x10 + st.rms_q_x10) / 2);
    const int16_t dbfs_x10 = rms_x10_to_dbfs_x10(rms_avg);
    const int16_t dbfs_abs = (dbfs_x10 < 0) ? -dbfs_x10 : dbfs_x10;

    char buf[34];
    std::snprintf(
        buf, sizeof(buf),
        "RMS:%2u/%2u -%2d.%1ddB Clp:%u.%u%% D:%u",
        static_cast<unsigned>(st.rms_i_x10 / 10),
        static_cast<unsigned>(st.rms_q_x10 / 10),
        static_cast<int>(dbfs_abs / 10),
        static_cast<int>(dbfs_abs % 10),
        static_cast<unsigned>(st.clip_pm / 10),
        static_cast<unsigned>(st.clip_pm % 10),
        static_cast<unsigned>(st.dma_drops));

    if (st.dma_drops > 0 || rms_avg < 20 || st.clip_pm >= 100) {
        text_adc_strip.set_style(Theme::getInstance()->fg_red);
    } else if (rms_avg < 80 || rms_avg > 750 || st.clip_pm >= 30) {
        text_adc_strip.set_style(Theme::getInstance()->fg_yellow);
    } else {
        text_adc_strip.set_style(Theme::getInstance()->fg_green);
    }
    text_adc_strip.set(buf);
}

void GPSScanView::select_rank(int delta) {
    if (model_.result_count == 0) return;
    int next = static_cast<int>(model_.selected_rank) + delta;
    if (next < 0) next = static_cast<int>(model_.result_count) - 1;
    if (next >= static_cast<int>(model_.result_count)) next = 0;
    model_.selected_rank = static_cast<size_t>(next);
    model_.selected_prn = model_.sorted_prns[model_.selected_rank];
    panel.set_dirty();
    request_selected_profile();
}

void GPSScanView::request_selected_profile() {
    if (!scanning_ && model_.selected_prn >= 1 && model_.selected_prn <= NUM_PRNS) {
        send_config(Mode::Idle);
    }
}

} /* namespace ui */
