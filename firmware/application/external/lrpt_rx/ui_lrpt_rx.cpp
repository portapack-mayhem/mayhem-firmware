/*
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

#include "ui_lrpt_rx.hpp"

#include "baseband_api.hpp"
#include "file_path.hpp"
#include "portapack.hpp"
#include "rtc_time.hpp"
#include "string_format.hpp"

using namespace portapack;

namespace ui::external_app::lrpt_rx {

namespace {

constexpr Dim PREVIEW_ROW = 5;  // first text row of the image preview

// red, green and blue APIDs of each composite choice
constexpr uint8_t composites[][3] = {
    {65, 65, 64},  // 221
    {66, 65, 64},  // 321
    {64, 65, 66},  // 123
    {64, 64, 64},
    {65, 65, 65},
    {66, 66, 66},
    {67, 67, 67},
    {68, 68, 68},
    {69, 69, 69},
};
constexpr size_t composite_count = sizeof(composites) / sizeof(composites[0]);

// a plain array: an external app has no static constructors
constexpr char16_t deint_path[] = u"/LRPT/DEINT.TMP";

// a block in the file is padded to whole sectors
constexpr size_t deint_pad = Deinterleaver::FILE_BLOCK - lrpt::DEINT_BLOCK;
const uint8_t zeros[deint_pad ? deint_pad : 1]{};

// at most 28 characters a line, 13 lines
const char* const warning_80k =
    "Experimental. In 80k the\n"
    "code is spread over 18 s:\n"
    "it is buffered on the SD\n"
#if defined(PORTAPACK_BOARD_PRALINE)
    "card, LRPT/DEINT.TMP 1.3 MB.\n"
#else
    "card, LRPT/DEINT.TMP 0.7 MB.\n"
#endif
    "- the image comes 18 s\n"
    "  after the lock\n"
    "- a slow or busy card gives\n"
    "  gaps and black blocks,\n"
    "  counted in S\n"
#if defined(PORTAPACK_BOARD_PRALINE)
    "- 4 bits a soft value\n"
#else
    "- 2 bits a soft value: about\n"
    "  1 dB less than 72k\n"
#endif
    "- many SD writes and reads";

}  // namespace

void Constellation::update(const int8_t* points) {
    memcpy(points_, points, sizeof(points_));
    set_dirty();
}

void Constellation::paint(Painter& painter) {
    const Rect r = screen_rect();
    painter.fill_rectangle(r, Color::black());
    const Point c = r.center();
    painter.draw_hline({r.left(), c.y()}, r.width(), Color::dark_grey());
    painter.draw_vline({c.x(), r.top()}, r.height(), Color::dark_grey());
    const int lim = r.width() / 2 - 1;
    for (size_t i = 0; i < lrpt::CONSTELLATION_POINTS; i++) {
        int x = points_[i][0] / 4;
        int y = -points_[i][1] / 4;
        if (x > lim) x = lim;
        if (x < -lim) x = -lim;
        if (y > lim) y = lim;
        if (y < -lim) y = -lim;
        display.draw_pixel({c.x() + x, c.y() + y}, Color::green());
    }
}

LrptRxView::LrptRxView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_lrpt_rx);

    add_children({&options_frequency, &label_mhz, &field_rf_amp, &field_lna, &field_vga, &rssi, &channel,
                  &button_record, &text_carrier, &text_code, &text_sync, &text_marker, &text_offset,
                  &text_frames, &constellation, &options_composite, &options_mode, &text_record});

    options_frequency.on_change = [](size_t, OptionsField::value_t f) {
        receiver_model.set_target_frequency(f);
    };
    options_frequency.set_by_value(receiver_model.target_frequency());
    receiver_model.set_target_frequency(options_frequency.selected_index_value());

    if (composite_ >= composite_count) composite_ = 0;
    options_composite.set_selected_index(composite_, false);
    set_composite(composite_);
    options_composite.on_change = [this](size_t, OptionsField::value_t v) {
        set_composite(v);
    };

    options_mode.set_selected_index(0, false);
    options_mode.on_change = [this](size_t, OptionsField::value_t v) {
        const auto mode = (v == (int32_t)lrpt::Mode::M80K) ? lrpt::Mode::M80K : lrpt::Mode::M72K;
        set_mode(mode);
        if (mode == lrpt::Mode::M80K && !warned_80k_) {
            warned_80k_ = true;
            nav_.display_modal("80k mode", warning_80k, INFO, nullptr, true);
        }
    };

    button_record.on_select = [this](Button&) {
        if (recording_)
            stop();
        else
            start();
    };

    show_record();
    receiver_model.enable();
}

LrptRxView::~LrptRxView() {
    stopping_ = true;
    if (recording_) stop();
    // the M4 buffers go with the baseband
    shared_ = nullptr;
    decoder_.set_planes(nullptr);
    receiver_model.disable();
    baseband::shutdown();
    if (deint_open_) {
        deint_file_.close();
        delete_file(deint_path);
    }
}

// The radio's rate, the M4's demodulator and, for 80k, the file of the deinterleaver.
void LrptRxView::set_mode(lrpt::Mode mode) {
    if (mode == mode_) return;
    mode_ = mode;
    receiver_model.set_sampling_rate(lrpt::baseband_rate(mode_));
    if (shared_) shared_->mode_request = (uint8_t)mode_;
    decoder_.restart();
    deint_.reset();
    if (mode_ == lrpt::Mode::M80K && !deint_open_) {
        ensure_directory(u"/LRPT");
        delete_file(deint_path);
        deint_open_ = !deint_file_.open(deint_path, false, true);
    } else if (mode_ == lrpt::Mode::M72K && deint_open_) {
        deint_file_.close();
        delete_file(deint_path);
        deint_open_ = false;
    }
    text_marker.set_style(Theme::getInstance()->fg_dark);
}

bool LrptRxView::deint_write(void* context, uint32_t offset, const uint8_t* data, size_t length) {
    auto& self = *static_cast<LrptRxView*>(context);
    if (!self.deint_open_ || self.deint_file_.seek(offset).is_error()) return false;
    const auto r = self.deint_file_.write(data, length);
    if (r.is_error() || r.value() != length) return false;
    if (deint_pad && self.deint_file_.write(zeros, deint_pad).is_error()) return false;
    return true;
}

bool LrptRxView::deint_read(void* context, uint32_t offset, uint8_t* data, size_t length) {
    auto& self = *static_cast<LrptRxView*>(context);
    if (!self.deint_open_ || self.deint_file_.seek(offset).is_error()) return false;
    const auto r = self.deint_file_.read(data, length);
    return !r.is_error() && r.value() == length;
}

// 80k: whatever the M4 waits for from the card. Called on each message and in long jobs.
void LrptRxView::service() {
    if (mode_ == lrpt::Mode::M80K && shared_) deint_.service(shared_);
}

void LrptRxView::focus() {
    options_frequency.focus();
}

void LrptRxView::set_composite(uint32_t composite) {
    if (composite >= composite_count) composite = 0;
    composite_ = composite;
    decoder_.set_apids(composites[composite][0], composites[composite][1], composites[composite][2]);
}

void LrptRxView::set_shared(lrpt::Shared* shared) {
    if (shared == shared_) return;
    shared_ = shared;
    decoder_.set_planes(shared_ ? &shared_->planes[0][0] : nullptr);
    if (shared_) shared_->mode_request = (uint8_t)mode_;
}

void LrptRxView::on_status(const LrptRxStatusMessage& message) {
    if (stopping_) return;
    set_shared(message.shared);

    if (message.flags != flags_) {
        flags_ = message.flags;
        const auto on = Theme::getInstance()->fg_green;
        const auto off = Theme::getInstance()->fg_dark;
        text_carrier.set_style((flags_ & LrptRxStatusMessage::FLAG_CARRIER) ? on : off);
        text_code.set_style((flags_ & LrptRxStatusMessage::FLAG_CODE) ? on : off);
        text_sync.set_style((flags_ & LrptRxStatusMessage::FLAG_SYNC) ? on : off);
        text_marker.set_style((flags_ & LrptRxStatusMessage::FLAG_MARKER) ? on : off);
    }
    service();

    text_offset.set(to_string_dec_int(message.offset_hz) + "Hz BER " +
                    to_string_dec_uint(message.ber_permille / 10) + "." +
                    to_string_dec_uint(message.ber_permille % 10) + "%");

    const auto& st = decoder_.stats();
    std::string frames = "F" + to_string_dec_uint(st.frames) + " C" + to_string_dec_uint(st.rs_fixed) +
                         " X" + to_string_dec_uint(st.rs_failed) + " O" + to_string_dec_uint(message.overruns);
    if (message.mode == (uint8_t)lrpt::Mode::M80K) {
        // blocks lost to the card, and the deinterleaver filling up
        frames += " S" + to_string_dec_uint(message.late + message.lost + deint_.errors());
        if (deint_.fill_percent() < 100) frames += " B" + to_string_dec_uint(deint_.fill_percent()) + "%";
        if (!deint_open_) frames = "No file on the SD card";
    }
    text_frames.set(frames);

    if (shared_) constellation.update(&shared_->constellation[0][0]);
}

void LrptRxView::on_frame(const LrptRxFrameMessage& message) {
    if (stopping_) return;
    set_shared(message.shared);
    if (!shared_ || message.slot >= lrpt::RING_SLOTS) return;
    decoder_.frame(shared_->frames[message.slot]);
    shared_->busy[message.slot] = 0;
    service();
}

void LrptRxView::on_strip(void* context) {
    auto& self = *static_cast<LrptRxView*>(context);
    self.preview_strip();
    if (self.recording_) {
        if (!self.record_strip()) {
            self.stop(false);
            self.text_record.set("SD write error");
            return;
        }
        if (++self.strips_uncommitted_ >= COMMIT_STRIPS) {
            self.strips_uncommitted_ = 0;
            self.bmp_.commit_rows();
        }
        self.show_record();
    }
}

Coord LrptRxView::preview_top() {
    return screen_pos().y() + UI_POS_Y(PREVIEW_ROW);
}

Dim LrptRxView::preview_height() {
    return screen_rect().height() - UI_POS_Y(PREVIEW_ROW);
}

// The image scaled to the screen width, keeping its aspect, from the top again when full.
void LrptRxView::preview_strip() {
    // not while a dialog is over the view
    if (!shared_ || !parent()) return;
    const size_t width = (screen_width < PREVIEW_MAX_WIDTH) ? screen_width : PREVIEW_MAX_WIDTH;
    const Dim height = preview_height();
    if (height <= 0) return;
    const uint8_t* planes[3] = {shared_->planes[0], shared_->planes[1], shared_->planes[2]};

    for (size_t r = 0; r < lrpt::STRIP_ROWS; r++, preview_rows_++) {
        const uint32_t p = preview_rows_ * width / lrpt::STRIP_WIDTH;
        if (preview_rows_ && p == (preview_rows_ - 1) * width / lrpt::STRIP_WIDTH) continue;

        const size_t row = r * lrpt::STRIP_WIDTH;
        size_t x0 = 0;
        for (size_t x = 0; x < width; x++) {
            const size_t x1 = (x + 1) * lrpt::STRIP_WIDTH / width;
            uint32_t sum[3] = {0, 0, 0};
            for (size_t i = x0; i < x1; i++)
                for (size_t c = 0; c < 3; c++) sum[c] += planes[c][row + i];
            const uint32_t n = x1 - x0;
            line_[x] = Color(sum[0] / n, sum[1] / n, sum[2] / n);
            x0 = x1;
        }
        const Coord y = preview_top() + (p % height);
        display.render_line({screen_pos().x(), y}, width, line_);
        // where the next row goes
        display.fill_rectangle({screen_pos().x(), preview_top() + (Coord)((p + 1) % height), (Dim)width, 1}, Color::dark_grey());
    }
}

// The strip as 8 BGR rows at the end of the file.
bool LrptRxView::record_strip() {
    if (!shared_) return false;
    const uint8_t* red = shared_->planes[0];
    const uint8_t* green = shared_->planes[1];
    const uint8_t* blue = shared_->planes[2];
    for (size_t i = 0; i < lrpt::PLANE_BYTES; i += CHUNK_PIXELS) {
        uint8_t* d = chunk_;
        for (size_t k = i; k < i + CHUNK_PIXELS; k++) {
            *d++ = blue[k];
            *d++ = green[k];
            *d++ = red[k];
        }
        if (!bmp_.append_rows(chunk_, sizeof(chunk_))) return false;
        service();
    }
    rows_written_ += lrpt::STRIP_ROWS;
    return true;
}

void LrptRxView::start() {
    ensure_directory(u"/LRPT");
    file_name_ = "LRPT_" + to_string_timestamp(rtc_time::now()) + ".BMP";
    if (!bmp_.create("/LRPT/" + file_name_, lrpt::STRIP_WIDTH, 0)) {
        text_record.set("Can't create file");
        return;
    }
    recording_ = true;
    rows_written_ = 0;
    strips_uncommitted_ = 0;
    button_record.set_text("Stop");
    show_record();
}

void LrptRxView::stop(bool partial) {
    // the strip being decoded, as far as it came
    if (partial && shared_ && decoder_.strip_pending()) record_strip();
    bmp_.commit_rows();
    bmp_.close();
    recording_ = false;
    button_record.set_text("Start");
    if (!stopping_) text_record.set(file_name_ + " " + to_string_dec_uint(rows_written_));
}

void LrptRxView::show_record() {
    if (recording_)
        text_record.set("Rec " + to_string_dec_uint(rows_written_) + " rows");
    else
        text_record.set("1568 px BMP in /LRPT");
}

}  // namespace ui::external_app::lrpt_rx
