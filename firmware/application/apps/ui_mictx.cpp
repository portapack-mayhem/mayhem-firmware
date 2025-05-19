/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2024 Mark Thompson
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
#include "radio.hpp"

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
            check_va.focus();
            break;
    }
}

void MicTXView::update_vumeter() {
    vumeter.set_value(audio_level);
}

void MicTXView::update_tx_icon() {
    tx_icon.set_foreground(transmitting ? Theme::getInstance()->fg_red->foreground : Theme::getInstance()->bg_darkest->background);
    tx_icon.set_background(transmitting ? Theme::getInstance()->fg_yellow->foreground : Theme::getInstance()->bg_darkest->background);
}

void MicTXView::on_tx_progress(const bool done) {
    // Roger beep played, stop transmitting
    if (done)
        set_tx(false);
}

uint8_t MicTXView::shift_bits(void) {
    uint8_t shift_bits_s16{4};  // shift bits factor to the captured ADC S16 audio sample.

    if (audio::debug::codec_name() == "WM8731") {
        switch (ak4951_alc_and_wm8731_boost_GUI) {
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
    } else {
        shift_bits_s16 = 8;  // Initialized default fixed >>8_FM for FM tx mod, shift audio data for AK4951, using top 8 bits s16 data (>>8)
    }
    return shift_bits_s16;
}

void MicTXView::configure_baseband() {
    // TODO: Can this use the transmitter model instead?
    baseband::set_audiotx_config(
        sampling_rate / 20,  // Update vu-meter at 20Hz
        transmitting ? transmitter_model.channel_bandwidth() : 0,
        mic_gain_x10 / 10.0,
        shift_bits(),  // to be used in dsp_modulate
        8,             // bits per sample
        TONES_F2D(tone_key_frequency(tone_key_index), sampling_rate),
        (mic_mod_index == MIC_MOD_AM),
        (mic_mod_index == MIC_MOD_DSB),
        (mic_mod_index == MIC_MOD_USB),
        (mic_mod_index == MIC_MOD_LSB));
}

void MicTXView::set_tx(bool enable) {
    if (enable) {
        if (rx_enabled)      // If audio RX is enabled
            rxaudio(false);  // Then turn off audio RX
        transmitting = true;
        configure_baseband();
        transmitter_model.set_target_frequency(tx_frequency);  // Now, no need: transmitter_model.set_tx_gain(tx_gain), nor (rf_amp);

        /* The max. Power Spectrum Densitiy in WFM with High tone mod level (80%) and high 32kHZ subtone as fmod.  with max fdeviation 150k ,
         BW aprox = 2 *(150K + 32K) = 364khz, then we just select the minimum TX  LPF 1M75. */
        transmitter_model.set_baseband_bandwidth(1'750'000);
        transmitter_model.enable();
        portapack::pin_i2s0_rx_sda.mode(3);  // This is already done in audio::init but gets changed by the CPLD overlay reprogramming
    } else {
        if (transmitting && rogerbeep_enabled) {
            baseband::request_roger_beep();  // Transmit the roger beep
            transmitting = false;            // Flag the end of the transmission (transmitter will be disabled after the beep)
        } else {
            transmitting = false;
            configure_baseband();
            transmitter_model.disable();

            if (rx_enabled) {
                rxaudio(true);  // Turn back on audio RX

                // TODO FIXME: this isn't working: vu meter isn't going to 0:
                vumeter.set_value(0);  // Reset  vumeter
                vumeter.dirty();       // Force to refresh vumeter.
            }
        }
    }
    update_tx_icon();
}

bool MicTXView::tx_button_held() {
    const auto switches_state = get_switches_state();
    return switches_state[(size_t)ui::KeyEvent::Select];
};

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
                    if (!button_touch && !tx_button_held())
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
        if (transmitting && !button_touch && !tx_button_held())
            set_tx(false);
    }
}

void MicTXView::rxaudio(bool enable) {
    if (enable) {
        audio::input::stop();
        baseband::shutdown();

        switch (mic_mod_index) {
            case MIC_MOD_NFM:  // NFM BW:8k5 or 11k / FM BW 16k
                baseband::run_image(portapack::spi_flash::image_tag_nfm_audio);
                receiver_model.set_modulation(ReceiverModel::Mode::NarrowbandFMAudio);
                // receiver_model.set_nbfm_configuration(n); is called above, depending user's selection (8k5, 11k, 16k).
                break;
            case MIC_MOD_WFM:  // WFM, BW 200Khz aprox, or the two new addional BW filters (180k, 40k)
                baseband::run_image(portapack::spi_flash::image_tag_wfm_audio);
                receiver_model.set_modulation(ReceiverModel::Mode::WidebandFMAudio);
                // receiver_model.set_wfm_configuration(n); is called above, depending user's selection (200k, 180k, 0k).
                break;
            case MIC_MOD_AM:
            case MIC_MOD_DSB:
            case MIC_MOD_USB:
            case MIC_MOD_LSB:
                baseband::run_image(portapack::spi_flash::image_tag_am_audio);
                receiver_model.set_modulation(ReceiverModel::Mode::AMAudio);                 // that AM demodulation engine is common to all Amplitude mod : AM/USB/LSB/DSB (2,3,4,5)
                if (options_mode.selected_index() < 5)                                       // We will be called here with 2,3,4,5. We treat here demod. filter 2,3,4; (excluding DSB-C case (5) it is treated more down).
                    receiver_model.set_am_configuration(options_mode.selected_index() - 1);  // selecting proper filter(2,3,4). 2-1=1=>6k-AM(1), 3-1=2=>+3k-USB(2), 4-1=3=>-3K-LSB(3),
                break;
        }

        receiver_model.set_target_frequency(bool_same_F_tx_rx_enabled ? tx_frequency : rx_frequency);
        receiver_model.enable();
        audio::output::start();
    } else {                                                                    // These incredibly convoluted steps are required for the vumeter to reappear when stopping RX.
        receiver_model.set_modulation(ReceiverModel::Mode::NarrowbandFMAudio);  // This fixes something with AM RX...
        receiver_model.disable();
        baseband::shutdown();

        baseband::run_image(portapack::spi_flash::image_tag_mic_tx);
        audio::output::stop();
        audio::input::start(ak4951_alc_and_wm8731_boost_GUI, mic_to_HP_enabled);  // set up ALC mode (AK4951) or set up mic_boost ON/OFF (WM8731). and the check box "Hear Mic"
        portapack::pin_i2s0_rx_sda.mode(3);
        configure_baseband();
    }
}

void MicTXView::set_rxbw_options(void) {
    using option_t = std::pair<std::string, int32_t>;
    using options_t = std::vector<option_t>;
    options_t rxbw;  // Aux structure to change dynamically field_rxbw contents

    switch (mic_mod_index) {
        case MIC_MOD_NFM:
            freqman_set_bandwidth_option(NFM_MODULATION, field_rxbw);  // restore dynamic field_rxbw value with NFM options from freqman_db.cpp
            field_rxbw.set_by_value(receiver_model.nbfm_configuration());
            break;
        case MIC_MOD_WFM:
            freqman_set_bandwidth_option(WFM_MODULATION, field_rxbw);  // restore dynamic field_rxbw value with WFM options from freqman_db.cpp
            field_rxbw.set_by_value(receiver_model.wfm_configuration());
            break;
        case MIC_MOD_AM:
            rxbw.emplace_back("DSB 9k", 0);  // we offer in AM DSB two audio BW 9k / 6k.
            rxbw.emplace_back("DSB 6k", 1);
            field_rxbw.set_options(rxbw);  // store that aux GUI option to the field_rxbw.
            break;
        case MIC_MOD_DSB:
            rxbw.emplace_back("USB+3k", 0);  // added dynamically two options (index 0,1) to that DSB-SC case to the field_rxbw value.
            rxbw.emplace_back("LSB-3k", 1);
            field_rxbw.set_options(rxbw);  // store that aux GUI option to the field_rxbw.
            break;
        case MIC_MOD_USB:
            rxbw.emplace_back("USB+3k", 0);  // locked a fixed option, to display it.
            field_rxbw.set_options(rxbw);    // store that aux GUI option to the field_rxbw.
            break;
        case MIC_MOD_LSB:
            rxbw.emplace_back("LSB-3k", 0);  // locked a fixed option, to display it.
            field_rxbw.set_options(rxbw);    // store that aux GUI option to the field_rxbw.
            break;
    }
}

void MicTXView::set_rxbw_defaults(bool use_app_settings) {  // Initially in that function we set up rxbw, but now also txbw.
    if (use_app_settings) {
        field_bw.set_value(transmitter_model.channel_bandwidth() / 1000);
        field_rxbw.set_by_value(rxbw_index);
    } else if (mic_mod_index == MIC_MOD_NFM) {
        field_bw.set_value(10);     // NFM TX bw 10k, RX bw 16k (index 2) default
        field_bw.set_range(1, 60);  // In NFM , FM , we are limitting index modulation range (0.08 ..5) ; (Ex max dev 60khz/12k = 5)
        field_bw.set_step(1);
        field_rxbw.set_by_value(2);  // 16k from the three options (8k5,11k,16k)
    } else if (mic_mod_index == MIC_MOD_WFM) {
        field_bw.set_value(75);      // WFM TX bw 75K, RX bw 200k (index 0) default
        field_bw.set_range(1, 150);  // In our case Mod. Index range (1,67 ...12,5) ; 150k/12k=12,5
        field_bw.set_step(1);
        field_rxbw.set_by_value(0);
    } else if ((mic_mod_index == MIC_MOD_USB) | (mic_mod_index == MIC_MOD_LSB)) {
        field_bw.set_value(3);     // In SSB by default let's limit TX_BW to 3kHz.
        field_bw.set_range(2, 3);  // User TXBW GUI range to modify that SSB TX_BW to limit SSB radiated spectrum.
        field_bw.set_step(1);
    }
    // field_bw is hidden in other modulation cases
}

void MicTXView::update_receiver_rxbw(void) {
    // In Previous fw versions, that nbfm_configuration(n) was done in any mode (FM/AM/SSB/DSB)...strictly speaking only need it in (NFM/FM)
    // In AM TX mode, we will allow both independent RX audio BW : AM 9K (9K00AE3 / AM 6K (6K00AE3). (In AM option v can be 0 (9k), 1 (6k)
    // In DSB-SC TX mode, we will allow both independent RX SSB demodulation (USB / LSB side band). in that submenu, v is 0 (SSB1 USB) or 1 (SSB2 LSB)
    switch (mic_mod_index) {
        case MIC_MOD_NFM:
            receiver_model.set_nbfm_configuration(rxbw_index);  // we are in NFM/FM case, we need to select proper NFM/FM RX channel filter, NFM BW 8K5(0), NFM BW 11K(1), FM BW 16K (2)
            break;
        case MIC_MOD_WFM:
            receiver_model.set_wfm_configuration(rxbw_index);  // we are in WFM case, we need to select proper WFB RX BW filter, WFM BW 200K(0), WFM BW 180K(1), WFM BW 40K(2)
            break;
        case MIC_MOD_AM:
            receiver_model.set_am_configuration(rxbw_index);  // we are in AM TX mode, we need to select proper AM full path config AM-9K filter. 0+0 =>AM-9K(0), 0+1=1 =>AM-6K(1),
            break;
        case MIC_MOD_USB:
        case MIC_MOD_LSB:
            break;  // change can't happen in these modes because there's only one BW choice
        case MIC_MOD_DSB:
            receiver_model.set_am_configuration(rxbw_index + 2);  // we are in DSB-SC TX mode, we need to select proper SSB filter. 0+2 =>usb(2), 1+2=3 =>lsb(3),
            break;
    }
}

MicTXView::MicTXView(
    NavigationView& nav) {
    portapack::pin_i2s0_rx_sda.mode(3);  // This is already done in audio::init but gets changed by the CPLD overlay reprogramming

    baseband::run_image(portapack::spi_flash::image_tag_mic_tx);

    bool wm = (audio::debug::codec_name() == "WM8731");
    add_children({&labels_both,
                  wm ? &labels_WM8731 : &labels_AK4951,  // enable ALC if AK4951
                  &vumeter,
                  &options_gain,  // MIC GAIN float factor on the GUI.
                  wm ? &options_wm8731_boost_mode : &options_ak4951_alc_mode,
                  &check_va,
                  &field_va_level,
                  &field_va_attack,
                  &field_va_decay,
                  &field_bw,
                  &tx_view,  // it already integrates previous rfgain, rfamp.
                  &options_mode,
                  &field_frequency,
                  &options_tone_key,
                  &check_rogerbeep,
                  &check_mic_to_HP,          // check box to activate "hear mic"
                  &check_common_freq_tx_rx,  // added to handle common or separate freq- TX/RX
                  &check_rxactive,
                  &field_volume,
                  &field_rxbw,
                  &field_squelch,
                  &field_rxfrequency,
                  &field_rxlna,
                  &field_rxvga,
                  &field_rxamp,
                  &field_tx_iq_phase_cal,
                  &tx_button,
                  &tx_icon});

    set_rxbw_options();
    set_rxbw_defaults(settings_.loaded());

    tone_keys_populate(options_tone_key);
    options_tone_key.set_selected_index(tone_key_index);
    options_tone_key.on_change = [this](size_t i, int32_t) {
        tone_key_index = i;
    };

    check_rogerbeep.set_value(rogerbeep_enabled);
    check_rogerbeep.on_select = [this](Checkbox&, bool v) {
        rogerbeep_enabled = v;
    };

    field_rxlna.set_value(receiver_model.lna());
    field_rxlna.on_change = [this](int32_t v) {
        receiver_model.set_lna(v);
    };

    field_rxvga.set_value(receiver_model.vga());
    field_rxvga.on_change = [this](int32_t v) {
        receiver_model.set_vga(v);
    };

    field_rxamp.set_value(receiver_model.rf_amp());
    field_rxamp.on_change = [this](int32_t v) {
        receiver_model.set_rf_amp(v);
    };

    radio::set_tx_max283x_iq_phase_calibration(iq_phase_calibration_value);
    field_tx_iq_phase_cal.set_range(0, hackrf_r9 ? 63 : 31);  // max2839 has 6 bits [0..63],  max2837 has 5 bits [0..31]
    field_tx_iq_phase_cal.set_value(iq_phase_calibration_value);
    field_tx_iq_phase_cal.on_change = [this](int32_t v) {
        iq_phase_calibration_value = v;
        radio::set_tx_max283x_iq_phase_calibration(iq_phase_calibration_value);
    };

    options_gain.on_change = [this](size_t, int32_t v) {
        mic_gain_x10 = v;
        configure_baseband();
    };
    options_gain.set_by_value(mic_gain_x10);  // x1.0 preselected default.

    field_rxbw.on_change = [this](size_t, int32_t v) {
        rxbw_index = v;
        update_receiver_rxbw();
    };
    field_rxbw.set_by_value(rxbw_index);

    field_squelch.set_value(receiver_model.squelch_level());
    field_squelch.on_change = [this](int32_t v) {
        receiver_model.set_squelch_level(v);
    };

    if (audio::debug::codec_name() == "WM8731") {
        options_wm8731_boost_mode.on_change = [this](size_t, int8_t v) {
            ak4951_alc_and_wm8731_boost_GUI = v;                                      // 0..4, WM8731_boost dB's options, (combination boost on/off, and effective gain in captured data >>x)
            audio::input::start(ak4951_alc_and_wm8731_boost_GUI, mic_to_HP_enabled);  // Detected (WM8731), set up the proper wm_boost on/off, 0..4 (0,1) boost_on, (2,3,4) boost_off,and the check box "Hear Mic"
            configure_baseband();                                                     // to update in real time, sending msg, var-parameters >>shift_bits FM msg, to audio_tx from M0 to M4 Proc -
        };
        if (!settings_.loaded()) {
            ak4951_alc_and_wm8731_boost_GUI = 3;  // preset GUI index 3 as default WM -> -02 dB's
        }
        options_wm8731_boost_mode.set_selected_index(ak4951_alc_and_wm8731_boost_GUI);
    } else {
        options_ak4951_alc_mode.on_change = [this](size_t, int8_t v) {
            ak4951_alc_and_wm8731_boost_GUI = v;                                      // 0..11, AK4951 Mic -Automatic volume Level Control options,
            audio::input::start(ak4951_alc_and_wm8731_boost_GUI, mic_to_HP_enabled);  // Detected (AK4951) ==> Set up proper ALC mode from 0..11 options, and the check box "Hear Mic"
            configure_baseband();                                                     // sending fixed >>8_FM, var-parameters msg, to audiotx from this M0 to M4 process.
        };
        if (!settings_.loaded())
            ak4951_alc_and_wm8731_boost_GUI = 0;
        options_ak4951_alc_mode.set_selected_index(ak4951_alc_and_wm8731_boost_GUI);
    }

    // WARNING: transmitter_model.set_target_frequency() and receiver_model.set_target_frequency() both update the same tuning freq, but one has an offset!
    tx_frequency = transmitter_model.target_frequency();
    field_frequency.on_change = [this](rf::Frequency f) {
        tx_frequency = f;
        if (!rx_enabled)
            transmitter_model.set_target_frequency(f);
        else if (bool_same_F_tx_rx_enabled)
            receiver_model.set_target_frequency(f);
        // else tuning freq will be updated when rx is enabled, transmitting starts, or when rx_frequency is changed (if rx_enabled)
    };
    field_frequency.on_edit = [this, &nav]() {
        focused_ui = 0;
        // TODO: Provide separate modal method/scheme?
        auto new_view = nav.push<FrequencyKeypadView>(tx_frequency);
        new_view->on_changed = [this](rf::Frequency f) {
            this->field_frequency.set_value(f);
        };
    };
    field_frequency.set_value(tx_frequency);

    // TODO: would be nice if frequency step was configurable in this app
    field_frequency.set_step(receiver_model.frequency_step());

    // WARNING: transmitter_model.set_target_frequency() and receiver_model.set_target_frequency() both update the same tuning freq, but one has an offset!
    field_rxfrequency.set_value(rx_frequency);
    field_rxfrequency.on_change = [this](rf::Frequency f) {  // available when field rxfrequency not hidden => user selected separated freq RX/TX-
        rx_frequency = f;
        if (rx_enabled)
            receiver_model.set_target_frequency(rx_frequency);
    };
    field_rxfrequency.on_edit = [this, &nav]() {  // available when field rxfrequency not hidden => user selected separated freq RX/TX-
        focused_ui = 1;
        // TODO: Provide separate modal method/scheme?
        auto new_view = nav.push<FrequencyKeypadView>(rx_frequency);
        new_view->on_changed = [this](rf::Frequency f) {
            this->field_rxfrequency.set_value(f);
        };
    };

    // TODO: would be nice if frequency step was configurable in this app
    field_rxfrequency.set_step(receiver_model.frequency_step());

    check_common_freq_tx_rx.on_select = [this](Checkbox&, bool v) {
        bool_same_F_tx_rx_enabled = v;
        field_rxfrequency.hidden(v);
        set_dirty();
        receiver_model.set_target_frequency(bool_same_F_tx_rx_enabled ? tx_frequency : rx_frequency);  // To go to the proper tuned freq. when toggling it
    };
    check_common_freq_tx_rx.set_value(bool_same_F_tx_rx_enabled);

    field_bw.on_change = [this](uint32_t v) {
        transmitter_model.set_channel_bandwidth(v * 1000);
    };
    field_bw.set_value(transmitter_model.channel_bandwidth() / 1000);  // pre-default first time, TX deviation FM for NFM / FM

    // now, no need direct update, field_rfgain, field_rfamp (it is done in ui_transmitter.cpp)

    options_mode.on_change = [this](size_t, int32_t v) {
        mic_mod_index = v;

        // update bw & tone key visibility - only visible in NFM/WFM modes
        if ((mic_mod_index == MIC_MOD_NFM) || (mic_mod_index == MIC_MOD_WFM)) {
            field_bw.hidden(false);
            options_tone_key.hidden(false);
        } else {
            if ((mic_mod_index == MIC_MOD_USB) || (mic_mod_index == MIC_MOD_LSB)) {
                field_bw.hidden(false);
            } else {
                field_bw.hidden(true);
            }
            options_tone_key.set_selected_index(0);
            options_tone_key.hidden(true);
        }

        // update rogerbeep visibility - disable in USB/LSB modes
        if ((mic_mod_index == MIC_MOD_USB) || (mic_mod_index == MIC_MOD_LSB)) {
            check_rogerbeep.set_value(false);
            check_rogerbeep.hidden(true);
        } else {
            check_rogerbeep.hidden(false);
        }

        // update squelch visibility - only visible in NFM mode
        field_squelch.hidden(mic_mod_index != MIC_MOD_NFM);

        // update bandwidth
        set_rxbw_options();
        set_rxbw_defaults(false);
        field_rxbw.hidden(false);
        set_dirty();  // Refresh display

        rxaudio(rx_enabled);  // Update now if we have RX audio on
        // configure_baseband();
    };
    options_mode.set_selected_index(mic_mod_index);

    check_mic_to_HP.on_select = [this](Checkbox&, bool v) {
        mic_to_HP_enabled = v;
        if (mic_to_HP_enabled) {  // When user click to "hear mic to HP", we will select the higher acoustic sound Option MODE  ALC or BOOST-
            audio::input::loopback_mic_to_hp_enable();
            if (audio::debug::codec_name() == "WM8731") {
                options_wm8731_boost_mode.set_selected_index(0);  // In WM we always go to Boost +12 dB’s respect reference level
            } else if (ak4951_alc_and_wm8731_boost_GUI == 0)      // In AK we are changing only that ALC index =0, because in that option, there is no sound.
                options_ak4951_alc_mode.set_selected_index(1);    // alc_mode =0 , means no ALC,no DIGITAL filter block (by passed), and that mode has no loopback mode.
        } else {
            audio::input::loopback_mic_to_hp_disable();
        }
    };
    check_mic_to_HP.set_value(mic_to_HP_enabled);

    tx_button.on_select = [this](Button&) {
        if (!transmitting) {
            set_tx(true);
        }
    };

    tx_button.on_touch_release = [this](Button&) {
        button_touch = false;
    };

    tx_button.on_touch_press = [this](Button&) {
        button_touch = true;
    };

    field_va_level.on_change = [this](int32_t v) {
        va_level = v;
        vumeter.set_mark(v);
    };
    field_va_level.set_value(va_level);

    field_va_attack.set_value(attack_ms);
    field_va_attack.on_change = [this](int32_t v) {
        attack_ms = v;
    };

    field_va_decay.set_value(decay_ms);
    field_va_decay.on_change = [this](int32_t v) {
        decay_ms = v;
    };

    check_va.on_select = [this](Checkbox&, bool v) {
        va_enabled = v;
        if (va_enabled)
            check_rxactive.set_value(false);  // Disallow RX-audio in VOX mode (for now) - Future TODO: Should allow VOX during RX audio
    };
    check_va.set_value(va_enabled);

    // These shouldn't be necessary, but because
    // this app uses both transmitter_model and directly
    // configures the baseband, these end up being required.
    transmitter_model.set_sampling_rate(sampling_rate);
    transmitter_model.set_baseband_bandwidth(1750000);

    set_tx(false);

    audio::set_rate(audio::Rate::Hz_24000);
    audio::input::start(ak4951_alc_and_wm8731_boost_GUI, mic_to_HP_enabled);  // set up ALC mode (AK4951) or set up mic_boost ON/OFF (WM8731). and the check box "Hear Mic"

    // Workaround for bad RX reception when app first started -- shouldn't be necessary:
    // Trigger receiver to update modulation.
    if (rx_enabled)
        receiver_model.set_squelch_level(receiver_model.squelch_level());

    check_rxactive.on_select = [this](Checkbox&, bool v) {
        //		vumeter.set_value(0);	//Start with a clean vumeter
        rx_enabled = v;
        check_mic_to_HP.hidden(rx_enabled);  // Toggle Hide / show "Hear Mic" checkbox depending if we activate or not the receiver. (if RX on => no visible "Mic Hear" option)
        if ((rx_enabled) && (transmitting))
            check_mic_to_HP.set_value(transmitting);  // Once we activate the "Rx audio" in reception time we disable "Hear Mic", but we allow it again in TX time.

        if (rx_enabled)
            check_va.set_value(false);  // Disallow voice activation during RX audio (for now) - Future TODO: Should allow VOX during RX audio

        rxaudio(v);   // Activate-Deactivate audio RX (receiver) accordingly
        set_dirty();  // Refresh interface
    };
    check_rxactive.set_value(rx_enabled);
}

MicTXView::MicTXView(
    NavigationView& nav,
    ReceiverModel::settings_t override)
    : MicTXView(nav) {
    // Settings to override when launched from another app (versus from AppSettings .ini file)
    // Try to use the modulation/bandwidth from RX settings.
    // TODO: These concepts should be merged so there's only one.
    switch (override.mode) {
        case ReceiverModel::Mode::AMAudio:
            options_mode.set_selected_index(MIC_MOD_AM);
            break;
        case ReceiverModel::Mode::NarrowbandFMAudio:
            options_mode.set_selected_index(MIC_MOD_NFM);
            break;
        case ReceiverModel::Mode::WidebandFMAudio:
            options_mode.set_selected_index(MIC_MOD_WFM);
            break;

        // Unsupported modulations.
        case ReceiverModel::Mode::SpectrumAnalysis:
        case ReceiverModel::Mode::Capture:
        default:
            break;
    }

    field_frequency.set_value(override.frequency_app_override);
    check_common_freq_tx_rx.set_value(true);  // freq passed from other app is in tx_frequency, so set rx_frequency=tx_frequency

    // TODO: bandwidth selection is tied too tightly to the UI
    // controls. It's not possible to set the bandwidth here without
    // refactoring. Also options_mode seems to have a category error.
}

MicTXView::~MicTXView() {
    audio::input::stop();
    if (rx_enabled) {  // Also turn off both (audio rx if enabled, and disable  mic_loop to HP)
        rxaudio(false);
        audio::input::loopback_mic_to_hp_disable();  // Leave Mic audio off in the main menu (as original audio path, otherwise we had No audio in next "Audio App")
    }
    transmitter_model.set_target_frequency(tx_frequency);  // Set tx_frequency here to be saved in app_settings (might not match rx_frequency)
    transmitter_model.disable();
    baseband::shutdown();
}

}  // namespace ui
