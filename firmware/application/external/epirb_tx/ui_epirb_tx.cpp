/*
 * Copyright (C) 2026 Frederic BORRY - ADRASEC 31
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

#include "ui_epirb_tx.hpp"

#include "tonesets.hpp"
#include "portapack.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "file_reader.hpp"
#include "file_path.hpp"
#include "ui_geomap.hpp"
#include "ui_alphanum.hpp"

#include <cstring>
#include <stdio.h>

#include "beacon.hpp"

using namespace portapack;

namespace ui::external_app::epirb_tx {

void EPIRBTXAppView::focus() {
    button_tx.focus();
}

EPIRBTXAppView::~EPIRBTXAppView() {
    // Restore original frequency before leaving
    transmitter_model.set_target_frequency(original_frequency);
    transmitter_model.disable();
    baseband::shutdown();
}

/**
 * Conversion from hex char to half byte
 */
static uint8_t hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

/**
 * Convert from hex chars to uint8_t
 */
static uint8_t hexToByte(char high, char low) {
    return (hexval(high) << 4) | hexval(low);
}

std::string EPIRBTXAppView::frame_to_hex_string_range(int offset_bytes, int count_bytes) {
    return beacon_to_hex_string_range(epirb_tx_message.data, offset_bytes, count_bytes);
}

void EPIRBTXAppView::generate_frame(BeaconParams params) {
    if (format_sgb) {
        uint32_t elapsed_s = sgb_start_time > 0 ? (chTimeNow() - sgb_start_time) / 1000 : 0;
        epirb_tx_message.data_len = generate_sgb_beacon(epirb_tx_message.data, params, elapsed_s);
    } else {
        epirb_tx_message.data_len = generate_beacon(epirb_tx_message.data, params);
    }
}

void EPIRBTXAppView::on_timer() {
    // Timer is called on display refresh
    if (loop) {
        if (loop_enabled) {
            // chTimeNow() returns milliseconds on our version of ChibiOS / Hardware
            auto now = chTimeNow();
            auto elapsed = ((now - last_frame_time) / 1000);
            std::string timeout = to_string_dec_uint((uint32_t)(delay > elapsed ? delay - elapsed : 0));
            if (timeout != text_timeout.get()) {
                // Update timeout text every seconds
                text_timeout.set(timeout);
            }
            if (now > (last_frame_time + (delay * 1000))) {
                if (mode_file && slideshow_enabled) {
                    // Move on to next frame
                    selected_beacon++;
                    if (selected_beacon >= beacons.size()) selected_beacon = 0;
                    // Update selection
                    file_mode_ui->options_frame.set_selected_index(selected_beacon);
                }
                // Send a new frame after the delay
                start_tx();
            }
        } else {
            // Stop send loop
            loop = false;
        }
    }
}

void EPIRBTXAppView::update_bpsk_frequency() {
    bool was_transmitting = false;
    if (transmitting && (am_enabled || transmitting_bpsk)) {
        // We need to stop transmission before changing frequency
        transmitter_model.disable();
        was_transmitting = true;
    }
    transmitter_model.set_target_frequency(bpsk_frequency);
    // Update displayed frequency
    tx_view.on_show();
    if (was_transmitting) {
        // Start over
        start_tx();
    }
}

void EPIRBTXAppView::update_am_transmission() {
    if (am_enabled && transmitting && !transmitting_bpsk) {
        // Start am transmission
        // Restore am frequency
        epirb_tx_message.mode_406 = false;
        transmitter_model.set_target_frequency(am_frequency);
        // Send config to baseband
        baseband::set_epirb_tx_config(epirb_tx_message);
        // Start transmitting
        transmitter_model.enable();
    } else if (transmitting && !transmitting_bpsk) {
        // Stop am transmission
        transmitter_model.disable();
        tx_view.set_transmitting(false);
    }
}

void EPIRBTXAppView::update_frame(bool updateConfig) {
    if (mode_file) {
        // In file mode, currently selected beacon has changed => load the new one
        Beacon& beacon = beacons[selected_beacon];
        // Set desciption
        file_mode_ui->text_description.set(beacon.description.substr(0, max_text_width_ext));
        file_mode_ui->text_description_end.set(beacon.description.size() > max_text_width_ext ? "-" + beacon.description.substr(max_text_width_ext, max_text_width_ext + max_text_width_ext - 1) : "");
        // Udapte frame content on display
        bool is_fgb = beacon.frame.size() <= BEACON_HEXA_SIZE_FGB;
        if(is_fgb) {
            text_frame.set(beacon.frame.substr(0, BEACON_HEXA_SPLIT_FGB));
            text_frame_end.set(beacon.frame.size() > BEACON_HEXA_SPLIT_FGB ? beacon.frame.substr(BEACON_HEXA_SPLIT_FGB, BEACON_HEXA_SPLIT_FGB) : "");
            text_frame_sgb_end.set("");
        } else {
            text_frame.set(" "+beacon.frame.substr(1, BEACON_HEXA_SPLIT_SGB-1));
            text_frame_end.set(beacon.frame.size() > BEACON_HEXA_SPLIT_SGB ? beacon.frame.substr(BEACON_HEXA_SPLIT_SGB, BEACON_HEXA_SPLIT_SGB) : "");
            text_frame_sgb_end.set(beacon.frame.size() > 2*BEACON_HEXA_SPLIT_SGB ? beacon.frame.substr(2*BEACON_HEXA_SPLIT_SGB, BEACON_HEXA_SPLIT_SGB) : "");;
        }
        // Prepare tx configuration
        epirb_tx_message.data_len = std::min<size_t>((beacon.frame.size() / 2), BEACON_SIZE_SGB);
        for (uint8_t i = 0; i < epirb_tx_message.data_len; i++) {
            epirb_tx_message.data[i] = hexToByte(
                beacon.frame[2 * i],
                beacon.frame[2 * i + 1]);
        }
    } else {
        // In manual mode, generate frame content for current beacon params
        generate_frame(beacon_params);
        // Update frame content on display
        if (format_sgb) {
            // SGB: 32 bytes across 3 lines (BEACON_HEXA_SPLIT_SGB = 22 hex chars = 11 bytes)
            // Remove first nibble and replace it with a space to match COSPAS hexadecimal representation specification
            text_frame.set(" "+frame_to_hex_string_range(0, BEACON_HEXA_SPLIT_SGB / 2).substr(1));
            text_frame_end.set(frame_to_hex_string_range(BEACON_HEXA_SPLIT_SGB  / 2, BEACON_HEXA_SPLIT_SGB / 2));
            text_frame_sgb_end.set(frame_to_hex_string_range(BEACON_HEXA_SPLIT_SGB, (BEACON_HEXA_SPLIT_SGB / 2)-1));
        } else {
            text_frame.set(frame_to_hex_string_range(0, BEACON_HEXA_SPLIT_FGB / 2));
            text_frame_end.set(frame_to_hex_string_range(BEACON_HEXA_SPLIT_FGB / 2, BEACON_HEXA_SPLIT_FGB / 2));
            text_frame_sgb_end.set("");
        }
    }
    if (updateConfig && send_on_change && loop) {
        // Need to update config / send new beacon
        if (am_enabled) {
            // Already transmitting => update config
            last_frame_time = chTimeNow();
            update_config();
        } else {
            // Not yet transmitting => start tx
            start_tx();
        }
    }
}

void EPIRBTXAppView::update_config() {
    if (!epirb_tx_message.mode_406) {
        // Previously in AM mode => restore bpsk frequency
        transmitter_model.set_target_frequency(bpsk_frequency);
        // Update displayed frequency
        tx_view.on_show();
    }
    // Set mode to bpsk
    epirb_tx_message.mode_406 = true;
    transmitting_bpsk = true;
    // Set pre/post count
    epirb_tx_message.pre_count = (160 * TONES_SAMPLERATE) / 1000;   // 160 ms carrier (COSPAS spec.)
    epirb_tx_message.post_count = (100 * TONES_SAMPLERATE) / 1000;  // 100 ms
    // Send config to baseband
    baseband::set_epirb_tx_config(epirb_tx_message);
}

void EPIRBTXAppView::set_tx_button_state(bool active) {
    button_tx.set_text(active ? "START" : "STOP");
    button_tx.set_style(active ? &style_tx_start : &style_tx_stop);
}

void EPIRBTXAppView::start_tx() {
    if (format_sgb && !mode_file) {
        // Track start time for the SGB rotating field elapsed-time counter
        if (!transmitting) sgb_start_time = chTimeNow();
        // update_frame regenerates the SGB frame with current elapsed time and refreshes display
        update_frame(false);
        set_dirty();
    }
    last_frame_time = chTimeNow();
    update_config();
    loop = loop_enabled;
    transmitter_model.enable();
    tx_view.set_transmitting(true);
    set_tx_button_state(false);
    transmitting = true;
}

void EPIRBTXAppView::stop_tx() {
    loop = false;
    transmitter_model.disable();
    tx_view.set_transmitting(false);
    set_tx_button_state(true);
    transmitting = false;
}

void EPIRBTXAppView::on_tx_progress(const uint32_t progress, const bool done) {
    (void)progress;
    if (done) {
        transmitting_bpsk = false;
        if (am_enabled) {
            // BPSK frame sent, switch back to 121.5 AM signal
            epirb_tx_message.mode_406 = false;
            // Start am transmission
            update_am_transmission();
        } else {
            // End of BPSK frame
            transmitter_model.disable();
            tx_view.set_transmitting(false);
            if (!loop) {
                // Not looping => update transmission state
                set_tx_button_state(true);
                transmitting = false;
            }
        }
    }
}

void EPIRBTXAppView::update_location(bool updateLocatorField) {
    locator = beacon_params.location.locator;
    if (updateLocatorField) text_field_beacon_locator.set_text(beacon_params.location.locator);
    text_beacon_latitude_value.set(to_latitude_string(beacon_params.location));
    text_beacon_longitude_value.set(to_longitude_string(beacon_params.location));
}

void EPIRBTXAppView::update_mode() {
    // Hide / show widgets for file mode or manual mode
    using option_t = std::pair<std::string, int32_t>;
    using options_t = std::vector<option_t>;
    if (mode_file) {
        // Load available beacons from BEACONS.TXT files (or default).
        load_beacons();
        if (!file_mode_ui) {
            file_mode_ui = std::make_unique<FileModeWidgets>();
            // Setup options_frame content with loaded beacons
            options_t entries;
            for (const auto& beacon : beacons)
                entries.emplace_back(beacon.title, entries.size());

            file_mode_ui->options_frame.set_options(entries);
            if (selected_beacon >= beacons.size())
                // BEACONS.TXT file has changed since last launch, default index to 0
                selected_beacon = 0;
            file_mode_ui->options_frame.set_selected_index(selected_beacon);

            file_mode_ui->options_frame.on_change = [this](size_t index, OptionsField::value_t) {
                selected_beacon = index;
                update_frame();
                set_dirty();
            };
            file_mode_ui->text_beacon.set_style(Theme::getInstance()->fg_light);
            file_mode_ui->text_description_label.set_style(Theme::getInstance()->fg_light);
            add_child(&file_mode_ui->text_beacon);
            add_child(&file_mode_ui->options_frame);
            add_child(&file_mode_ui->text_description_label);
            add_child(&file_mode_ui->text_description);
            add_child(&file_mode_ui->text_description_end);
            add_child(&file_mode_ui->checkbox_slideshow);
            // Restore settings
            file_mode_ui->checkbox_slideshow.set_value(slideshow_enabled);
            // Add callback
            file_mode_ui->checkbox_slideshow.on_select = [this](Checkbox&, bool v) {
                slideshow_enabled = v;
            };
        }
    } else {
        // Clear beacons to save ram for Map display and Locatior editor
        if (file_mode_ui) {
            remove_child(&file_mode_ui->text_beacon);
            remove_child(&file_mode_ui->options_frame);
            remove_child(&file_mode_ui->text_description_label);
            remove_child(&file_mode_ui->text_description);
            remove_child(&file_mode_ui->text_description_end);
            remove_child(&file_mode_ui->checkbox_slideshow);
            file_mode_ui.reset();
        }
        beacons.clear();
        beacons.shrink_to_fit();
    }
    text_beacon_type.hidden(mode_file);
    text_beacon_country.hidden(mode_file);
    checkbox_beacon_internal.hidden(mode_file);
    text_beacon_locator.hidden(mode_file);
    text_beacon_latitude.hidden(mode_file);
    text_beacon_latitude_value.hidden(mode_file);
    text_beacon_longitude.hidden(mode_file);
    text_beacon_longitude_value.hidden(mode_file);
    button_mangps.hidden(mode_file);
    options_beacon_type.hidden(mode_file);
    options_beacon_protocol.hidden(mode_file);
    options_beacon_country.hidden(mode_file);
    options_format.hidden(mode_file);
    text_field_beacon_locator.hidden(mode_file);
}

EPIRBTXAppView::EPIRBTXAppView(
    NavigationView& nav)
    : nav_{nav} {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&labels,
                  &options_mode,
                  &text_beacon_type,
                  &options_beacon_type,
                  &options_beacon_protocol,
                  &options_format,
                  &text_beacon_country,
                  &options_beacon_country,
                  &checkbox_beacon_internal,
                  &text_beacon_locator,
                  &text_beacon_latitude,
                  &text_beacon_latitude_value,
                  &text_beacon_longitude,
                  &text_beacon_longitude_value,
                  &button_mangps,
                  &text_field_beacon_locator,
                  &text_frame,
                  &text_frame_end,
                  &text_frame_sgb_end,
                  &text_timeout,
                  &checkbox_loop,
                  &field_delay,
                  &button_tx,
                  &checkbox_am,
                  &field_am_frequency,
                  &checkbox_send_on_change,
                  &options_am_channel,
                  &options_bpsk_channel,
                  &tx_view});

    text_beacon_type.set_style(Theme::getInstance()->fg_light);
    text_beacon_country.set_style(Theme::getInstance()->fg_light);
    text_beacon_locator.set_style(Theme::getInstance()->fg_light);
    text_beacon_latitude.set_style(Theme::getInstance()->fg_light);
    text_beacon_longitude.set_style(Theme::getInstance()->fg_light);

    // Restore settings
    checkbox_am.set_value(am_enabled);
    checkbox_loop.set_value(loop_enabled);
    checkbox_send_on_change.set_value(send_on_change);
    options_mode.set_by_value(!mode_file);
    transmitter_model.set_target_frequency(bpsk_frequency);
    field_am_frequency.set_value(am_frequency);
    options_am_channel.set_by_value(am_channel);
    options_am_channel.set_style((am_channel == (uint8_t)AmChannel::REAL) ? Theme::getInstance()->fg_red : Theme::getInstance()->bg_darkest);
    manual_am_frequency = am_frequency;
    options_bpsk_channel.set_by_value(bpsk_channel);
    manual_bpsk_frequency = bpsk_frequency;
    field_delay.set_value(delay);
    options_beacon_type.set_by_value(beacon_type);
    options_beacon_protocol.set_by_value(beacon_protocol);
    options_beacon_country.set_by_value(beacon_country);
    checkbox_beacon_internal.set_value(beacon_internal);
    beacon_params.type = (BeaconType)beacon_type;
    beacon_params.has_121_5 = am_enabled;
    beacon_params.location.locator = locator;
    beacon_params.is_internal = beacon_internal;
    init_from_locator(beacon_params.location);
    update_mode();
    update_location();
    options_format.set_by_value(format_sgb ? 1 : 0);

    options_mode.on_change = [this](size_t, OptionsField::value_t value) {
        mode_file = (((BeaconMode)value) == BeaconMode::FILE);
        update_mode();
        update_frame();
        set_dirty();
    };

    options_beacon_type.on_change = [this](size_t, OptionsField::value_t value) {
        beacon_params.type = (BeaconType)value;
        beacon_type = value;
        update_frame();
        set_dirty();
    };

    options_beacon_protocol.on_change = [this](size_t, OptionsField::value_t value) {
        beacon_params.protocol = (BeaconProtocol)value;
        beacon_protocol = value;
        update_frame();
        set_dirty();
    };

    options_beacon_country.on_change = [this](size_t, OptionsField::value_t v) {
        beacon_params.country = v;
        beacon_country = v;
        update_frame();
        set_dirty();
    };

    options_format.on_change = [this](size_t, OptionsField::value_t v) {
        format_sgb = (v == 1);
        sgb_start_time = 0;  // reset elapsed counter when format changes
        update_frame();
        set_dirty();
    };

    options_am_channel.on_change = [this](size_t, OptionsField::value_t v) {
        bool is_real = false;
        switch ((AmChannel)v) {
            case AmChannel::REAL:
                is_real = true;
                am_frequency = AM_REAL_FREQUENCY;
                break;
            case AmChannel::MANUAL:
                am_frequency = manual_am_frequency;
                break;
            default:
                v = (uint8_t)AmChannel::TEST;
                // fallthrough
            case AmChannel::TEST:
                am_frequency = AM_TEST_FREQUENCY;
                break;
        }
        // Actual frequency change will be done by field_am_frequency.on_change()
        field_am_frequency.set_value(am_frequency);
        am_channel = v;
        options_am_channel.set_style(is_real ? Theme::getInstance()->fg_red : Theme::getInstance()->bg_darkest);
        set_dirty();
    };

    options_bpsk_channel.on_change = [this](size_t, OptionsField::value_t v) {
        switch ((BpskChannel)v) {
            case BpskChannel::MANUAL:
                bpsk_frequency = manual_bpsk_frequency;
                break;
            case BpskChannel::B:
                bpsk_frequency = BPSK_FREQUENCY_B;
                break;
            case BpskChannel::C:
                bpsk_frequency = BPSK_FREQUENCY_C;
                break;
            case BpskChannel::F:
                bpsk_frequency = BPSK_FREQUENCY_F;
                break;
            case BpskChannel::G:
                bpsk_frequency = BPSK_FREQUENCY_G;
                break;
            case BpskChannel::J:
                bpsk_frequency = BPSK_FREQUENCY_J;
                break;
            case BpskChannel::K:
                bpsk_frequency = BPSK_FREQUENCY_K;
                break;
            case BpskChannel::N:
                bpsk_frequency = BPSK_FREQUENCY_N;
                break;
            case BpskChannel::O:
                bpsk_frequency = BPSK_FREQUENCY_O;
                break;
            case BpskChannel::SGB:
                bpsk_frequency = BPSK_FREQUENCY_SGB;
                break;
            default:
                v = (uint8_t)BpskChannel::HAM;
                // fallthrough
            case BpskChannel::HAM:
                bpsk_frequency = BPSK_FREQUENCY_HAM;
                break;
        }
        bpsk_channel = v;
        update_bpsk_frequency();
        set_dirty();
    };

    checkbox_beacon_internal.on_select = [this](Checkbox&, bool v) {
        beacon_internal = v;
        beacon_params.is_internal = v;
        update_frame();
        set_dirty();
    };

    text_field_beacon_locator.on_select = [this](TextField&) mutable {
        auto te_view = nav_.push<AlphanumView>(locator, 10, ENTER_KEYBOARD_MODE_ALPHA);
        te_view->on_changed = [this](std::string& value) {
            beacon_params.location.locator = value;
            init_from_locator(beacon_params.location);
            update_location();
            update_frame();
            set_dirty();
        };
    };

    button_mangps.on_select = [this](Button&) {
        nav_.push<GeoMapView>(
            0,
            GeoPos::alt_unit::METERS,
            GeoPos::spd_unit::HIDDEN,
            beacon_params.location.latitude,
            beacon_params.location.longitude,
            [this](int32_t, float lat, float lon, int32_t) {
                beacon_params.location.latitude = lat;
                beacon_params.location.longitude = lon;
                init_from_decimal(beacon_params.location);
                // Update locator field
                update_location();
                update_frame();
                set_dirty();
            });
    };

    field_am_frequency.on_change = [this](rf::Frequency freq) {
        am_frequency = freq;
        if (transmitting && !transmitting_bpsk && am_enabled)
            // Update transmitter frequency
            transmitter_model.set_target_frequency(am_frequency);
    };

    // Init frame content / baseband param with currently setup beacon
    update_frame(false);

    checkbox_loop.on_select = [this](Checkbox&, bool v) {
        loop_enabled = v;
    };

    field_delay.on_change = [this](int32_t v) {
        delay = v;
    };

    checkbox_am.on_select = [this](Checkbox&, bool v) {
        beacon_params.has_121_5 = v;
        am_enabled = v;
        update_am_transmission();
        // We update the additional location device data in the frame based on this
        if (!mode_file) update_frame(false);
    };

    // AM frequency field edit
    field_am_frequency.on_edit = [this]() {
        auto new_view = nav_.push<FrequencyKeypadView>(field_am_frequency.value());
        new_view->on_changed = [this](rf::Frequency f) {
            switch (f) {
                case AM_REAL_FREQUENCY:
                    am_channel = (uint8_t)AmChannel::REAL;
                    break;
                case AM_TEST_FREQUENCY:
                    am_channel = (uint8_t)AmChannel::TEST;
                    break;
                default:
                    manual_am_frequency = f;
                    am_channel = (uint8_t)AmChannel::MANUAL;
                    break;
            }
            // Actual frequency change will be done by options_am_channel.on_change()
            options_am_channel.set_by_value(am_channel);
            set_dirty();
        };
    };

    checkbox_send_on_change.on_select = [this](Checkbox&, bool v) {
        send_on_change = v;
    };

    tx_view.on_edit_frequency = [this]() {
        auto new_view = nav_.push<FrequencyKeypadView>(transmitter_model.target_frequency());
        new_view->on_changed = [this](rf::Frequency f) {
            bpsk_frequency = f;
            switch (f) {
                case BPSK_FREQUENCY_HAM:
                    bpsk_channel = (uint8_t)BpskChannel::HAM;
                    break;
                case BPSK_FREQUENCY_B:
                    bpsk_channel = (uint8_t)BpskChannel::B;
                    break;
                case BPSK_FREQUENCY_C:
                    bpsk_channel = (uint8_t)BpskChannel::C;
                    break;
                case BPSK_FREQUENCY_F:
                    bpsk_channel = (uint8_t)BpskChannel::F;
                    break;
                case BPSK_FREQUENCY_G:
                    bpsk_channel = (uint8_t)BpskChannel::G;
                    break;
                case BPSK_FREQUENCY_J:
                    bpsk_channel = (uint8_t)BpskChannel::J;
                    break;
                case BPSK_FREQUENCY_K:
                    bpsk_channel = (uint8_t)BpskChannel::K;
                    break;
                case BPSK_FREQUENCY_N:
                    bpsk_channel = (uint8_t)BpskChannel::N;
                    break;
                case BPSK_FREQUENCY_O:
                    bpsk_channel = (uint8_t)BpskChannel::O;
                    break;
                case BPSK_FREQUENCY_SGB:
                    bpsk_channel = (uint8_t)BpskChannel::SGB;
                    break;
                default:
                    bpsk_channel = (uint8_t)BpskChannel::MANUAL;
                    manual_bpsk_frequency = bpsk_frequency;
                    break;
            }
            // Actual frequency change will be done by options_bpsk_channel.on_change()
            options_bpsk_channel.set_by_value(bpsk_channel);
            set_dirty();
        };
    };

    tx_view.on_start = [this]() {
        start_tx();
    };

    tx_view.on_stop = [this]() {
        stop_tx();
    };

    button_tx.on_select = [this](Button&) {
        if (!transmitting)
            start_tx();
        else
            stop_tx();
    };
}

/**
 * Load beacons from /EPIRB/BEACONS.TXT file on sd card
 */
void EPIRBTXAppView::load_beacons() {
    File beacons_file;
    auto error = beacons_file.open(epirb_dir / u"BEACONS.TXT");
    beacons.clear();

    if (!error) {
        // BEACONS.TXT file exists
        auto reader = FileLineReader(beacons_file);
        for (const auto& line : reader) {
            // Skip comment lines
            if (line.length() == 0 || line[0] == '#')
                continue;

            // Split line with ';' separator
            auto cols = split_string(line, ';');
            if (cols.size() != 3)
                continue;

            // Read current beacon
            Beacon beacon{};
            beacon.title = trim(cols[0]);
            beacon.description = trim(cols[1]);
            // 1G uses up to 36 hex chars, 2G uses 64 hex chars (250 bits rounded to 32 bytes).
            beacon.frame = trim(cols[2]).substr(0, BEACON_HEXA_SIZE_SGB );
            size_t size = beacon.frame.size();
            if (size <= 0)
                continue;  // Invalid line.
            if((size % 2) != 0)
            {
                beacon.frame = "0" + beacon.frame;  // Pad with leading 0 to make it even length
            }
            // Beacon is valid, add it to the list
            beacons.emplace_back(std::move(beacon));
        }
    }
    if (beacons.empty()) {
        // No beacons file or empty flile: just add default beacon
        beacons.push_back(default_beacon);
    }
}

}  // namespace ui::external_app::epirb_tx