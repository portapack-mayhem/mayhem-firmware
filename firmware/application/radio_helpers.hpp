/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
 * Copyright (C) 2023 gullradriel, Nilorea Studio Inc.
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

#ifndef _RADIO_HELPERS
#define _RADIO_HELPERS

void change_mode(NavigationView &nav, freqman_index_t new_mod, OptionsField &field_mode, OptionsField &field_bw, Text &text_ctcss, RecordView *record_view) {
    field_mode.on_change = [this](size_t, OptionsField::value_t) {};
    field_bw.on_change = [this](size_t, OptionsField::value_t) {};
    if( record_view )
    {
        recon_stop_recording();
        if (new_mod != SPEC_MODULATION) {
            remove_children({record_view});
            delete record_view;
            record_view = new RecordView({0, 0, 30 * 8, 1 * 16}, u"AUTO_AUDIO_", u"AUDIO", RecordView::FileType::WAV, 4096, 4);
            record_view->set_filename_date_frequency(true);
            add_children({record_view});
        }
        if (new_mod == SPEC_MODULATION) {
            audio::output::stop();
            remove_children({record_view});
            delete record_view;
            record_view = new RecordView({0, 0, 30 * 8, 1 * 16}, u"AUTO_RAW_", u"CAPTURES", RecordView::FileType::RawS16, 16384, 3);
            record_view->set_filename_date_frequency(true);
            add_children({record_view});
        }
        record_view->hidden(true);
        record_view->on_error = [this](std::string message) {
            nav_.display_modal("Error", message);
        };
    }
    receiver_model.disable();
    baseband::shutdown();
    size_t recording_sampling_rate = 0;
    switch (new_mod) {
        case AM_MODULATION:
            freqman_set_bandwidth_option(new_mod, field_bw);
            // bw DSB (0) default
            field_bw.set_by_value(0);
            baseband::run_image(portapack::spi_flash::image_tag_am_audio);
            receiver_model.set_modulation(ReceiverModel::Mode::AMAudio);
            receiver_model.set_am_configuration(field_bw.selected_index_value());
            field_bw.on_change = [this](size_t, OptionsField::value_t n) { receiver_model.set_am_configuration(n); };
            text_ctcss.set("        ");
            recording_sampling_rate = 12000;
            break;
        case NFM_MODULATION:
            freqman_set_bandwidth_option(new_mod, field_bw);
            // bw 16k (2) default
            field_bw.set_by_value(2);
            baseband::run_image(portapack::spi_flash::image_tag_nfm_audio);
            receiver_model.set_modulation(ReceiverModel::Mode::NarrowbandFMAudio);
            receiver_model.set_nbfm_configuration(field_bw.selected_index_value());
            field_bw.on_change = [this](size_t, OptionsField::value_t n) { receiver_model.set_nbfm_configuration(n); };
            recording_sampling_rate = 24000;
            break;
        case WFM_MODULATION:
            freqman_set_bandwidth_option(new_mod, field_bw);
            // bw 200k (0) default
            field_bw.set_by_value(0);
            baseband::run_image(portapack::spi_flash::image_tag_wfm_audio);
            receiver_model.set_modulation(ReceiverModel::Mode::WidebandFMAudio);
            receiver_model.set_wfm_configuration(field_bw.selected_index_value());
            field_bw.on_change = [this](size_t, OptionsField::value_t n) { receiver_model.set_wfm_configuration(n); };
            text_ctcss.set("        ");
            recording_sampling_rate = 48000;
            break;
        case SPEC_MODULATION:
            freqman_set_bandwidth_option(new_mod, field_bw);
            // bw 200k (0) default
            baseband::run_image(portapack::spi_flash::image_tag_capture);
            receiver_model.set_modulation(ReceiverModel::Mode::Capture);
            field_bw.set_by_value(0);
            field_bw.on_change = [this](size_t, OptionsField::value_t sampling_rate) {
                uint32_t anti_alias_baseband_bandwidth_filter = 2500000;
                switch (sampling_rate) {                                 // we use the var fs (sampling_rate) , to set up BPF aprox < fs_max/2 by Nyquist theorem.
                    case 0 ... 2000000:                                  // BW Captured range  (0 <= 250kHz max )  fs = 8 x 250 kHz
                        anti_alias_baseband_bandwidth_filter = 1750000;  // Minimum BPF MAX2837 for all those lower BW options.
                        break;
                    case 4000000 ... 6000000:                            // BW capture  range (500k ... 750kHz max )  fs_max = 8 x 750kHz = 6Mhz
                                                                         // BW 500k ... 750kHz   ,  ex. 500kHz   (fs = 8*BW =  4Mhz) , BW 600kHz (fs = 4,8Mhz) , BW  750 kHz (fs = 6Mhz)
                        anti_alias_baseband_bandwidth_filter = 2500000;  // in some IC MAX2837 appear 2250000 , but both works similar.
                        break;
                    case 8800000:  // BW capture 1,1Mhz  fs = 8 x 1,1Mhz = 8,8Mhz . (1Mhz showed slightly higher noise background).
                        anti_alias_baseband_bandwidth_filter = 3500000;
                        break;
                    case 14000000:  // BW capture 1,75Mhz  , fs = 8 x 1,75Mhz = 14Mhz
                                    // good BPF, good matching, but LCD making flicker , refresh rate should be < 20 Hz , but reasonable picture
                        anti_alias_baseband_bandwidth_filter = 5000000;
                        break;
                    case 16000000:  // BW capture 2Mhz  , fs = 8 x 2Mhz = 16Mhz
                                    // good BPF, good matching, but LCD making flicker , refresh rate should be < 20 Hz , but reasonable picture
                        anti_alias_baseband_bandwidth_filter = 6000000;
                        break;
                    case 20000000:  // BW capture 2,5Mhz  , fs= 8 x 2,5 Mhz = 20Mhz
                                    // good BPF, good matching, but LCD making flicker , refresh rate should be < 20 Hz , but reasonable picture
                        anti_alias_baseband_bandwidth_filter = 7000000;
                        break;
                    default:  // BW capture 2,75Mhz, fs = 8 x 2,75Mhz= 22Mhz max ADC sampling) and others.
                              //  We tested also 9Mhz FPB stightly too much noise floor, better 8Mhz
                        anti_alias_baseband_bandwidth_filter = 8000000;
                }
                if( record_view )
                    record_view->set_sampling_rate(sampling_rate);
                receiver_model.set_sampling_rate(sampling_rate);
                receiver_model.set_baseband_bandwidth(anti_alias_baseband_bandwidth_filter);
            };
            text_ctcss.set("        ");
            break;
        default:
            break;
    }
    if (new_mod != SPEC_MODULATION && record_view )
        record_view->set_sampling_rate(recording_sampling_rate);

    field_mode.set_selected_index(new_mod);
    field_mode.on_change = [this](size_t, OptionsField::value_t v) {
        if (v != -1) {
            change_mode(v);
        }
    };
}
#endif
