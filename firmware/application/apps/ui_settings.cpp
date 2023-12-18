/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2023 gullradriel, Nilorea Studio Inc.
 * Copyright (C) 2023 Kyle Reed
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

#include "ui_settings.hpp"

#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_touch_calibration.hpp"

#include "portapack_persistent_memory.hpp"
#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

#include "audio.hpp"
#include "portapack.hpp"
using namespace portapack;

#include "file.hpp"
namespace fs = std::filesystem;

#include "string_format.hpp"
#include "ui_styles.hpp"
#include "cpld_update.hpp"

namespace pmem = portapack::persistent_memory;

namespace ui {

/* Sends a UI refresh message to cause the status bar to redraw. */
static void send_system_refresh() {
    StatusRefreshMessage message{};
    EventDispatcher::send_message(message);
}

/* SetDateTimeView ***************************************/

SetDateTimeView::SetDateTimeView(
    NavigationView& nav) {
    button_save.on_select = [&nav, this](Button&) {
        const auto model = this->form_collect();
        const rtc::RTC new_datetime{
            model.year, model.month, model.day,
            model.hour, model.minute, model.second};
        rtcSetTime(&RTCD1, &new_datetime);
        nav.pop();
    },

    button_cancel.on_select = [&nav](Button&) {
        nav.pop();
    },

    add_children({
        &labels,
        &field_year,
        &field_month,
        &field_day,
        &field_hour,
        &field_minute,
        &field_second,
        &button_save,
        &button_cancel,
    });

    rtc::RTC datetime;
    rtcGetTime(&RTCD1, &datetime);
    SetDateTimeModel model{
        datetime.year(),
        datetime.month(),
        datetime.day(),
        datetime.hour(),
        datetime.minute(),
        datetime.second()};

    form_init(model);
}

void SetDateTimeView::focus() {
    button_cancel.focus();
}

void SetDateTimeView::form_init(const SetDateTimeModel& model) {
    field_year.set_value(model.year);
    field_month.set_value(model.month);
    field_day.set_value(model.day);
    field_hour.set_value(model.hour);
    field_minute.set_value(model.minute);
    field_second.set_value(model.second);
}

SetDateTimeModel SetDateTimeView::form_collect() {
    return {
        .year = static_cast<uint16_t>(field_year.value()),
        .month = static_cast<uint8_t>(field_month.value()),
        .day = static_cast<uint8_t>(field_day.value()),
        .hour = static_cast<uint8_t>(field_hour.value()),
        .minute = static_cast<uint8_t>(field_minute.value()),
        .second = static_cast<uint8_t>(field_second.value())};
}

/* SetRadioView ******************************************/

SetRadioView::SetRadioView(
    NavigationView& nav) {
    button_cancel.on_select = [&nav](Button&) {
        nav.pop();
    };

    add_children({
        &label_source,
        &value_source,
        &value_source_frequency,
        &check_clkout,
        &field_clkout_freq,
        &labels_clkout_khz,
        &value_freq_step,
        &labels_bias,
        &check_bias,
        &disable_external_tcxo,  // TODO: always show?
        &button_save,
        &button_cancel,
    });

    const auto reference = clock_manager.get_reference();

    if (reference.source == ClockManager::ReferenceSource::Xtal) {
        add_children({
            &labels_correction,
            &field_ppm,
        });
    }

    std::string source_name("---");
    switch (reference.source) {
        case ClockManager::ReferenceSource::Xtal:
            source_name = "HackRF";
            break;
        case ClockManager::ReferenceSource::PortaPack:
            source_name = "PortaPack";
            break;
        case ClockManager::ReferenceSource::External:
            source_name = "External";
            break;
    }

    value_source.set(source_name);
    value_source_frequency.set(
        to_string_dec_uint(reference.frequency / 1000000, 2) + "." +
        to_string_dec_uint((reference.frequency % 1000000) / 100, 4, '0') + " MHz");

    // Make these Text controls look like Labels.
    label_source.set_style(&Styles::light_grey);
    value_source.set_style(&Styles::light_grey);
    value_source_frequency.set_style(&Styles::light_grey);

    SetFrequencyCorrectionModel model{
        static_cast<int8_t>(pmem::correction_ppb() / 1000), 0};

    form_init(model);

    check_clkout.set_value(pmem::clkout_enabled());
    check_clkout.on_select = [this](Checkbox&, bool v) {
        clock_manager.enable_clock_output(v);
        pmem::set_clkout_enabled(v);
        send_system_refresh();
    };

    field_clkout_freq.set_value(pmem::clkout_freq());
    value_freq_step.set_style(&Styles::light_grey);

    field_clkout_freq.on_select = [this](NumberField&) {
        freq_step_khz++;
        if (freq_step_khz > 3) {
            freq_step_khz = 0;
        }
        switch (freq_step_khz) {
            case 0:
                value_freq_step.set("   |");
                break;
            case 1:
                value_freq_step.set("  | ");
                break;
            case 2:
                value_freq_step.set(" |  ");
                break;
            case 3:
                value_freq_step.set("|   ");
                break;
        }
        field_clkout_freq.set_step(pow(10, freq_step_khz));
    };

    check_bias.set_value(get_antenna_bias());
    check_bias.on_select = [this](Checkbox&, bool v) {
        set_antenna_bias(v);

        // Update the radio.
        receiver_model.set_antenna_bias();
        transmitter_model.set_antenna_bias();
        // The models won't actually disable this if they are not 'enabled_'.
        // Be extra sure this is turned off.
        if (!v)
            radio::set_antenna_bias(false);

        send_system_refresh();
    };

    disable_external_tcxo.set_value(pmem::config_disable_external_tcxo());

    button_save.on_select = [this, &nav](Button&) {
        const auto model = this->form_collect();
        pmem::set_correction_ppb(model.ppm * 1000);
        pmem::set_clkout_freq(model.freq);
        pmem::set_config_disable_external_tcxo(disable_external_tcxo.value());
        clock_manager.enable_clock_output(pmem::clkout_enabled());
        nav.pop();
    };
}

void SetRadioView::focus() {
    button_save.focus();
}

void SetRadioView::form_init(const SetFrequencyCorrectionModel& model) {
    field_ppm.set_value(model.ppm);
}

SetFrequencyCorrectionModel SetRadioView::form_collect() {
    return {
        .ppm = static_cast<int8_t>(field_ppm.value()),
        .freq = static_cast<uint32_t>(field_clkout_freq.value()),
    };
}

/* SetUIView *********************************************/

SetUIView::SetUIView(NavigationView& nav) {
    add_children({&checkbox_disable_touchscreen,
                  &checkbox_bloff,
                  &options_bloff,
                  &checkbox_showsplash,
                  &checkbox_showclock,
                  &options_clockformat,
                  &checkbox_guireturnflag,
                  &labels,
                  &toggle_camera,
                  &toggle_sleep,
                  &toggle_stealth,
                  &toggle_converter,
                  &toggle_bias_tee,
                  &toggle_clock,
                  &toggle_mute,
                  &toggle_sd_card,
                  &button_save,
                  &button_cancel});

    // Display "Disable speaker" option only if AK4951 Codec which has separate speaker/headphone control
    if (audio::speaker_disable_supported()) {
        add_child(&toggle_speaker);
    }

    checkbox_disable_touchscreen.set_value(pmem::disable_touchscreen());
    checkbox_showsplash.set_value(pmem::config_splash());
    checkbox_showclock.set_value(!pmem::hide_clock());
    checkbox_guireturnflag.set_value(pmem::show_gui_return_icon());

    const auto backlight_config = pmem::config_backlight_timer();
    checkbox_bloff.set_value(backlight_config.timeout_enabled());
    options_bloff.set_by_value(backlight_config.timeout_enum());

    if (pmem::clock_with_date()) {
        options_clockformat.set_selected_index(1);
    } else {
        options_clockformat.set_selected_index(0);
    }

    // NB: Invert so "active" == "not hidden"
    toggle_camera.set_value(!pmem::ui_hide_camera());
    toggle_sleep.set_value(!pmem::ui_hide_sleep());
    toggle_stealth.set_value(!pmem::ui_hide_stealth());
    toggle_converter.set_value(!pmem::ui_hide_converter());
    toggle_bias_tee.set_value(!pmem::ui_hide_bias_tee());
    toggle_clock.set_value(!pmem::ui_hide_clock());
    toggle_speaker.set_value(!pmem::ui_hide_speaker());
    toggle_mute.set_value(!pmem::ui_hide_mute());
    toggle_sd_card.set_value(!pmem::ui_hide_sd_card());

    button_save.on_select = [&nav, this](Button&) {
        pmem::set_config_backlight_timer({(pmem::backlight_timeout_t)options_bloff.selected_index_value(),
                                          checkbox_bloff.value()});

        if (checkbox_showclock.value()) {
            if (options_clockformat.selected_index() == 1)
                pmem::set_clock_with_date(true);
            else
                pmem::set_clock_with_date(false);
        }

        pmem::set_config_splash(checkbox_showsplash.value());
        pmem::set_clock_hidden(!checkbox_showclock.value());
        pmem::set_gui_return_icon(checkbox_guireturnflag.value());
        pmem::set_disable_touchscreen(checkbox_disable_touchscreen.value());

        pmem::set_ui_hide_camera(!toggle_camera.value());
        pmem::set_ui_hide_sleep(!toggle_sleep.value());
        pmem::set_ui_hide_stealth(!toggle_stealth.value());
        pmem::set_ui_hide_converter(!toggle_converter.value());
        pmem::set_ui_hide_bias_tee(!toggle_bias_tee.value());
        pmem::set_ui_hide_clock(!toggle_clock.value());
        pmem::set_ui_hide_speaker(!toggle_speaker.value());
        pmem::set_ui_hide_mute(!toggle_mute.value());
        pmem::set_ui_hide_sd_card(!toggle_sd_card.value());
        send_system_refresh();

        nav.pop();
    };
    button_cancel.on_select = [&nav, this](Button&) {
        nav.pop();
    };
}

void SetUIView::focus() {
    button_save.focus();
}

/* SetSDCardView *********************************************/

SetSDCardView::SetSDCardView(NavigationView& nav) {
    add_children({&labels,
                  &checkbox_sdcard_speed,
                  &button_test_sdcard_high_speed,
                  &text_sdcard_test_status,
                  &button_save,
                  &button_cancel});

    checkbox_sdcard_speed.set_value(pmem::config_sdcard_high_speed_io());

    button_test_sdcard_high_speed.on_select = [&nav, this](Button&) {
        pmem::set_config_sdcard_high_speed_io(true, false);
        text_sdcard_test_status.set("!! HIGH SPEED MODE ON !!");
    };

    button_save.on_select = [&nav, this](Button&) {
        pmem::set_config_sdcard_high_speed_io(checkbox_sdcard_speed.value(), true);
        send_system_refresh();
        nav.pop();
    };

    button_cancel.on_select = [&nav, this](Button&) {
        nav.pop();
    };
}

void SetSDCardView::focus() {
    button_save.focus();
}

/* SetConverterSettingsView ******************************/

SetConverterSettingsView::SetConverterSettingsView(NavigationView& nav) {
    add_children({
        &labels,
        &check_show_converter,
        &check_converter,
        &opt_converter_mode,
        &field_converter_freq,
        &button_return,
    });

    check_show_converter.set_value(!pmem::ui_hide_converter());
    check_show_converter.on_select = [this](Checkbox&, bool v) {
        pmem::set_ui_hide_converter(!v);
        if (!v) {
            check_converter.set_value(false);
        }
        // Retune to take converter change in account.
        receiver_model.set_target_frequency(receiver_model.target_frequency());
        // Refresh status bar converter icon.
        send_system_refresh();
    };

    check_converter.set_value(pmem::config_converter());
    check_converter.on_select = [this](Checkbox&, bool v) {
        if (v) {
            check_show_converter.set_value(true);
            pmem::set_ui_hide_converter(false);
        }
        pmem::set_config_converter(v);
        // Retune to take converter change in account.
        receiver_model.set_target_frequency(receiver_model.target_frequency());
        // Refresh status bar converter icon.
        send_system_refresh();
    };

    opt_converter_mode.set_by_value(pmem::config_updown_converter());
    opt_converter_mode.on_change = [this](size_t, OptionsField::value_t v) {
        pmem::set_config_updown_converter(v);
        // Refresh status bar with up or down icon.
        send_system_refresh();
    };

    field_converter_freq.set_step(1'000'000);
    field_converter_freq.set_value(pmem::config_converter_freq());
    field_converter_freq.on_change = [this](rf::Frequency f) {
        pmem::set_config_converter_freq(f);
        // Retune to take converter change in account.
        receiver_model.set_target_frequency(receiver_model.target_frequency());
    };
    field_converter_freq.on_edit = [this, &nav]() {
        auto new_view = nav.push<FrequencyKeypadView>(field_converter_freq.value());
        new_view->on_changed = [this](rf::Frequency f) {
            field_converter_freq.set_value(f);
        };
    };

    button_return.on_select = [&nav, this](Button&) {
        nav.pop();
    };
}

void SetConverterSettingsView::focus() {
    button_return.focus();
}

/* SetFrequencyCorrectionView ****************************/

SetFrequencyCorrectionView::SetFrequencyCorrectionView(NavigationView& nav) {
    add_children({
        &labels,
        &opt_rx_correction_mode,
        &field_rx_correction,
        &opt_tx_correction_mode,
        &field_tx_correction,
        &button_return,
    });

    opt_rx_correction_mode.set_by_value(pmem::config_freq_rx_correction_updown());
    opt_rx_correction_mode.on_change = [this](size_t, OptionsField::value_t v) {
        pmem::set_freq_rx_correction_updown(v);
    };

    opt_tx_correction_mode.set_by_value(pmem::config_freq_tx_correction_updown());
    opt_tx_correction_mode.on_change = [this](size_t, OptionsField::value_t v) {
        pmem::set_freq_tx_correction_updown(v);
    };

    field_rx_correction.set_step(100'000);
    field_rx_correction.set_value(pmem::config_freq_rx_correction());
    field_rx_correction.on_change = [this](rf::Frequency f) {
        pmem::set_config_freq_rx_correction(f);
        // Retune to take converter change in account.
        receiver_model.set_target_frequency(receiver_model.target_frequency());
    };
    field_rx_correction.on_edit = [this, &nav]() {
        auto new_view = nav.push<FrequencyKeypadView>(field_rx_correction.value());
        new_view->on_changed = [this](rf::Frequency f) {
            field_rx_correction.set_value(f);
        };
    };

    field_tx_correction.set_step(100'000);
    field_tx_correction.set_value(pmem::config_freq_tx_correction());
    field_tx_correction.on_change = [this](rf::Frequency f) {
        pmem::set_config_freq_tx_correction(f);
        // Retune to take converter change in account. NB: receiver_model.
        receiver_model.set_target_frequency(receiver_model.target_frequency());
    };
    field_tx_correction.on_edit = [this, &nav]() {
        auto new_view = nav.push<FrequencyKeypadView>(field_tx_correction.value());
        new_view->on_changed = [this](rf::Frequency f) {
            field_tx_correction.set_value(f);
        };
    };

    button_return.on_select = [&nav, this](Button&) {
        nav.pop();
    };
}

void SetFrequencyCorrectionView::focus() {
    button_return.focus();
}

/* SetPersistentMemoryView *******************************/

SetPersistentMemoryView::SetPersistentMemoryView(NavigationView& nav) {
    add_children({
        &labels,
        &text_pmem_status,
        &check_use_sdcard_for_pmem,
        &button_save_mem_to_file,
        &button_load_mem_from_file,
        &button_load_mem_defaults,
        &button_return,
    });

    text_pmem_status.set_style(&Styles::yellow);

    check_use_sdcard_for_pmem.set_value(pmem::should_use_sdcard_for_pmem());
    check_use_sdcard_for_pmem.on_select = [this](Checkbox&, bool v) {
        File pmem_flag_file_handle;
        if (v) {
            if (fs::file_exists(PMEM_FILEFLAG)) {
                text_pmem_status.set("P.Mem flag file present.");
            } else {
                auto error = pmem_flag_file_handle.create(PMEM_FILEFLAG);
                if (error)
                    text_pmem_status.set("Error creating P.Mem File!");
                else
                    text_pmem_status.set("P.Mem flag file created.");
            }
        } else {
            auto result = delete_file(PMEM_FILEFLAG);
            if (result.code() != FR_OK)
                text_pmem_status.set("Error deleting P.Mem flag!");
            else
                text_pmem_status.set("P.Mem flag file deleted.");
        }
    };

    button_save_mem_to_file.on_select = [&nav, this](Button&) {
        if (!pmem::save_persistent_settings_to_file())
            text_pmem_status.set("Error saving settings!");
        else
            text_pmem_status.set("Settings saved.");
    };

    button_load_mem_from_file.on_select = [&nav, this](Button&) {
        if (!pmem::load_persistent_settings_from_file()) {
            text_pmem_status.set("Error loading settings!");
        } else {
            text_pmem_status.set("Settings loaded.");
            // Refresh status bar with icon up or down
            send_system_refresh();
        }
    };

    button_load_mem_defaults.on_select = [&nav, this](Button&) {
        nav.push<ModalMessageView>(
            "Warning!",
            "This will reset the P.Mem\nto default settings.",
            YESNO,
            [this](bool choice) {
                if (choice) {
                    pmem::cache::defaults();
                }
            });
    };

    button_return.on_select = [&nav, this](Button&) {
        nav.pop();
    };
}

void SetPersistentMemoryView::focus() {
    button_return.focus();
}

/* SetAudioView ******************************************/

SetAudioView::SetAudioView(NavigationView& nav) {
    add_children({&labels,
                  &field_tone_mix,
                  &button_save,
                  &button_cancel});

    field_tone_mix.set_value(pmem::tone_mix());

    button_save.on_select = [&nav, this](Button&) {
        pmem::set_tone_mix(field_tone_mix.value());
        audio::output::update_audio_mute();
        nav.pop();
    };

    button_cancel.on_select = [&nav, this](Button&) {
        nav.pop();
    };
}

void SetAudioView::focus() {
    button_save.focus();
}

/* SetQRCodeView *****************************************/

SetQRCodeView::SetQRCodeView(NavigationView& nav) {
    add_children({
        &labels,
        &checkbox_bigger_qr,
        &button_save,
        &button_cancel,
    });

    checkbox_bigger_qr.set_value(pmem::show_bigger_qr_code());

    button_save.on_select = [&nav, this](Button&) {
        pmem::set_show_bigger_qr_code(checkbox_bigger_qr.value());
        nav.pop();
    };

    button_cancel.on_select = [&nav, this](Button&) {
        nav.pop();
    };
}

void SetQRCodeView::focus() {
    button_save.focus();
}

/* SetEncoderDialView ************************************/

SetEncoderDialView::SetEncoderDialView(NavigationView& nav) {
    add_children({&labels,
                  &field_encoder_dial_sensitivity,
                  &button_save,
                  &button_cancel});

    field_encoder_dial_sensitivity.set_by_value(pmem::config_encoder_dial_sensitivity());

    button_save.on_select = [&nav, this](Button&) {
        pmem::set_encoder_dial_sensitivity(field_encoder_dial_sensitivity.selected_index_value());
        nav.pop();
    };

    button_cancel.on_select = [&nav, this](Button&) {
        nav.pop();
    };
}

void SetEncoderDialView::focus() {
    button_save.focus();
}

/* SettingsMenuView **************************************/

SettingsMenuView::SettingsMenuView(NavigationView& nav) {
    if (pmem::show_gui_return_icon()) {
        add_items({{"..", ui::Color::light_grey(), &bitmap_icon_previous, [&nav]() { nav.pop(); }}});
    }
    add_items({
        {"Audio", ui::Color::dark_cyan(), &bitmap_icon_speaker, [&nav]() { nav.push<SetAudioView>(); }},
        {"Calibration", ui::Color::dark_cyan(), &bitmap_icon_options_touch, [&nav]() { nav.push<TouchCalibrationView>(); }},
        {"Converter", ui::Color::dark_cyan(), &bitmap_icon_options_radio, [&nav]() { nav.push<SetConverterSettingsView>(); }},
        {"Date/Time", ui::Color::dark_cyan(), &bitmap_icon_options_datetime, [&nav]() { nav.push<SetDateTimeView>(); }},
        {"Encoder Dial", ui::Color::dark_cyan(), &bitmap_icon_setup, [&nav]() { nav.push<SetEncoderDialView>(); }},
        {"Freq. Correct", ui::Color::dark_cyan(), &bitmap_icon_options_radio, [&nav]() { nav.push<SetFrequencyCorrectionView>(); }},
        {"P.Memory Mgmt", ui::Color::dark_cyan(), &bitmap_icon_memory, [&nav]() { nav.push<SetPersistentMemoryView>(); }},
        {"QR Code", ui::Color::dark_cyan(), &bitmap_icon_qr_code, [&nav]() { nav.push<SetQRCodeView>(); }},
        {"Radio", ui::Color::dark_cyan(), &bitmap_icon_options_radio, [&nav]() { nav.push<SetRadioView>(); }},
        {"User Interface", ui::Color::dark_cyan(), &bitmap_icon_options_ui, [&nav]() { nav.push<SetUIView>(); }},
        {"SD Card", ui::Color::dark_cyan(), &bitmap_icon_sdcard, [&nav]() { nav.push<SetSDCardView>(); }},
    });
    set_max_rows(2);  // allow wider buttons
}

} /* namespace ui */
