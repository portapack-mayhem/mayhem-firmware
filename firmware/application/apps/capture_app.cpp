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
        &check_trim,
        &record_view,
        &waterfall,
    });

    field_frequency_step.set_by_value(receiver_model.frequency_step());
    field_frequency_step.on_change = [this](size_t, OptionsField::value_t v) {
        receiver_model.set_frequency_step(v);
        this->field_frequency.set_step(v);
    };

    option_format.set_selected_index(previous_format);
    option_format.on_change = [this](size_t, uint32_t file_type) {
        previous_format = file_type;
        record_view.set_file_type((RecordView::FileType)file_type);
    };

    check_trim.set_value(previous_trim);
    check_trim.on_select = [this](Checkbox&, bool v) {
        previous_trim = v;
        record_view.set_auto_trim(v);
    };

    freqman_set_bandwidth_option(SPEC_MODULATION, option_bandwidth);
    option_bandwidth.on_change = [this](size_t, uint32_t bandwidth) {
        /* Nyquist would imply a sample rate of 2x bandwidth, but because the ADC
         * provides 2 values (I,Q), the sample_rate is equal to bandwidth here. */
        auto sample_rate = bandwidth;

        /* base_rate (bandwidth) is used for FFT calculation and display LCD, and also in recording writing SD Card rate. */
        /* ex. sampling_rate values, 4Mhz, when recording 500 kHz (BW) and fs 8 Mhz, when selected 1 Mhz BW ... */
        /* ex. recording 500kHz BW to .C16 file, base_rate clock 500kHz x2(I,Q) x 2 bytes (int signed) =2MB/sec rate SD Card. */

        waterfall.stop();

        // record_view determines the correct oversampling to apply and returns the actual sample rate.
        // NB: record_view is what actually updates proc_capture baseband settings.
        auto actual_sample_rate = record_view.set_sampling_rate(sample_rate);

        // Update the radio model with the actual sampling rate.
        receiver_model.set_sampling_rate(actual_sample_rate);

        // Get suitable anti-aliasing BPF bandwidth for MAX2837 given the actual sample rate.
        auto anti_alias_filter_bandwidth = filter_bandwidth_for_sampling_rate(actual_sample_rate);
        receiver_model.set_baseband_bandwidth(anti_alias_filter_bandwidth);

        // Automatically switch default capture format to C8 when bandwidth setting is increased to >=1.5MHz anb back to C16 for <=1,25Mhz
        if ((bandwidth >= 1500000) && (previous_bandwidth < 1500000)) {
            option_format.set_selected_index(1);  // Default C8 format for REC, 1500K ... 5500k
        }
        if ((bandwidth <= 1250000) && (previous_bandwidth > 1250000)) {
            option_format.set_selected_index(0);  // Default C16 format for REC , 12k5 ... 1250K
        }
        previous_bandwidth = bandwidth;

        waterfall.start();
    };

    receiver_model.enable();
    option_bandwidth.set_by_value(previous_bandwidth);

    record_view.on_error = [&nav](std::string message) {
        nav.display_modal("Error", message);
    };
}

CaptureAppView::CaptureAppView(
    NavigationView& nav,
    ReceiverModel::settings_t override)
    : CaptureAppView(nav) {
    // Settings to override when launched from another app (versus from AppSettings .ini file)
    field_frequency.set_value(override.frequency_app_override);
}

CaptureAppView::~CaptureAppView() {
    receiver_model.disable();
    baseband::shutdown();
}

void CaptureAppView::set_parent_rect(const Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);

    ui::Rect waterfall_rect{0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height};
    waterfall.set_parent_rect(waterfall_rect);
}

void CaptureAppView::focus() {
    record_view.focus();
}

void CaptureAppView::on_freqchg(int64_t freq) {
    field_frequency.set_value(freq);
}

} /* namespace ui */
