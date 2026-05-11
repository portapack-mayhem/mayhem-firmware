/*
 * Copyright (C) 2026
 *
 * This file is part of PortaPack / Mayhem.
 */
#include "ui_meteor_lrpt_rx.hpp"

#include "baseband_api.hpp"
#include "file.hpp"
#include "lcd_ili9341.hpp"
#include "rtc_time.hpp"
#include "string_format.hpp"
#include "ui_spectrum.hpp"
#include "utility.hpp"

using namespace portapack;

namespace ui::external_app::meteor_lrpt_rx {

void MeteorLrptRxView::focus() {
    field_frequency.focus();
}

void MeteorLrptRxView::push_baseband_config() {
    uint8_t flags = 0;
    if (check_m2x.value()) flags |= 1u << 0;
    if (check_interleaved.value()) flags |= 1u << 1;
    if (check_diff.value()) flags |= 1u << 2;
    if (check_legacy_corr.value()) flags |= 1u << 3;
    const uint8_t sym_k = check_sym_80k.value() ? 80 : 72;
    baseband::set_meteor_lrpt_rx_config(flags, sym_k);
}

MeteorLrptRxView::MeteorLrptRxView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&rssi,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &field_volume,
                  &field_frequency,
                  &txt_status,
                  &check_m2x,
                  &check_interleaved,
                  &check_diff,
                  &check_legacy_corr,
                  &check_sym_80k,
                  &check_save_bmp,
                  &record_view,
                  &soft_record_view});

    record_view.set_filename_date_frequency(true);
    record_view.on_error = [&nav](std::string message) {
        nav.display_modal("Error", message);
    };
    soft_record_view.set_filename_date_frequency(true);
    soft_record_view.on_error = [&nav](std::string message) {
        nav.display_modal("Error", message);
    };

    const auto actual_sr = record_view.set_sampling_rate(3072000);
    (void)soft_record_view.set_sampling_rate(3072000);
    receiver_model.set_sampling_rate(actual_sr);
    receiver_model.set_baseband_bandwidth(filter_bandwidth_for_sampling_rate((int32_t)actual_sr));

    field_frequency.set_value(137'900'000);
    field_frequency.set_step(1000);
    field_frequency.updated = [this](auto) {
        push_baseband_config();
    };
    field_frequency.on_edit_shown = [this]() {
        paused = true;
    };
    field_frequency.on_edit_hidden = [this]() {
        paused = false;
    };

    apply_lrpt_flags_to_ui();

    check_m2x.on_select = [this](Checkbox&, bool) { push_baseband_config(); };
    check_interleaved.on_select = [this](Checkbox&, bool) { push_baseband_config(); };
    check_diff.on_select = [this](Checkbox&, bool) { push_baseband_config(); };
    check_legacy_corr.on_select = [this](Checkbox&, bool) { push_baseband_config(); };
    check_sym_80k.on_select = [this](Checkbox&, bool) { push_baseband_config(); };
    check_save_bmp.on_select = [this](Checkbox&, bool) { push_baseband_config(); };

    receiver_model.set_hidden_offset(0);
    receiver_model.set_modulation(ReceiverModel::Mode::Capture);
    receiver_model.set_squelch_level(0);
    receiver_model.enable();

    txt_status.set("LRPT demod + CADU (beta).");

    push_baseband_config();
}

MeteorLrptRxView::~MeteorLrptRxView() {
    stopping = true;
    soft_record_view.stop();
    record_view.stop();
    bmp_preview.close();
    pm_lrpt_flags_ = encode_lrpt_flags();
    receiver_model.disable();
    baseband::shutdown();
}

uint8_t MeteorLrptRxView::encode_lrpt_flags() const {
    uint8_t v = 0;
    if (check_m2x.value())
        v |= 1u << 0;
    if (check_interleaved.value())
        v |= 1u << 1;
    if (check_diff.value())
        v |= 1u << 2;
    if (check_legacy_corr.value())
        v |= 1u << 3;
    if (check_sym_80k.value())
        v |= 1u << 4;
    if (check_save_bmp.value())
        v |= 1u << 5;
    return v;
}

void MeteorLrptRxView::apply_lrpt_flags_to_ui() {
    (void)check_m2x.set_value((pm_lrpt_flags_ & (1u << 0)) != 0);
    (void)check_interleaved.set_value((pm_lrpt_flags_ & (1u << 1)) != 0);
    (void)check_diff.set_value((pm_lrpt_flags_ & (1u << 2)) != 0);
    (void)check_legacy_corr.set_value((pm_lrpt_flags_ & (1u << 3)) != 0);
    (void)check_sym_80k.set_value((pm_lrpt_flags_ & (1u << 4)) != 0);
    (void)check_save_bmp.set_value((pm_lrpt_flags_ & (1u << 5)) != 0);
}

void MeteorLrptRxView::on_status(MeteorLrptRxStatusDataMessage msg) {
    std::string s = "CADU:" + to_string_dec_uint(msg.cadu_frames, 5) +
                    " ASM+" + to_string_dec_uint(msg.cadu_asm_accepts, 4) +
                    "/-" + to_string_dec_uint(msg.cadu_asm_rejects, 4) +
                    " s:" + to_string_dec_uint(msg.soft_sym_count, 4) +
                    " L" + to_string_dec_uint(msg.demod_lock, 1) +
                    "/" + to_string_dec_uint(msg.fec_lock, 1) +
                    " V" + to_string_dec_uint(msg.viterbi_sync, 1) +
                    " D" + to_string_dec_uint(msg.deframer_sync, 1) +
                    " rot" + to_string_dec_uint(msg.soft_rotate_shift, 2) +
                    " c" + to_string_dec_uint(msg.corr_score, 2) +
                    (msg.corr_lock ? "*" : "-") +
                    " a" + to_string_dec_uint(msg.soft_align_skip, 4) +
                    " B" + to_string_dec_uint(msg.ber_x1000, 4);
    if (msg.rs_err0 >= 0) {
        s += " RS" + to_string_dec_int((int32_t)msg.rs_err0, 2) + "/" +
             to_string_dec_int((int32_t)msg.rs_err1, 2) + "/" +
             to_string_dec_int((int32_t)msg.rs_err2, 2) + "/" +
             to_string_dec_int((int32_t)msg.rs_err3, 2);
    } else {
        s += " RS----";
    }
    txt_status.set(s);
}

void MeteorLrptRxView::on_preview(MeteorLrptRxPreviewLineMessage msg) {
    const uint16_t n = msg.pixel_count > 240 ? 240 : msg.pixel_count;
    for (uint16_t i = 0; i < n; i++) {
        const uint8_t g = msg.gray[i];
        line_buffer[i] = Color{g, g, g};
    }
    for (uint16_t i = n; i < 240; i++)
        line_buffer[i] = Color{0, 0, 0};

    portapack::display.render_line({0, METEOR_PREVIEW_ROW * 16}, 240, line_buffer);

    if (!check_save_bmp.value() || n == 0)
        return;

    if (!bmp_preview.is_loaded()) {
        (void)ensure_directory("/LRPT");
        const auto path = std::filesystem::path{"/LRPT"} / ("strip_" + to_string_timestamp(rtc_time::now()) + ".bmp");
        if (!bmp_preview.create(path, 240, 1)) {
            bmp_preview.close();
            return;
        }
    }
    if (!bmp_preview.expand_y_delta(1))
        return;
    for (uint16_t i = 0; i < 240; i++) {
        const uint8_t g = i < n ? msg.gray[i] : 0;
        Color px{g, g, g};
        (void)bmp_preview.write_next_px(px);
    }
}

}  // namespace ui::external_app::meteor_lrpt_rx
