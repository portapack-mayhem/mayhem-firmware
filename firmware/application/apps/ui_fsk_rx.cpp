/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#include "ui_fsk_rx.hpp"

#include "audio.hpp"
#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"
#include "string_format.hpp"
#include "utility.hpp"
#include "file_path.hpp"

#include "ui_freqman.hpp"

using namespace portapack;
namespace pmem = portapack::persistent_memory;

void FskRxLogger::log_raw_data(const std::string& data, const uint32_t frequency) {
    std::string entry = "Raw: F:" + to_string_dec_uint(frequency) + "Hz";

    // // Raw hex dump of all the codewords
    // for (size_t c = 0; c < 16; c++)
    //     entry += to_string_hex(packet[c], 8) + " ";

    log_file.write_entry(data + entry);
}

void FskRxLogger::log_decoded(Timestamp timestamp, const std::string& text) {
    log_file.write_entry(timestamp, text);
}

namespace ui {
//---------------------------------------------------------------------------------------------------------------
//  Console View
//---------------------------------------------------------------------------------------------------------------
FskRxAppConsoleView::FskRxAppConsoleView(NavigationView& nav, Rect parent_rect)
    : View(parent_rect), nav_{nav} {
    add_child(&console);
};

void FskRxAppConsoleView::on_packet(uint32_t value, bool is_data) {
    if (is_data) {
        console.write(to_string_dec_uint(value) + " ");
    }
}

void FskRxAppConsoleView::on_show() {
    hidden(false);
}

void FskRxAppConsoleView::on_hide() {
    hidden(true);
}

FskRxAppConsoleView::~FskRxAppConsoleView() {
}

//---------------------------------------------------------------------------------------------------------------
//  Spectrum View
//---------------------------------------------------------------------------------------------------------------
FskRxAppView::FskRxAppView(NavigationView& nav, Rect parent_rect)
    : View(parent_rect), nav_{nav} {
    add_child(&waterfall);
    hidden(true);
}

FskRxAppView::~FskRxAppView() {
}

void FskRxAppView::focus() {
}

void FskRxAppView::on_show() {
    hidden(false);
    waterfall.start();
}

void FskRxAppView::on_hide() {
    hidden(true);
    waterfall.stop();
}

//---------------------------------------------------------------------------------------------------------------
//  Base View
//---------------------------------------------------------------------------------------------------------------
FskxRxMainView::FskxRxMainView(NavigationView& nav)
    : nav_{nav} {
    add_children({&tab_view,
                  &view_data,
                  &view_stream,
                  &labels,
                  &rssi,
                  &channel,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &field_frequency,
                  &deviation_frequency,
                  &record_view});

    baseband::run_image(portapack::spi_flash::image_tag_fskrx);

    // DEBUG
    record_view.on_error = [&nav](std::string message) {
        nav.display_modal("Error", message);
    };

    deviation_frequency.on_change = [this](rf::Frequency f) {
        refresh_ui(f);
    };

    // Set initial sampling rate
    /* Bandwidth of 2FSK is 2 * Deviation */
    record_view.set_sampling_rate(initial_deviation * 2);

    field_frequency.set_value(initial_target_frequency);
    deviation_frequency.set_value(initial_deviation);

    logger.append(logs_dir / u"FSKRX.TXT");

    baseband::set_fsk(initial_deviation);

    audio::output::start();
    receiver_model.enable();
}

void FskxRxMainView::handle_decoded(Timestamp timestamp, const std::string& prefix) {
    if (logging()) {
        logger.log_decoded(timestamp, prefix);
    }
}

void FskxRxMainView::refresh_ui(rf::Frequency deviationHz) {
    /* Nyquist would imply a sample rate of 2x bandwidth, but because the ADC
     * provides 2 values (I,Q), the sample_rate is equal to bandwidth here. */
    /* Bandwidth of 2FSK is 2 * Deviation */
    auto sample_rate = deviationHz * 2;

    /* base_rate (bandwidth) is used for FFT calculation and display LCD, and also in recording writing SD Card rate. */
    /* ex. sampling_rate values, 4Mhz, when recording 500 kHz (BW) and fs 8 Mhz, when selected 1 Mhz BW ... */
    /* ex. recording 500kHz BW to .C16 file, base_rate clock 500kHz x2(I,Q) x 2 bytes (int signed) =2MB/sec rate SD Card. */

    if (!view_stream.hidden()) {
        view_stream.waterfall.stop();
    }

    // record_view determines the correct oversampling to apply and returns the actual sample rate.
    // NB: record_view is what actually updates proc_capture baseband settings.
    auto actual_sample_rate = record_view.set_sampling_rate(sample_rate);

    // Update the radio model with the actual sampling rate.
    receiver_model.set_sampling_rate(actual_sample_rate);

    // Get suitable anti-aliasing BPF bandwidth for MAX2837 given the actual sample rate.
    auto anti_alias_filter_bandwidth = filter_bandwidth_for_sampling_rate(actual_sample_rate);
    receiver_model.set_baseband_bandwidth(anti_alias_filter_bandwidth);

    if (!view_stream.hidden()) {
        view_stream.waterfall.start();
    }
}

void FskxRxMainView::focus() {
    field_frequency.focus();
}

void FskxRxMainView::set_parent_rect(const Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);

    ui::Rect waterfall_rect{0, 0, new_parent_rect.width(), new_parent_rect.height() - header_height};
    view_stream.waterfall.set_parent_rect(waterfall_rect);
}

FskxRxMainView::~FskxRxMainView() {
    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();
}
} /* namespace ui */
