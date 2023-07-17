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

#include "ui_mictx.hpp"

#include "audio.hpp"
#include "baseband_api.hpp"
#include "irq_controls.hpp"
#include "ui_freqman.hpp"
#include "portapack_hal.hpp"
#include "string_format.hpp"
#include "tonesets.hpp"
#include "ui_tone_key.hpp"
#include "wm8731.hpp"

#include <cstring>

using namespace tonekey;
using namespace portapack;
using wolfson::wm8731::WM8731;

WM8731 audio_codec_wm8731{i2c0, 0x1a};

namespace ui {

void MicTXView::focus() {
    switch (focused_ui) {
        case 0:
            field_frequency.focus();
            break;
        case 1:
            field_rxfrequency.focus();
            break;
        default:
            field_va.focus();
            break;
    }
}

void MicTXView::update_vumeter() {
    vumeter.set_value(audio_level);
}

void MicTXView::on_tx_progress(const bool done) {
    // Roger beep played, stop transmitting
    if (done)
        set_tx(false);
}

void MicTXView::configure_baseband() {
    // TODO: Can this use the transmitter model instead?
    baseband::set_audiotx_config(
        sampling_rate / 20,  // Update vu-meter at 20Hz
        transmitting ? transmitter_model.channel_bandwidth() : 0,
        mic_gain,
        shift_bits_s16,  // to be used in dsp_modulate
        TONES_F2D(tone_key_frequency(tone_key_index), sampling_rate),
        enable_am,
        enable_dsb,
        enable_usb,
        enable_lsb);
}

void MicTXView::set_tx(bool enable) {
    if (enable) {
        if (rx_enabled)      // If audio RX is enabled
            rxaudio(false);  // Then turn off audio RX
        transmitting = true;
        configure_baseband();
        transmitter_model.set_target_frequency(tx_frequency);  // Now, no need: transmitter_model.set_tx_gain(tx_gain), nor (rf_amp);
        transmitter_model.enable();
        portapack::pin_i2s0_rx_sda.mode(3);  // This is already done in audio::init but gets changed by the CPLD overlay reprogramming
    } else {
        if (transmitting && rogerbeep_enabled) {
            baseband::request_beep();  // Transmit the roger beep
            transmitting = false;      // And flag the end of the transmission so ...
        } else {                       // (if roger beep was enabled, this will be executed after the beep ends transmitting.
            transmitting = false;
            configure_baseband();
            transmitter_model.disable();
            if (rx_enabled)     // If audio RX is enabled and we've been transmitting
                rxaudio(true);  // Turn back on audio RX
        }
    }
}

void MicTXView::do_timing() {
    if (va_enabled) {
        if (!transmitting) {
            // Attack
            if (audio_level >= va_level) {
                if ((attack_timer >> 8) >= attack_ms) {
                    decay_timer = 0;
                    attack_timer = 0;
                    set_tx(true);
                } else {
                    attack_timer += lcd_frame_duration;
                }
            } else {
                attack_timer = 0;
            }
        } else {
            // Decay
            if (audio_level < va_level) {
                if ((decay_timer >> 8) >= decay_ms) {
                    decay_timer = 0;
                    attack_timer = 0;
                    set_tx(false);
                } else {
                    decay_timer += lcd_frame_duration;
                }
            } else {
                decay_timer = 0;
            }
        }
    } else {
        // Check for PTT release
        const auto switches_state = get_switches_state();
        if (!switches_state[4] && transmitting && !button_touch)  // Select button
            set_tx(false);
    }
}

void MicTXView::rxaudio(bool is_on) {
    if (is_on) {
        audio::input::stop();
        baseband::shutdown();

        if (enable_am || enable_usb || enable_lsb || enable_dsb) {  // "NFM/FM", 0, " WFM  ", 1,  "  AM  ", 2, " USB  ", 3, " LSB  ", 4, " DSB-SC", 5
            baseband::run_image(portapack::spi_flash::image_tag_am_audio);
            receiver_model.set_modulation(ReceiverModel::Mode::AMAudio);                 // that AM demodulation engine is common to all Amplitude mod : AM/USB/LSB/DSB (2,3,4,5)
            if (options_mode.selected_index() < 5)                                       // We will be called here with 2,3,4,5. We treat here demod. filter 2,3,4; (excluding DSB-C case (5) it is treated more down).
                receiver_model.set_am_configuration(options_mode.selected_index() - 1);  // selecting proper filter(2,3,4). 2-1=1=>6k-AM(1), 3-1=2=>+3k-USB(2), 4-1=3=>-3K-LSB(3),
        } else {                                                                         // We are in NFM/FM or WFM (NFM BW:8k5 or 11k / FM BW 16k / WFM BW:200k)

            if (enable_wfm) {  // WFM, BW 200Khz aprox, or the two new addional BW filters (180k, 40k)
                baseband::run_image(portapack::spi_flash::image_tag_wfm_audio);
                receiver_model.set_modulation(ReceiverModel::Mode::WidebandFMAudio);
                // receiver_model.set_wfm_configuration(n);  // it is called above, depending user's selection (200k, 180k, 0k).
            } else {  // NFM BW:8k5 or 11k / FM BW 16k
                baseband::run_image(portapack::spi_flash::image_tag_nfm_audio);
                receiver_model.set_modulation(ReceiverModel::Mode::NarrowbandFMAudio);  //
                                                                                        // receiver_model.set_nbfm_configuration(n); is called above, depending user's selection (8k5, 11k, 16k).
            }
        }

        if (bool_same_F_tx_rx_enabled)                          // when stop TX, define to which freq RX we return
            receiver_model.set_target_frequency(tx_frequency);  // Update freq also for RX = TX
        else
            receiver_model.set_target_frequency(rx_frequency);  // Now with separate freq controls!
        receiver_model.set_lna(rx_lna);
        receiver_model.set_vga(rx_vga);
        receiver_model.set_rf_amp(rx_amp);
        receiver_model.enable();
        audio::output::start();
    } else {                                                                    // These incredibly convoluted steps are required for the vumeter to reappear when stopping RX.
        receiver_model.set_modulation(ReceiverModel::Mode::NarrowbandFMAudio);  // This fixes something with AM RX...
        receiver_model.disable();
        baseband::shutdown();

        baseband::run_image(portapack::spi_flash::image_tag_mic_tx);
        audio::output::stop();
        audio::input::start(ak4951_alc_and_wm8731_boost_GUI);  // When detected AK4951 => set up ALC mode; when detected WM8731 => set up mic_boost ON/OFF.
        portapack::pin_i2s0_rx_sda.mode(3);
        configure_baseband();
    }
}

void MicTXView::set_ptt_visibility(bool v) {
    tx_button.hidden(!v);
}

MicTXView::MicTXView(
    NavigationView& nav) {
    portapack::pin_i2s0_rx_sda.mode(3);  // This is already done in audio::init but gets changed by the CPLD overlay reprogramming

    baseband::run_image(portapack::spi_flash::image_tag_mic_tx);

    if (audio::debug::codec_name() == "WM8731") {
        add_children({&labels_WM8731,  // we have audio codec WM8731, same MIC menu as original.
                      &vumeter,
                      &options_gain,  // MIC GAIN float factor on the GUI.
                      &options_wm8731_boost_mode,
                      //		&check_va,
                      &field_va,
                      &field_va_level,
                      &field_va_attack,
                      &field_va_decay,
                      &field_bw,
                      &tx_view,  // it already integrates previous rfgain, rfamp.
                      &options_mode,
                      &field_frequency,
                      &options_tone_key,
                      &check_rogerbeep,
                      &check_common_freq_tx_rx,  // added to handle common or separate freq- TX/RX
                      &check_rxactive,
                      &field_volume,
                      &field_rxbw,
                      &field_squelch,
                      &field_rxfrequency,
                      &field_rxlna,
                      &field_rxvga,
                      &field_rxamp,
                      &tx_button});

    } else {
        add_children({&labels_AK4951,  // we have audio codec AK4951, enable Automatic Level Control
                      &vumeter,
                      &options_gain,
                      &options_ak4951_alc_mode,
                      //		&check_va,
                      &field_va,
                      &field_va_level,
                      &field_va_attack,
                      &field_va_decay,
                      &field_bw,
                      &tx_view,  // it already integrates previous rfgain, rfamp.
                      &options_mode,
                      &field_frequency,
                      &options_tone_key,
                      &check_rogerbeep,
                      &check_common_freq_tx_rx,  // added to handle common or separate freq- TX/RX
                      &check_rxactive,
                      &field_volume,
                      &field_rxbw,
                      &field_squelch,
                      &field_rxfrequency,
                      &field_rxlna,
                      &field_rxvga,
                      &field_rxamp,
                      &tx_button});
    }

    tone_keys_populate(options_tone_key);
    options_tone_key.on_change = [this](size_t i, int32_t) {
        tone_key_index = i;
    };
    options_tone_key.set_selected_index(0);

    options_gain.on_change = [this](size_t, int32_t v) {
        mic_gain = v / 10.0;
        configure_baseband();
    };
    options_gain.set_selected_index(1);  // x1.0 preselected default.

    if (audio::debug::codec_name() == "WM8731") {
        options_wm8731_boost_mode.on_change = [this](size_t, int8_t v) {
            switch (v) {
                case 0:                  // +12 dB’s respect reference level orig fw 1.5.x fw FM : when +20dB's boost ON) and shift bits (>>8),
                    shift_bits_s16 = 6;  // now mic-boost on (+20dBs) and shift bits (>>6), +20+12=32 dB’s (orig fw +20 dBs+ 0dBs)=> +12dB's respect ref.
                    break;
                case 1:                  // +06 dB’s reference level, (when +20dB's boost ON)
                    shift_bits_s16 = 7;  // now mic-boost on (+20dBs) and shift bits (>>7), +20+06=26 dB’s (orig fw +20 dBs+ 0dBs) => +06dB's respect ref.
                    break;
                case 2:
                    shift_bits_s16 = 4;  // +04 dB’s respect ref level, (when +20dB's boost OFF)
                    break;               // now mic-boost off (+00dBs) shift bits (4) (+0+24dB's)=24 dBs => +04dB's respect ref.
                case 3:
                    shift_bits_s16 = 5;  // -02 dB’s respect ref level, (when +20dB's boost OFF)
                    break;               // now mic-boost off (+00dBs) shift bits (5) (+0+18dB's)=18 dBs => -02dB's respect ref.
                case 4:
                    shift_bits_s16 = 6;  // -08 dB’s respect ref level, (when +20dB's boost OFF)
                    break;               // now mic-boost off (+00dBs) shift bits (6) (+0+12dB's)=12 dBs => -08dB's respect ref.
            }
            ak4951_alc_and_wm8731_boost_GUI = v;                   // 0..4, WM8731_boost dB's options, (combination boost on/off, and effective gain in captured data >>x)
            audio::input::start(ak4951_alc_and_wm8731_boost_GUI);  // Detected (WM8731), set up the proper wm_boost on/off, 0..4 (0,1) boost_on, (2,3,4) boost_off
            configure_baseband();                                  // to update in real time, sending msg, var-parameters >>shift_bits FM msg, to audio_tx from M0 to M4 Proc -
        };
        options_wm8731_boost_mode.set_selected_index(3);  // preset GUI index 3 as default WM -> -02 dB's.
    } else {
        shift_bits_s16 = 8;  // Initialized default fixed >>8_FM for FM tx mod, shift audio data for AK4951, using top 8 bits s16 data (>>8)
        options_ak4951_alc_mode.on_change = [this](size_t, int8_t v) {
            ak4951_alc_and_wm8731_boost_GUI = v;                   // 0..11, AK4951 Mic -Automatic volume Level Control options,
            audio::input::start(ak4951_alc_and_wm8731_boost_GUI);  // Detected (AK4951) ==> Set up proper ALC mode from 0..11 options
            configure_baseband();                                  // sending fixed >>8_FM, var-parameters msg, to audiotx from this M0 to M4 process.
        };
    }

    // options_ak4951_alc_mode.set_selected_index(0);

    tx_frequency = transmitter_model.target_frequency();
    field_frequency.set_value(transmitter_model.target_frequency());
    field_frequency.set_step(receiver_model.frequency_step());
    field_frequency.on_change = [this](rf::Frequency f) {
        tx_frequency = f;
        if (!rx_enabled) {  // not activated receiver. just update freq TX
            transmitter_model.set_target_frequency(f);
        } else {                                         // activated receiver.
            if (bool_same_F_tx_rx_enabled)               // user selected common freq- TX = RX
                receiver_model.set_target_frequency(f);  // Update common freq also for RX
        }
    };
    field_frequency.on_edit = [this, &nav]() {
        focused_ui = 0;
        // TODO: Provide separate modal method/scheme?
        auto new_view = nav.push<FrequencyKeypadView>(tx_frequency);
        new_view->on_changed = [this](rf::Frequency f) {
            tx_frequency = f;
            if (!rx_enabled) {
                transmitter_model.set_target_frequency(f);
            } else {
                if (bool_same_F_tx_rx_enabled)
                    receiver_model.set_target_frequency(f);  // Update freq also for RX
            }
            this->field_frequency.set_value(f);
            set_dirty();
        };
    };

    field_bw.on_change = [this](uint32_t v) {
        transmitter_model.set_channel_bandwidth(v * 1000);
    };
    field_bw.set_value(10);  // pre-default first time, TX deviation FM for NFM / FM

    // now, no need direct update, field_rfgain, field_rfamp (it is done in ui_transmitter.cpp)

    options_mode.on_change = [this](size_t, int32_t v) {  // { "NFM/FM", 0 }, { " WFM  ", 1 }, { "AM", 2 }, { "USB", 3 }, { "LSB", 4 }, { "DSB", 5 }
        enable_am = false;
        enable_usb = false;
        enable_lsb = false;
        enable_dsb = false;
        enable_wfm = false;

        using option_t = std::pair<std::string, int32_t>;
        using options_t = std::vector<option_t>;
        options_t rxbw;  // Aux structure to change dynamically field_rxbw contents,

        switch (v) {
            case 0:  //{ "FM", 0 }
                enable_am = false;
                enable_usb = false;
                enable_lsb = false;
                enable_dsb = false;
                field_bw.set_value(10);  // pre-default deviation FM for WFM
                // field_bw.set_value(transmitter_model.channel_bandwidth() / 1000);
                // if (rx_enabled)
                rxaudio(rx_enabled);         // Update now if we have RX audio on
                options_tone_key.hidden(0);  // we are in FM mode, we should have active the Key-tones & CTCSS option.

                freqman_set_bandwidth_option(NFM_MODULATION, field_rxbw);  // restore dynamic field_rxbw value with NFM options from freqman_db.cpp
                field_rxbw.set_by_value(2);                                // // bw 16k (2) default
                field_rxbw.hidden(0);                                      // we are in FM mode, we need to allow the user set up of the RX NFM BW selection (8K5, 11K, 16K)
                field_bw.hidden(0);                                        // we are in FM mode, we need to allow FM deviation parameter, in non FM mode.
                break;
            case 1:  //{ "WFM", 1 }
                enable_am = false;
                enable_usb = false;
                enable_lsb = false;
                enable_dsb = false;
                enable_wfm = true;
                field_bw.set_value(75);  // pre-default deviation FM for WFM
                // field_bw.set_value(transmitter_model.channel_bandwidth() / 1000);
                // if (rx_enabled)
                rxaudio(rx_enabled);         // Update now if we have RX audio on
                options_tone_key.hidden(0);  // we are in WFM mode, we should have active the Key-tones & CTCSS option.

                freqman_set_bandwidth_option(WFM_MODULATION, field_rxbw);  // restore dynamic field_rxbw value with WFM options from freqman_db.cpp
                field_rxbw.set_by_value(0);                                // bw 200k (0) default
                field_rxbw.hidden(0);                                      // we are in WFM mode, we need to show to the user the selected BW WFM filter.
                field_bw.hidden(0);                                        // we are in WFM mode, we need to allow WFM deviation parameter, in non FM mode.
                break;
            case 2:  //{ "AM", 2 }
                enable_am = true;
                rxaudio(rx_enabled);                     // Update now if we have RX audio on
                options_tone_key.set_selected_index(0);  // we are NOT in FM mode, we reset the possible previous key-tones &CTCSS selection.
                set_dirty();                             // Refresh display
                options_tone_key.hidden(1);              // we hide that Key-tones & CTCSS input selecction, (no meaning in AM/DSB/SSB).

                rxbw.emplace_back("DSB 9k", 0);  // we offer in AM DSB two audio BW 9k / 6k.
                rxbw.emplace_back("DSB 6k", 1);
                field_rxbw.set_options(rxbw);  // store that aux GUI option to the field_rxbw.

                field_rxbw.hidden(0);       // we show fixed RX AM BW 6Khz
                field_bw.hidden(1);         // we hide the FM TX deviation parameter, in non FM mode.
                check_rogerbeep.hidden(0);  // make visible again the "rogerbeep" selection.
                break;
            case 3:  //{ "USB", 3 }
                enable_usb = true;
                rxaudio(rx_enabled);               // Update now if we have RX audio on
                check_rogerbeep.set_value(false);  // reset the possible activation of roger beep, because it is not compatible with SSB, by now.
                check_rogerbeep.hidden(1);         // hide that roger beep selection.

                rxbw.emplace_back("USB+3k", 0);  // locked a fixed option, to display it.
                field_rxbw.set_options(rxbw);    // store that aux GUI option to the field_rxbw.

                set_dirty();  // Refresh display
                break;
            case 4:  //{ "LSB", 4 }
                enable_lsb = true;
                rxaudio(rx_enabled);               // Update now if we have RX audio on
                check_rogerbeep.set_value(false);  // reset the possible activation of roger beep, because it is not compatible with SSB, by now.
                check_rogerbeep.hidden(1);         // hide that roger beep selection.

                rxbw.emplace_back("LSB-3k", 0);  // locked a fixed option, to display it.
                field_rxbw.set_options(rxbw);    // store that aux GUI option to the field_rxbw.

                set_dirty();  // Refresh display
                break;
            case 5:  //{ "DSB-SC", 5 }
                enable_dsb = true;
                rxaudio(rx_enabled);        // Update now if we have RX audio on
                check_rogerbeep.hidden(0);  // make visible again the "rogerbeep" selection.

                rxbw.emplace_back("USB+3k", 0);  // added dynamically two options (index 0,1) to that DSB-SC case to the field_rxbw value.
                rxbw.emplace_back("LSB-3k", 1);

                field_rxbw.set_options(rxbw);  // store that aux GUI option to the field_rxbw.

                break;
        }
        // configure_baseband();
    };

    /*
        check_va.on_select = [this](Checkbox&, bool v) {
                va_enabled = v;
                text_ptt.hidden(v);			//hide / show PTT text
                check_rxactive.hidden(v); 	//hide / show the RX AUDIO
                set_dirty();				//Refresh display
        };
        */
    field_va.set_selected_index(1);
    field_va.on_change = [this](size_t, int32_t v) {
        switch (v) {
            case 0:
                va_enabled = 0;
                this->set_ptt_visibility(0);
                check_rxactive.hidden(0);
                ptt_enabled = 0;
                break;
            case 1:
                va_enabled = 0;
                this->set_ptt_visibility(1);
                check_rxactive.hidden(0);
                ptt_enabled = 1;
                break;
            case 2:
                if (!rx_enabled) {
                    va_enabled = 1;
                    this->set_ptt_visibility(0);
                    check_rxactive.hidden(1);
                    ptt_enabled = 0;
                } else {
                    field_va.set_selected_index(1);
                }
                break;
        }
        set_dirty();
    };

    check_rogerbeep.on_select = [this](Checkbox&, bool v) {
        rogerbeep_enabled = v;
    };

    check_common_freq_tx_rx.on_select = [this](Checkbox&, bool v) {
        bool_same_F_tx_rx_enabled = v;
        field_rxfrequency.hidden(v);                                           // Hide or show separated freq RX field. (When no hide user can enter down indep. freq for RX)
        set_dirty();                                                           // Refresh GUI interface
        receiver_model.set_target_frequency(v ? tx_frequency : rx_frequency);  // To go to the proper tuned freq. when toggling it
    };

    field_va_level.on_change = [this](int32_t v) {
        va_level = v;
        vumeter.set_mark(v);
    };
    field_va_level.set_value(40);

    field_va_attack.on_change = [this](int32_t v) {
        attack_ms = v;
    };
    field_va_attack.set_value(500);

    field_va_decay.on_change = [this](int32_t v) {
        decay_ms = v;
    };
    field_va_decay.set_value(1000);

    check_rxactive.on_select = [this](Checkbox&, bool v) {
        //		vumeter.set_value(0);	//Start with a clean vumeter
        rx_enabled = v;
        //		check_va.hidden(v); 	//Hide or show voice activation
        rxaudio(v);   // Activate-Deactivate audio rx accordingly
        set_dirty();  // Refresh interface
    };

    field_rxbw.on_change = [this](size_t, int32_t v) {
        if (!(enable_am || enable_usb || enable_lsb || enable_dsb || enable_wfm)) {
            // In Previous fw versions, that nbfm_configuration(n) was done in any mode (FM/AM/SSB/DSB)...strictly speaking only need it in (NFM/FM)
            receiver_model.set_nbfm_configuration(v);    // we are in NFM/FM case, we need to select proper NFM/FM RX channel filter, NFM BW 8K5(0), NFM BW 11K(1), FM BW 16K (2)
        } else {                                         // we are not in NFM/FM mode. (we could be in any of the rest : AM /USB/LSB/DSB-SC)
            if (enable_am) {                             // we are in AM TX mode, we will allow both independent RX audio BW : AM 9K (9K00AE3 / AM 6K (6K00AE3). (In AM option v can be 0 (9k), 1 (6k)
                receiver_model.set_am_configuration(v);  // we are in AM TX mode, we need to select proper AM full path config AM-9K filter. 0+0 =>AM-9K(0), 0+1=1 =>AM-6K(1),
            }
            if (enable_dsb) {                                // we are in DSB-SC in TX mode, we will allow both independent RX SSB demodulation (USB / LSB side band). in that submenu, v is 0 (SSB1 USB) or 1 (SSB2 LSB)
                receiver_model.set_am_configuration(v + 2);  // we are in DSB-SC TX mode, we need to select proper SSB filter. 0+2 =>usb(2), 1+2=3 =>lsb(3),
            }
            if (enable_wfm) {
                receiver_model.set_wfm_configuration(v);  // we are in WFM case, we need to select proper WFB RX BW filter, WFM BW 200K(0), WFM BW 180K(1), WFM BW 40K(2)
            }
        }
    };

    field_squelch.on_change = [this](int32_t v) {
        receiver_model.set_squelch_level(100 - v);
    };
    field_squelch.set_value(0);
    receiver_model.set_squelch_level(0);

    rx_frequency = receiver_model.target_frequency();
    field_rxfrequency.set_value(rx_frequency);
    field_rxfrequency.set_step(receiver_model.frequency_step());
    field_rxfrequency.on_change = [this](rf::Frequency f) {  // available when field rxfrequency not hidden => user selected separated freq RX/TX-
        rx_frequency = f;
        if (rx_enabled)
            receiver_model.set_target_frequency(f);
    };
    field_rxfrequency.on_edit = [this, &nav]() {  // available when field rxfrequency not hidden => user selected separated freq RX/TX-
        focused_ui = 1;
        // TODO: Provide separate modal method/scheme?
        auto new_view = nav.push<FrequencyKeypadView>(rx_frequency);
        new_view->on_changed = [this](rf::Frequency f) {
            rx_frequency = f;
            if (rx_enabled)
                receiver_model.set_target_frequency(f);
            this->field_rxfrequency.set_value(f);
            set_dirty();
        };
    };

    rx_lna = receiver_model.lna();
    field_rxlna.on_change = [this](int32_t v) {
        rx_lna = v;
        if (rx_enabled)
            receiver_model.set_lna(v);
    };
    field_rxlna.set_value(rx_lna);

    rx_vga = receiver_model.vga();
    field_rxvga.on_change = [this](int32_t v) {
        rx_vga = v;
        if (rx_enabled)
            receiver_model.set_vga(v);
    };
    field_rxvga.set_value(rx_vga);

    rx_amp = receiver_model.rf_amp();
    field_rxamp.on_change = [this](int32_t v) {
        rx_amp = v;
        if (rx_enabled)
            receiver_model.set_rf_amp(rx_amp);
    };
    field_rxamp.set_value(rx_amp);

    tx_button.on_select = [this](Button&) {
        if (ptt_enabled && !transmitting) {
            set_tx(true);
        }
    };

    tx_button.on_touch_release = [this](Button&) {
        if (button_touch) {
            button_touch = false;
            set_tx(false);
        }
    };

    tx_button.on_touch_press = [this](Button&) {
        if (!transmitting) {
            button_touch = true;
        }
    };

    // These shouldn't be necessary, but because
    // this app uses both transmitter_model and directly
    // configures the baseband, these end up being required.
    transmitter_model.set_sampling_rate(sampling_rate);
    transmitter_model.set_baseband_bandwidth(1750000);

    set_tx(false);

    audio::set_rate(audio::Rate::Hz_24000);
    audio::input::start(ak4951_alc_and_wm8731_boost_GUI);  // When detected AK4951 => set up ALC mode; when detected WM8731 => set up mic_boost ON/OFF.
}

MicTXView::MicTXView(
    NavigationView& nav,
    ReceiverModel::settings_t override)
    : MicTXView(nav) {
    // Try to use the modulation/bandwidth from RX settings.
    // TODO: These concepts should be merged so there's only one.
    // TODO: enums/constants for these indexes.
    switch (override.mode) {
        case ReceiverModel::Mode::AMAudio:
            options_mode.set_selected_index(2);
            break;
        case ReceiverModel::Mode::NarrowbandFMAudio:
            options_mode.set_selected_index(0);
            break;
        case ReceiverModel::Mode::WidebandFMAudio:
            options_mode.set_selected_index(1);
            break;

        // Unsupported modulations.
        case ReceiverModel::Mode::SpectrumAnalysis:
        case ReceiverModel::Mode::Capture:
        default:
            break;
    }

    // TODO: bandwidth selection is tied too tightly to the UI
    // controls. It's not possible to set the bandwidth here without
    // refactoring. Also options_mode seems to have a category error.
}

MicTXView::~MicTXView() {
    audio::input::stop();
    transmitter_model.set_target_frequency(tx_frequency);  // Save Tx frequency instead of Rx. Or maybe we need some "System Wide" changes to seperate Tx and Rx frequency.
    transmitter_model.disable();
    if (rx_enabled)  // Also turn off audio rx if enabled
        rxaudio(false);
    baseband::shutdown();
}

}  // namespace ui
