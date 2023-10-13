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

#include "ui_freqman.hpp"

using namespace portapack;
namespace pmem = portapack::persistent_memory;

void FskRxLogger::log_raw_data(const std::string& data, const uint32_t frequency) 
{
    std::string entry = "Raw: F:" + to_string_dec_uint(frequency) + "Hz";

    // // Raw hex dump of all the codewords
    // for (size_t c = 0; c < 16; c++)
    //     entry += to_string_hex(packet[c], 8) + " ";

    log_file.write_entry(data + entry);
}

void FskRxLogger::log_decoded(Timestamp timestamp, const std::string& text) 
{
    log_file.write_entry(timestamp, text);
}

namespace ui 
{
    FskRxAppView::FskRxAppView(NavigationView& nav)
        : nav_{nav} 
    {
        baseband::run_image(portapack::spi_flash::image_tag_fskrx);

        add_children(
            {&labels,
            &rssi,
            &channel,
            &field_rf_amp,
            &field_lna,
            &field_vga,
            &field_frequency,
            &field_squelch,
            &field_volume,
            &option_bandwidth,
            &record_view,
            &console,
            &waterfall});

        //Set initial sampling rate
        record_view.set_sampling_rate(previous_bandwidth);
        option_bandwidth.set_by_value(previous_bandwidth);

        // DEBUG
        record_view.on_error = [&nav](std::string message) 
        {
            nav.display_modal("Error", message);
        };

        freqman_set_bandwidth_option(SPEC_MODULATION, option_bandwidth);
        
        option_bandwidth.on_change = [this](size_t, uint32_t bandwidth) 
        {
            refresh_ui(bandwidth);
        };

        field_frequency.set_value(initial_target_frequency);

        field_squelch.set_value(receiver_model.squelch_level());
        field_squelch.on_change = [this](int32_t v) {
            receiver_model.set_squelch_level(v);
        };

        logger.append(LOG_ROOT_DIR "/FSKRX.TXT");

        audio::output::start();
        receiver_model.enable();

        baseband::set_fsk(4500, receiver_model.squelch_level());
    }

    void FskRxAppView::refresh_ui(uint32_t bandwidth)
    {
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

        previous_bandwidth = bandwidth;

        waterfall.start();
    }

    void FskRxAppView::handle_decoded(Timestamp timestamp, const std::string& prefix) 
    {
        if (logging()) 
        {
            logger.log_decoded(timestamp, prefix);
        }
    }

    void FskRxAppView::on_packet(uint32_t value, bool is_data)
    {
        if(is_data)
        {
            console.write(to_string_dec_uint(value) + " ");
        }
    }

    FskRxAppView::~FskRxAppView() 
    {
        audio::output::stop();
        receiver_model.disable();
        baseband::shutdown();
    }

    void FskRxAppView::focus() 
    {
        field_frequency.focus();
    }

    void FskRxAppView::set_parent_rect(const Rect new_parent_rect) 
    {
        View::set_parent_rect(new_parent_rect);

        ui::Rect waterfall_rect{0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height};
        waterfall.set_parent_rect(waterfall_rect);
    }
} /* namespace ui */
