/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
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

#include "capture_app.hpp"
#include "baseband_api.hpp"
#include "portapack.hpp"
#include "ui_freqman.hpp"

using namespace portapack;

namespace ui {

CaptureAppView::CaptureAppView(NavigationView& nav)
    : nav_{nav} {
    freqman_set_bandwidth_option(SPEC_MODULATION, option_bandwidth);
    baseband::run_image(portapack::spi_flash::image_tag_capture);

    add_children({
        &labels,
        &rssi,
        &channel,
        &field_frequency,
        &field_frequency_step,
        &field_rf_amp,
        &field_lna,
        &field_vga,
        &option_bandwidth,
        &option_format,
        &record_view,
        &waterfall,
    });

    /* THE ONE LINE BELOW IS A TEMPORARY KLUDGE WORKAROUND FOR A MYSTERY M4 BASEBAND HANG ISSUE WHEN THE CAPTURE
       APP IS THE FIRST APP STARTED AFTER POWER-UP, OR THE CAPTURE APP IS RUN AFTER RUNNING REPLAY WITH A HIGH
       SAMPLE RATE. IT SHOULD NOT BE NECESSARY SINCE sampling_rate IS ALREADY INITIALIZED BY RADIOSTATE BUT
       APPARENTLY IS ADDING JUST THE RIGHT AMOUNT OF DELAY.  ISSUE DOES NOT AFFECT ALL PORTAPACK UNITS.
       INVESTIGATION IS ONGOING, SEE ISSUE #1283. */
    receiver_model.set_sampling_rate(3072000);
    /* END MYSTERY HANG WORKAROUND. */

    field_frequency_step.set_by_value(receiver_model.frequency_step());
    field_frequency_step.on_change = [this](size_t, OptionsField::value_t v) {
        receiver_model.set_frequency_step(v);
        this->field_frequency.set_step(v);
    };

    option_format.set_selected_index(0);  // Default to C16
    option_format.on_change = [this](size_t, uint32_t file_type) {
        record_view.set_file_type((RecordView::FileType)file_type);
    };

    option_bandwidth.on_change = [this](size_t, uint32_t base_rate) {
        sampling_rate = 8 * base_rate;  // Decimation by 8 done on baseband side
                                        /* base_rate is used for FFT calculation and display LCD, and also in recording writing SD Card rate. */
        /* ex. sampling_rate values, 4Mhz, when recording 500 kHz (BW) and fs 8 Mhz, when selected 1 Mhz BW ... */
        /* ex. recording 500kHz BW to .C16 file, base_rate clock 500kHz x2(I,Q) x 2 bytes (int signed) =2MB/sec rate SD Card  */
        waterfall.on_hide();

        /* Set up proper anti aliasing BPF bandwith in MAX2837 before ADC sampling according to the new added BW Options. */
        auto anti_alias_baseband_bandwidth_filter = filter_bandwidth_for_sampling_rate(sampling_rate);

        record_view.set_sampling_rate(sampling_rate);
        receiver_model.set_sampling_rate(sampling_rate);
        receiver_model.set_baseband_bandwidth(anti_alias_baseband_bandwidth_filter);

        waterfall.on_show();
    };

    option_bandwidth.set_selected_index(7);  // Preselected default option 500kHz.
    receiver_model.enable();

    record_view.on_error = [&nav](std::string message) {
        nav.display_modal("Error", message);
    };
}

CaptureAppView::~CaptureAppView() {
    receiver_model.disable();
    baseband::shutdown();
}

void CaptureAppView::on_hide() {
    waterfall.on_hide();
    View::on_hide();
}

void CaptureAppView::set_parent_rect(const Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);

    ui::Rect waterfall_rect{0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height};
    waterfall.set_parent_rect(waterfall_rect);
}

void CaptureAppView::focus() {
    record_view.focus();
}

} /* namespace ui */
