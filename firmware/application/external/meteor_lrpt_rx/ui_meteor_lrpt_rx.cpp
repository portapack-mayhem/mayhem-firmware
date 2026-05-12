/*
 * Copyright (C) 2026
 *
 * This file is part of PortaPack / Mayhem.
 */
#include "ui_meteor_lrpt_rx.hpp"

#include "baseband_api.hpp"
#include "bmp.hpp"
#include "file.hpp"
#include "meteor_lrpt_g4_service.hpp"
#include "lcd_ili9341.hpp"
#include "rtc_time.hpp"
#include "string_format.hpp"
#include "ui_spectrum.hpp"
#include "utility.hpp"
#include "ui/ui_textentry.hpp"

#include <array>

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
    if (check_g4.value()) flags |= 1u << 6;
    if (check_g4.value() && check_g4_live.value()) flags |= 1u << 7;
    const uint8_t sym_k = check_sym_80k.value() ? 80 : 72;
    const size_t soft_blk = (check_m2x.value() && check_interleaved.value()) ? 8192u : 16384u;
    soft_record_view.set_raw_capture_block_size(soft_blk);
    meteor_lrpt_g4_set_input_path_utf8(pm_g4_cadu_path_.c_str());
    meteor_lrpt_g4_set_trace_flags(check_g4_ppm.value() ? 1u : 0u);
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
                  &check_g4,
                  &check_g4_live,
                  &check_g4_ppm,
                  &button_g4_path,
                  &check_g4_bmp_view,
                  &button_bmp_pgup,
                  &button_bmp_pgdn,
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
    apply_g4_settings_to_ui();

    check_m2x.on_select = [this](Checkbox&, bool) { push_baseband_config(); };
    check_interleaved.on_select = [this](Checkbox&, bool) { push_baseband_config(); };
    check_diff.on_select = [this](Checkbox&, bool) { push_baseband_config(); };
    check_legacy_corr.on_select = [this](Checkbox&, bool) { push_baseband_config(); };
    check_sym_80k.on_select = [this](Checkbox&, bool) { push_baseband_config(); };
    check_save_bmp.on_select = [this](Checkbox&, bool) { push_baseband_config(); };
    check_g4.on_select = [this](Checkbox&, bool) { push_baseband_config(); };
    check_g4_live.on_select = [this](Checkbox&, bool) { push_baseband_config(); };
    check_g4_ppm.on_select = [this](Checkbox&, bool) { push_baseband_config(); };
    check_g4_bmp_view.on_select = [this](Checkbox&, bool) { g4_bmp_scroll_ = 0; };
    button_bmp_pgup.on_select = [this](Button&) {
        if (g4_bmp_scroll_ >= 8u)
            g4_bmp_scroll_ -= 8u;
    };
    button_bmp_pgdn.on_select = [this](Button&) { g4_bmp_scroll_ += 8u; };
    button_g4_path.on_select = [this](Button&) {
        text_prompt(
            nav_,
            pm_g4_cadu_path_,
            SharedMemory::MeteorLrptG4Ipc::kInputPathUtf8Max - 1,
            ENTER_KEYBOARD_MODE_SYMBOLS,
            [this](std::string&) { push_baseband_config(); });
    };

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
    close_strip_bmp();
    pm_lrpt_flags_ = encode_lrpt_flags();
    pm_g4_debug_ = check_g4_ppm.value() ? 1u : 0u;
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
    if (check_g4.value())
        v |= 1u << 6;
    if (check_g4.value() && check_g4_live.value())
        v |= 1u << 7;
    return v;
}

void MeteorLrptRxView::apply_lrpt_flags_to_ui() {
    (void)check_m2x.set_value((pm_lrpt_flags_ & (1u << 0)) != 0);
    (void)check_interleaved.set_value((pm_lrpt_flags_ & (1u << 1)) != 0);
    (void)check_diff.set_value((pm_lrpt_flags_ & (1u << 2)) != 0);
    (void)check_legacy_corr.set_value((pm_lrpt_flags_ & (1u << 3)) != 0);
    (void)check_sym_80k.set_value((pm_lrpt_flags_ & (1u << 4)) != 0);
    (void)check_save_bmp.set_value((pm_lrpt_flags_ & (1u << 5)) != 0);
    (void)check_g4.set_value((pm_lrpt_flags_ & (1u << 6)) != 0);
    if (!check_g4.value()) {
        (void)check_g4_live.set_value(false);
    } else {
        (void)check_g4_live.set_value((pm_lrpt_flags_ & (1u << 7)) != 0);
    }
}

void MeteorLrptRxView::apply_g4_settings_to_ui() {
    (void)check_g4_ppm.set_value((pm_g4_debug_ & 1u) != 0);
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
                    " B" + to_string_dec_uint(msg.ber_x1000, 4) +
                    " G" + to_string_dec_int((int32_t)msg.sym_timing_err, 5) +
                    (msg.sym_timing_lock ? "*" : "-") +
                    " d" + to_string_dec_uint(msg.ipc_deint_dropped, 4) +
                    " Sd" + to_string_dec_uint(msg.ipc_sd_deint_errors, 4) +
                    " w" + to_string_dec_uint(msg.m2x_vit_winner, 1) +
                    " v" + to_string_dec_uint(msg.m2x_vit_state_a, 1) + "/" +
                    to_string_dec_uint(msg.m2x_vit_state_b, 1) +
                    " f0x" + to_string_dec_uint(msg.interleaved_mode_flags, 1);
    if (check_g4.value()) {
        const auto& g4 = shared_memory.meteor_lrpt_g4_ipc;
        s += " G4";
        if (g4.live_ring_enable)
            s += "[L]";
        else
            s += "[F]";
        s += " c" + to_string_dec_uint(g4.cadus_processed, 4) +
             " j" + to_string_dec_uint(g4.jpeg_ok_count, 3) +
             " b" + to_string_dec_uint(g4.bmp_write_count, 3) +
             " ro" + to_string_dec_uint(g4.live_ring_overflows, 3) +
             " x" + to_string_dec_uint(g4.drop_bits, 4) +
             " jr" + to_string_dec_uint(g4.last_jresult, 2);
    }
    if (msg.rs_err0 >= 0) {
        s += " RS" + to_string_dec_int((int32_t)msg.rs_err0, 2) + "/" +
             to_string_dec_int((int32_t)msg.rs_err1, 2) + "/" +
             to_string_dec_int((int32_t)msg.rs_err2, 2) + "/" +
             to_string_dec_int((int32_t)msg.rs_err3, 2);
    } else {
        s += " RS----";
    }
    if (check_interleaved.value() && check_m2x.value()) {
        if (msg.interleaved_mode_flags & 2u)
            s += " [Intlv:M0SD+M4]";
        else if (msg.interleaved_mode_flags & 1u)
            s += " [Intlv:host/py]";
        else
            s += " [Intlv]";
    }
    txt_status.set(s);
    if (check_g4.value()) {
        draw_g4_ipc_preview_thumb();
        if (check_g4_bmp_view.value())
            draw_g4_bmp_scroll_view();
    }
}

namespace {

constexpr unsigned kG4BmpViewLines = 44u;

Coord g4_thumb_base_y() {
    return (Coord)((METEOR_PREVIEW_ROW + 1) * 16);
}

unsigned g4_bmp_view_pixel_y() {
    return (unsigned)g4_thumb_base_y() + SharedMemory::MeteorLrptG4Ipc::kPreviewHeight;
}

uint32_t bmp_abs_ih(const bmp_header_t& h) {
    return (uint32_t)(h.height >= 0 ? (uint32_t)h.height : (uint32_t)(-h.height));
}

uint32_t bmp_storage_row_for_visual_top(const bmp_header_t& h, const uint32_t y_top, const uint32_t ih) {
    if (h.height >= 0)
        return ih - 1u - y_top;
    return y_top;
}

}  // namespace

void MeteorLrptRxView::draw_g4_ipc_preview_thumb() {
    const auto& g4 = shared_memory.meteor_lrpt_g4_ipc;
    __DMB();
    const uint32_t seq = g4.preview_seq;
    if (seq == last_g4_preview_seq_)
        return;
    last_g4_preview_seq_ = seq;
    const uint16_t pw = g4.preview_width;
    const uint16_t ph = g4.preview_height;
    if (pw == 0 || ph == 0 || pw > 240)
        return;
    for (unsigned py = 0; py < ph; py++) {
        for (unsigned px = 0; px < pw; px++) {
            const size_t o = ((size_t)py * (size_t)pw + (size_t)px) * 3u;
            line_buffer[px] = Color{
                g4.preview_rgb[o + 0],
                g4.preview_rgb[o + 1],
                g4.preview_rgb[o + 2]};
        }
        for (unsigned px = pw; px < 240; px++)
            line_buffer[px] = Color{0, 0, 0};
        portapack::display.render_line({0, (Coord)(g4_thumb_base_y() + (int)py)}, pw, line_buffer);
    }
}

void MeteorLrptRxView::draw_g4_bmp_scroll_view() {
    static uint8_t row_scratch[6144];
    char path[SharedMemory::MeteorLrptG4Ipc::kLastBmpUtf8Max];
    {
        const auto& g4 = shared_memory.meteor_lrpt_g4_ipc;
        __DMB();
        size_t i = 0;
        for (; i + 1 < sizeof(path); i++) {
            const char c = g4.last_bmp_utf8[i];
            path[i] = c;
            if (c == '\0')
                break;
        }
        path[sizeof(path) - 1] = '\0';
    }
    if (path[0] == '\0')
        return;

    File f{};
    if (f.open(std::filesystem::path{path}, true, false).is_valid())
        return;

    bmp_header_t hdr{};
    const auto rh = f.read(&hdr, sizeof(hdr));
    if (rh.is_error() || rh.value() != sizeof(hdr)) {
        f.close();
        return;
    }
    if (hdr.signature != 0x4D42 || hdr.bpp != 24 || (hdr.compression != 0 && hdr.compression != 3)) {
        f.close();
        return;
    }
    const uint32_t iw = hdr.width;
    const uint32_t ih = bmp_abs_ih(hdr);
    if (iw < 2 || ih < 2) {
        f.close();
        return;
    }
    const uint32_t bpr = (iw * 3u + 3u) & ~3u;
    if (bpr > sizeof(row_scratch)) {
        f.close();
        return;
    }
    const uint32_t max_scroll = ih > kG4BmpViewLines ? ih - kG4BmpViewLines : 0;
    if (g4_bmp_scroll_ > max_scroll)
        g4_bmp_scroll_ = max_scroll;

    const unsigned y0 = g4_bmp_view_pixel_y();
    for (unsigned dy = 0; dy < kG4BmpViewLines; dy++) {
        const uint32_t y_top = g4_bmp_scroll_ + dy;
        if (y_top >= ih) {
            for (unsigned x = 0; x < 240; x++)
                line_buffer[x] = Color{0, 0, 0};
            portapack::display.render_line({0, (Coord)(y0 + dy)}, 240, line_buffer);
            continue;
        }
        const uint32_t store_y = bmp_storage_row_for_visual_top(hdr, y_top, ih);
        const uint64_t row_off = hdr.image_data + (uint64_t)store_y * (uint64_t)bpr;
        const auto sk = f.seek(row_off);
        if (sk.is_error())
            break;
        const auto rd = f.read(row_scratch, bpr);
        if (rd.is_error() || rd.value() < bpr)
            break;
        for (unsigned x = 0; x < 240; x++) {
            const uint32_t sx = (x * (iw - 1u)) / 239u;
            const uint8_t* px = row_scratch + sx * 3u;
            line_buffer[x] = Color{px[2], px[1], px[0]};
        }
        portapack::display.render_line({0, (Coord)(y0 + dy)}, 240, line_buffer);
    }
    f.close();
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

    if (strip_bpr_ == 0) {
        (void)ensure_directory("/LRPT");
        const auto path = std::filesystem::path{"/LRPT"} / ("strip_" + to_string_timestamp(rtc_time::now()) + ".bmp");
        if (!strip_bmp_begin_first_row(path, msg.gray, n))
            return;
    } else {
        if (!strip_bmp_append_row(msg.gray, n))
            return;
    }
}

void MeteorLrptRxView::close_strip_bmp() {
    if (strip_bpr_ == 0)
        return;
    bmp_header_t hdr{};
    (void)strip_bmp_.seek(0);
    const auto rd = strip_bmp_.read(&hdr, sizeof(hdr));
    if (!rd.is_error() && rd.value() == sizeof(hdr)) {
        hdr.height = -(int32_t)strip_rows_;
        hdr.data_size = strip_rows_ * strip_bpr_;
        hdr.size = sizeof(hdr) + hdr.data_size;
        (void)strip_bmp_.seek(0);
        (void)strip_bmp_.write(&hdr, sizeof(hdr));
    }
    strip_bmp_.close();
    strip_bpr_ = 0;
    strip_rows_ = 0;
}

bool MeteorLrptRxView::strip_bmp_begin_first_row(const std::filesystem::path& path, const uint8_t* gray, const uint16_t n) {
    close_strip_bmp();
    constexpr uint32_t kW = 240u;
    strip_bpr_ = (kW * 3u + 3u) & ~3u;
    if (file_exists(path))
        (void)delete_file(path);
    const auto oe = strip_bmp_.open(path, false, true);
    if (oe) {
        strip_bpr_ = 0;
        return false;
    }
    bmp_header_t h{};
    h.signature = 0x4D42;
    h.size = sizeof(h) + strip_bpr_;
    h.reserved_1 = 0;
    h.reserved_2 = 0;
    h.image_data = sizeof(bmp_header_t);
    h.BIH_size = 40;
    h.width = kW;
    h.height = -1;
    h.planes = 1;
    h.bpp = 24;
    h.compression = 0;
    h.data_size = strip_bpr_;
    h.h_res = 100;
    h.v_res = 100;
    h.colors_count = 0;
    h.icolors_count = 0;
    auto wr = strip_bmp_.write(&h, sizeof(h));
    if (wr.is_error() || wr.value() != sizeof(h)) {
        strip_bmp_.close();
        strip_bpr_ = 0;
        return false;
    }
    std::array<uint8_t, 720> row{};
    for (uint16_t i = 0; i < kW; i++) {
        const uint8_t g = (gray && i < n) ? gray[i] : 0;
        row[(size_t)i * 3u + 0] = g;
        row[(size_t)i * 3u + 1] = g;
        row[(size_t)i * 3u + 2] = g;
    }
    wr = strip_bmp_.write(row.data(), strip_bpr_);
    if (wr.is_error() || wr.value() != strip_bpr_) {
        strip_bmp_.close();
        strip_bpr_ = 0;
        return false;
    }
    strip_rows_ = 1;
    return true;
}

bool MeteorLrptRxView::strip_bmp_append_row(const uint8_t* gray, const uint16_t n) {
    if (strip_bpr_ == 0 || !gray)
        return false;
    constexpr uint32_t kW = 240u;
    std::array<uint8_t, 720> row{};
    for (uint16_t i = 0; i < kW; i++) {
        const uint8_t g = i < n ? gray[i] : 0;
        row[(size_t)i * 3u + 0] = g;
        row[(size_t)i * 3u + 1] = g;
        row[(size_t)i * 3u + 2] = g;
    }
    const auto wr = strip_bmp_.write(row.data(), strip_bpr_);
    if (wr.is_error() || wr.value() != strip_bpr_)
        return false;
    strip_rows_++;
    return true;
}

}  // namespace ui::external_app::meteor_lrpt_rx
