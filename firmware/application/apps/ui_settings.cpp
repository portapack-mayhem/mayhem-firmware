/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2023 gullradriel, Nilorea Studio Inc.
 * Copyright (C) 2023 Kyle Reed
 * Copyright (C) 2024 Mark Thompson
 * Copyright (C) 2024 u-foka
 * Copyright (C) 2024 HTotoo
 * Copyleft (ɔ) 2024 zxkmm under GPL license
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
#include "ui_text_editor.hpp"
#include "ui_external_items_menu_loader.hpp"

#include "portapack_persistent_memory.hpp"
#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

#include "audio.hpp"
#include "portapack.hpp"
using namespace portapack;

#include "file.hpp"
#include "file_path.hpp"
namespace fs = std::filesystem;

#include "string_format.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "cpld_update.hpp"
#include "config_mode.hpp"
#include "i2cdevmanager.hpp"
#include "i2cdev_max17055.hpp"

#include "file_reader.hpp"

extern ui::SystemView* system_view_ptr;

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
        rtc::RTC new_datetime{model.year, model.month, model.day, model.hour, model.minute, model.second};
        pmem::set_config_dst(model.dst);
        rtc_time::set(new_datetime);  // NB: 1 hour will be subtracted if value is stored in RTC during DST
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
        &text_weekday,
        &text_day_of_year,
        &text_in_dst_range,
        &checkbox_dst_enable,
        &options_dst_start_which,
        &options_dst_start_weekday,
        &options_dst_start_month,
        &options_dst_end_which,
        &options_dst_end_weekday,
        &options_dst_end_month,
        &button_save,
        &button_cancel,
    });

    // Populate DST options (same string text for start & end)
    options_dst_start_which.set_options(which_options);
    options_dst_end_which.set_options(which_options);
    options_dst_start_weekday.set_options(weekday_options);
    options_dst_end_weekday.set_options(weekday_options);
    options_dst_start_month.set_options(month_options);
    options_dst_end_month.set_options(month_options);

    const auto dst_changed_fn = [this](size_t, uint32_t) {
        handle_date_field_update();
    };

    const auto date_changed_fn = [this](int32_t) {
        handle_date_field_update();
    };

    field_year.on_change = date_changed_fn;
    field_month.on_change = date_changed_fn;
    field_day.on_change = date_changed_fn;

    options_dst_start_which.on_change = dst_changed_fn;
    options_dst_start_weekday.on_change = dst_changed_fn;
    options_dst_start_month.on_change = dst_changed_fn;
    options_dst_end_which.on_change = dst_changed_fn;
    options_dst_end_weekday.on_change = dst_changed_fn;
    options_dst_end_month.on_change = dst_changed_fn;

    rtc::RTC datetime;
    rtc_time::now(datetime);
    SetDateTimeModel model{
        datetime.year(),
        datetime.month(),
        datetime.day(),
        datetime.hour(),
        datetime.minute(),
        datetime.second(),
        pmem::config_dst()};
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
    checkbox_dst_enable.set_value(model.dst.b.dst_enabled);
    options_dst_start_which.set_by_value(model.dst.b.start_which);
    options_dst_start_weekday.set_by_value(model.dst.b.start_weekday);
    options_dst_start_month.set_by_value(model.dst.b.start_month);
    options_dst_end_which.set_by_value(model.dst.b.end_which);
    options_dst_end_weekday.set_by_value(model.dst.b.end_weekday);
    options_dst_end_month.set_by_value(model.dst.b.end_month);
}

SetDateTimeModel SetDateTimeView::form_collect() {
    return {
        .year = static_cast<uint16_t>(field_year.value()),
        .month = static_cast<uint8_t>(field_month.value()),
        .day = static_cast<uint8_t>(field_day.value()),
        .hour = static_cast<uint8_t>(field_hour.value()),
        .minute = static_cast<uint8_t>(field_minute.value()),
        .second = static_cast<uint8_t>(field_second.value()),
        .dst = dst_collect()};
}

pmem::dst_config_t SetDateTimeView::dst_collect() {
    pmem::dst_config_t dst;
    dst.b.dst_enabled = static_cast<uint8_t>(checkbox_dst_enable.value());
    dst.b.start_which = static_cast<uint8_t>(options_dst_start_which.selected_index_value());
    dst.b.start_weekday = static_cast<uint8_t>(options_dst_start_weekday.selected_index_value());
    dst.b.start_month = static_cast<uint8_t>(options_dst_start_month.selected_index_value());
    dst.b.end_which = static_cast<uint8_t>(options_dst_end_which.selected_index_value());
    dst.b.end_weekday = static_cast<uint8_t>(options_dst_end_weekday.selected_index_value());
    dst.b.end_month = static_cast<uint8_t>(options_dst_end_month.selected_index_value());
    return dst;
}

void SetDateTimeView::handle_date_field_update() {
    auto weekday = rtc_time::day_of_week(field_year.value(), field_month.value(), field_day.value());
    auto doy = rtc_time::day_of_year(field_year.value(), field_month.value(), field_day.value());
    bool valid_date = (field_day.value() <= rtc_time::days_per_month(field_year.value(), field_month.value()));
    text_weekday.set(valid_date ? weekday_options[weekday].first : "-");
    text_day_of_year.set(valid_date ? to_string_dec_uint(doy, 3) : "-");
    text_in_dst_range.set(checkbox_dst_enable.value() && rtc_time::dst_test_date_range(field_year.value(), doy, dst_collect()) ? "DST" : "");
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

    std::string source_name = clock_manager.get_source();

    value_source.set(source_name);
    value_source_frequency.set(clock_manager.get_freq());

    // Make these Text controls look like Labels.
    label_source.set_style(Theme::getInstance()->fg_light);
    value_source.set_style(Theme::getInstance()->fg_light);
    value_source_frequency.set_style(Theme::getInstance()->fg_light);

    SetFrequencyCorrectionModel model{
        static_cast<int8_t>(pmem::correction_ppb() / 1000), 0};

    form_init(model);

    check_clkout.set_value(pmem::clkout_enabled());
    check_clkout.on_select = [this](Checkbox&, bool v) {
        clock_manager.enable_clock_output(v);
        pmem::set_clkout_enabled(v);
        send_system_refresh();
    };

    // Disallow CLKOUT freq change on hackrf_r9 due to dependencies on GP_CLKIN (same Si5351A clock);
    // see comments in ClockManager::enable_clock_output()
    if (hackrf_r9) {
        if (pmem::clkout_freq() != 10000)
            pmem::set_clkout_freq(10000);
        field_clkout_freq.set_focusable(false);
    }

    field_clkout_freq.set_value(pmem::clkout_freq());
    field_clkout_freq.on_change = [this](SymField&) {
        if (field_clkout_freq.to_integer() < 4)  // Min. CLK out of Si5351A/B/C-B is 2.5khz , but in our application -intermediate freq 800Mhz-,Min working CLK=4khz.
            field_clkout_freq.set_value(4);
        if (field_clkout_freq.to_integer() > 60000)
            field_clkout_freq.set_value(60000);
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
        .freq = static_cast<uint32_t>(field_clkout_freq.to_integer()),
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
                  &toggle_fake_brightness,
                  &toggle_sd_card,
                  &button_save,
                  &button_cancel});

    // Display "Disable speaker" option only if AK4951 Codec which has separate speaker/headphone control
    if (audio::speaker_disable_supported()) {
        add_child(&toggle_speaker);
    }
    if (battery::BatteryManagement::isDetected()) {
        add_child(&toggle_battery_icon);
        add_child(&toggle_battery_text);
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
    toggle_fake_brightness.set_value(!pmem::ui_hide_fake_brightness());
    toggle_battery_icon.set_value(!pmem::ui_hide_battery_icon());
    toggle_battery_text.set_value(!pmem::ui_hide_numeric_battery());
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
        pmem::set_ui_hide_fake_brightness(!toggle_fake_brightness.value());
        pmem::set_ui_hide_battery_icon(!toggle_battery_icon.value());
        pmem::set_ui_hide_numeric_battery(!toggle_battery_text.value());
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

    text_pmem_status.set_style(Theme::getInstance()->fg_yellow);

    check_use_sdcard_for_pmem.set_value(pmem::should_use_sdcard_for_pmem());
    check_use_sdcard_for_pmem.on_select = [this](Checkbox&, bool v) {
        File pmem_flag_file_handle;
        if (v) {
            if (fs::file_exists(settings_dir / PMEM_FILEFLAG)) {
                text_pmem_status.set("P.Mem flag file present.");
            } else {
                auto error = pmem_flag_file_handle.create(settings_dir / PMEM_FILEFLAG);
                if (error)
                    text_pmem_status.set("Error creating P.Mem File!");
                else
                    text_pmem_status.set("P.Mem flag file created.");
            }
        } else {
            auto result = delete_file(settings_dir / PMEM_FILEFLAG);
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
                    // Refresh status bar
                    send_system_refresh();
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
                  &checkbox_beep_on_packets,
                  &button_save,
                  &button_cancel});

    field_tone_mix.set_value(pmem::tone_mix());

    checkbox_beep_on_packets.set_value(pmem::beep_on_packets());

    button_save.on_select = [&nav, this](Button&) {
        pmem::set_tone_mix(field_tone_mix.value());
        pmem::set_beep_on_packets(checkbox_beep_on_packets.value());
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

/* SetEncoderDialView ************************************/

SetEncoderDialView::SetEncoderDialView(NavigationView& nav) {
    add_children({&labels,
                  &field_encoder_dial_sensitivity,
                  &field_encoder_rate_multiplier,
                  &button_save,
                  &button_cancel,
                  &button_dial_sensitivity_plus,
                  &button_dial_sensitivity_minus,
                  &button_rate_multiplier_plus,
                  &button_rate_multiplier_minus,
                  &field_encoder_dial_direction});

    field_encoder_dial_sensitivity.set_by_value(pmem::encoder_dial_sensitivity());
    field_encoder_rate_multiplier.set_value(pmem::encoder_rate_multiplier());
    field_encoder_dial_direction.set_by_value(pmem::encoder_dial_direction());

    button_dial_sensitivity_plus.on_select = [this](Button&) {
        field_encoder_dial_sensitivity.on_encoder(1);
    };
    button_dial_sensitivity_minus.on_select = [this](Button&) {
        field_encoder_dial_sensitivity.on_encoder(-1);
    };
    button_rate_multiplier_plus.on_select = [this](Button&) {
        field_encoder_rate_multiplier.on_encoder(1);
    };
    button_rate_multiplier_minus.on_select = [this](Button&) {
        field_encoder_rate_multiplier.on_encoder(-1);
    };

    button_save.on_select = [&nav, this](Button&) {
        pmem::set_encoder_dial_sensitivity(field_encoder_dial_sensitivity.selected_index_value());
        pmem::set_encoder_rate_multiplier(field_encoder_rate_multiplier.value());
        pmem::set_encoder_dial_direction(field_encoder_dial_direction.selected_index_value());
        nav.pop();
    };

    button_cancel.on_select = [&nav, this](Button&) {
        nav.pop();
    };
}

void SetEncoderDialView::focus() {
    button_save.focus();
}

/* SetButtonsView ************************************/

SetButtonsView::SetButtonsView(NavigationView& nav) {
    add_children({&labels,
                  &button_save,
                  &button_cancel,
                  &field_repeat_delay,
                  &field_repeat_speed,
                  &field_long_press_delay});

    field_repeat_delay.set_by_value(pmem::ui_button_repeat_delay());
    field_repeat_speed.set_by_value(pmem::ui_button_repeat_speed());
    field_long_press_delay.set_by_value(pmem::ui_button_long_press_delay());

    button_save.on_select = [&nav, this](Button&) {
        pmem::set_ui_button_repeat_delay(field_repeat_delay.selected_index_value());
        pmem::set_ui_button_repeat_speed(field_repeat_speed.selected_index_value());
        pmem::set_ui_button_long_press_delay(field_long_press_delay.selected_index_value());
        nav.pop();
    };

    button_cancel.on_select = [&nav, this](Button&) {
        nav.pop();
    };
}

void SetButtonsView::focus() {
    button_save.focus();
}

/* AppSettingsView ************************************/

AppSettingsView::AppSettingsView(
    NavigationView& nav)
    : nav_{nav} {
    add_children({&labels,
                  &menu_view});

    menu_view.set_parent_rect({0, 3 * 8, 240, 33 * 8});

    ensure_directory(settings_dir);

    for (const auto& entry : std::filesystem::directory_iterator(settings_dir, u"*.ini")) {
        auto path = settings_dir / entry.path();

        menu_view.add_item({path.filename().string().substr(0, 26),
                            ui::Theme::getInstance()->fg_darkcyan->foreground,
                            &bitmap_icon_file_text,
                            [this, path](KeyEvent) {
                                nav_.push<TextEditorView>(path);
                            }});
    }
}

void AppSettingsView::focus() {
    menu_view.focus();
}

/* SetConfigModeView ************************************/

SetConfigModeView::SetConfigModeView(NavigationView& nav) {
    add_children({&labels,
                  &checkbox_config_mode_enabled,
                  &button_save,
                  &button_cancel});

    checkbox_config_mode_enabled.set_value(!pmem::config_disable_config_mode());

    button_save.on_select = [&nav, this](Button&) {
        pmem::set_config_disable_config_mode(!checkbox_config_mode_enabled.value());
        nav.pop();
    };

    button_cancel.on_select = [&nav, this](Button&) {
        nav.pop();
    };
}

void SetConfigModeView::focus() {
    button_save.focus();
}

/* SetDisplayView ************************************/

SetDisplayView::SetDisplayView(NavigationView& nav) {
    add_children({&labels,
                  &field_fake_brightness,
                  &button_save,
                  &button_cancel,
                  &checkbox_invert_switch,
                  &checkbox_brightness_switch});

    field_fake_brightness.set_by_value(pmem::fake_brightness_level());
    checkbox_brightness_switch.set_value(pmem::apply_fake_brightness());
    checkbox_invert_switch.set_value(pmem::config_lcd_inverted_mode());

    button_save.on_select = [&nav, this](Button&) {
        pmem::set_apply_fake_brightness(checkbox_brightness_switch.value());
        pmem::set_fake_brightness_level(field_fake_brightness.selected_index_value());
        if (checkbox_invert_switch.value() != pmem::config_lcd_inverted_mode()) {
            display.set_inverted(checkbox_invert_switch.value());
            pmem::set_lcd_inverted_mode(checkbox_invert_switch.value());
        }
        send_system_refresh();
        nav.pop();
    };

    // only enable invert OR fake brightness
    checkbox_invert_switch.on_select = [this](Checkbox&, bool v) {
        if (v) checkbox_brightness_switch.set_value(false);
    };
    checkbox_brightness_switch.on_select = [this](Checkbox&, bool v) {
        if (v) checkbox_invert_switch.set_value(false);
    };

    button_cancel.on_select = [&nav, this](Button&) {
        nav.pop();
    };
}

void SetDisplayView::focus() {
    button_save.focus();
}

/* SetTouchscreenSensitivityView ************************************/
/* sample max: 1023 sample_t AKA uint16_t
 * touch_sensitivity: range: 1 to 128
 * threshold = 1023 / sensitive
 * threshold range: 1023/1 to 1023/128  =  1023 to 8
 */
SetTouchscreenThresholdView::SetTouchscreenThresholdView(NavigationView& nav) {
    add_children({&labels,
                  &field_threshold,
                  &button_autodetect,
                  &button_reset,
                  &button_save,
                  &button_cancel,
                  &text_hint,
                  &text_wait_timer});

    set_dirty();
    org_threshold = portapack::touch_threshold;
    field_threshold.set_value(pmem::touchscreen_threshold());
    text_hint.set_style(Theme::getInstance()->error_dark);
    text_hint.hidden(true);
    text_wait_timer.set_style(Theme::getInstance()->error_dark);
    text_wait_timer.hidden(true);
    // clang-format off
    button_autodetect.on_select = [this, &nav](Button&) {
        nav.display_modal("NOTICE",
                          "Now on don't touch screen;\n"
                          "Use arrow keys to operate.\n"
                          "Follow instructions.\n"
                          "Press YES to continue",
                          YESNO, [this, &nav](bool choice) {
                if (choice){
                        time_start_auto_detect = chTimeNow();
                        text_hint.hidden(false);
                        text_wait_timer.hidden(false);
                        text_wait_timer.set("ETA " + to_string_dec_uint(10) + "s");
                        in_auto_detect = true;
                        field_threshold.set_value(1);
                        portapack::touch_threshold = 1;
                        set_dirty(); } }, TRUE);
    };
    // clang-format on

    button_reset.on_select = [this](Button&) {
        field_threshold.set_value(32);
        portapack::touch_threshold = 32;
    };

    button_save.on_select = [&nav, this](Button&) {
        pmem::set_touchscreen_threshold(field_threshold.value());
        portapack::touch_threshold = field_threshold.value();
        send_system_refresh();
        nav.pop();
    };

    button_cancel.on_select = [&nav, this](Button&) {
        portapack::touch_threshold = org_threshold;
        nav.pop();
    };
}

void SetTouchscreenThresholdView::focus() {
    button_autodetect.focus();
    set_dirty();
}

void SetTouchscreenThresholdView::on_frame_sync() {
    if (!in_auto_detect) return;
    uint32_t time_now = chTimeNow();
    int32_t time_diff = time_now - time_start_auto_detect;
    text_wait_timer.set("ETA " + to_string_dec_uint((10 - time_diff / 1000) <= 0 ? 0 : 10 - time_diff / 1000) + "s");
    if (time_diff >= 10001 && !auto_detect_succeed_consumed) {  // 10s
        in_auto_detect = false;
        text_wait_timer.hidden(true);
        text_hint.set("OK, press save and reboot");
        portapack::touch_threshold = org_threshold;
        pmem::set_touchscreen_threshold(org_threshold);
        set_dirty();
        auto_detect_succeed_consumed = true;
        button_save.focus();
        return;
    }
    if (get_touch_frame().touch) {
        if (in_auto_detect) {
            uint16_t sen = field_threshold.value();
            sen++;
            portapack::touch_threshold = sen;
            field_threshold.set_value(sen);
        }
    }
}

SetTouchscreenThresholdView::~SetTouchscreenThresholdView() {
    // it seems that sometimes in the msg handler func it would enter the condi that not possible to entered,
    // so added this workaround.
    // TODO: find out why
    in_auto_detect = false;
    auto_detect_succeed_consumed = false;
    time_start_auto_detect = 0;
}

/* SetMenuColorView ************************************/

void SetMenuColorView::paint_sample() {
    Color c = Color(field_red_level.value(), field_green_level.value(), field_blue_level.value());
    button_sample.set_bg_color(c);
}

SetMenuColorView::SetMenuColorView(NavigationView& nav) {
    add_children({&labels,
                  &button_sample,
                  &field_red_level,
                  &field_green_level,
                  &field_blue_level,
                  &button_save,
                  &button_reset,
                  &button_cancel});

    button_sample.set_focusable(false);

    Color c = pmem::menu_color();
    field_red_level.set_value(c.r());
    field_green_level.set_value(c.g());
    field_blue_level.set_value(c.b());
    paint_sample();

    const auto color_changed_fn = [this](int32_t) {
        paint_sample();
    };
    field_red_level.on_change = color_changed_fn;
    field_green_level.on_change = color_changed_fn;
    field_blue_level.on_change = color_changed_fn;

    button_reset.on_select = [&nav, this](Button&) {
        field_red_level.set_value(127);
        field_green_level.set_value(127);
        field_blue_level.set_value(127);
        set_dirty();
    };

    button_save.on_select = [&nav, this](Button&) {
        Color c = Color(field_red_level.value(), field_green_level.value(), field_blue_level.value());
        pmem::set_menu_color(c);
        send_system_refresh();
        nav.pop();
    };

    button_cancel.on_select = [&nav, this](Button&) {
        nav.pop();
    };
}

void SetMenuColorView::focus() {
    button_save.focus();
}

/* SetThemeView ************************************/

SetThemeView::SetThemeView(NavigationView& nav) {
    add_children({&labels,
                  &button_save,
                  &button_cancel,
                  &options,
                  &checkbox_menuset});

    button_save.on_select = [&nav, this](Button&) {
        if (selected < Theme::ThemeId::MAX && (uint8_t)selected != portapack::persistent_memory::ui_theme_id()) {
            portapack::persistent_memory::set_ui_theme_id((uint8_t)selected);
            Theme::SetTheme((Theme::ThemeId)selected);
            if (checkbox_menuset.value()) {
                pmem::set_menu_color(Theme::getInstance()->bg_medium->background);
            }
            send_system_refresh();
        }
        nav.pop();
    };

    checkbox_menuset.set_value(true);

    button_cancel.on_select = [&nav, this](Button&) {
        nav.pop();
    };

    options.on_change = [this](size_t, OptionsField::value_t v) {
        selected = v;
    };
    options.set_selected_index(portapack::persistent_memory::ui_theme_id());
}

void SetThemeView::focus() {
    options.focus();
}

/* SetBatteryView ************************************/

SetBatteryView::SetBatteryView(NavigationView& nav) {
    add_children({&labels,
                  &button_save,
                  &button_cancel,
                  &checkbox_overridebatt,
                  &checkbox_battery_charge_hint});

    if (i2cdev::I2CDevManager::get_dev_by_model(I2C_DEVMDL::I2CDEVMDL_MAX17055)) add_children({&button_reset, &labels2});

    button_save.on_select = [&nav, this](Button&) {
        pmem::set_ui_override_batt_calc(checkbox_overridebatt.value());
        pmem::set_ui_battery_charge_hint(checkbox_battery_charge_hint.value());
        battery::BatteryManagement::set_calc_override(checkbox_overridebatt.value());
        send_system_refresh();
        nav.pop();
    };

    button_reset.on_select = [&nav, this](Button&) {
        auto dev = (i2cdev::I2cDev_MAX17055*)i2cdev::I2CDevManager::get_dev_by_model(I2C_DEVMDL::I2CDEVMDL_MAX17055);
        if (dev->reset_learned())
            nav.display_modal("Reset", "Battery parameters reset");
        else
            nav.display_modal("Error", "Error parameter reset");
    };

    checkbox_overridebatt.set_value(pmem::ui_override_batt_calc());
    checkbox_battery_charge_hint.set_value(pmem::ui_battery_charge_hint());

    button_cancel.on_select = [&nav, this](Button&) {
        nav.pop();
    };
}

void SetBatteryView::focus() {
    button_cancel.focus();
}

/* SettingsMenuView **************************************/

SettingsMenuView::SettingsMenuView(NavigationView& nav)
    : nav_(nav) {
    set_max_rows(2);  // allow wider buttons
}

void SettingsMenuView::on_populate() {
    const bool return_icon = pmem::show_gui_return_icon();

    if (return_icon) {
        add_items({{"..", ui::Color::light_grey(), &bitmap_icon_previous, [this]() { nav_.pop(); }}});
    }

    add_items({
        {"App Settings", ui::Color::dark_cyan(), &bitmap_icon_notepad, [this]() { nav_.push<AppSettingsView>(); }},
        {"Audio", ui::Color::dark_cyan(), &bitmap_icon_speaker, [this]() { nav_.push<SetAudioView>(); }},
        {"Calibration", ui::Color::dark_cyan(), &bitmap_icon_options_touch, [this]() { nav_.push<TouchCalibrationView>(); }},
        {"TouchThreshold", ui::Color::dark_cyan(), &bitmap_icon_options_touch, [this]() { nav_.push<SetTouchscreenThresholdView>(); }},
        {"Config Mode", ui::Color::dark_cyan(), &bitmap_icon_clk_ext, [this]() { nav_.push<SetConfigModeView>(); }},
        {"Converter", ui::Color::dark_cyan(), &bitmap_icon_options_radio, [this]() { nav_.push<SetConverterSettingsView>(); }},
        {"Date/Time", ui::Color::dark_cyan(), &bitmap_icon_options_datetime, [this]() { nav_.push<SetDateTimeView>(); }},
        {"Encoder Dial", ui::Color::dark_cyan(), &bitmap_icon_setup, [this]() { nav_.push<SetEncoderDialView>(); }},
        {"Button Speed", ui::Color::dark_cyan(), &bitmap_icon_controls, [this]() { nav_.push<SetButtonsView>(); }},
        {"Freq. Correct", ui::Color::dark_cyan(), &bitmap_icon_options_radio, [this]() { nav_.push<SetFrequencyCorrectionView>(); }},
        {"P.Memory Mgmt", ui::Color::dark_cyan(), &bitmap_icon_memory, [this]() { nav_.push<SetPersistentMemoryView>(); }},
        {"Radio", ui::Color::dark_cyan(), &bitmap_icon_options_radio, [this]() { nav_.push<SetRadioView>(); }},
        {"SD Card", ui::Color::dark_cyan(), &bitmap_icon_sdcard, [this]() { nav_.push<SetSDCardView>(); }},
        {"User Interface", ui::Color::dark_cyan(), &bitmap_icon_options_ui, [this]() { nav_.push<SetUIView>(); }},
        {"Display", ui::Color::dark_cyan(), &bitmap_icon_brightness, [this]() { nav_.push<SetDisplayView>(); }},
        {"Menu Color", ui::Color::dark_cyan(), &bitmap_icon_brightness, [this]() { nav_.push<SetMenuColorView>(); }},
        {"Theme", ui::Color::dark_cyan(), &bitmap_icon_setup, [this]() { nav_.push<SetThemeView>(); }},
    });

    if (battery::BatteryManagement::isDetected()) add_item({"Battery", ui::Color::dark_cyan(), &bitmap_icon_batt_icon, [this]() { nav_.push<SetBatteryView>(); }});

    add_external_items(nav_, app_location_t::SETTINGS, *this, return_icon ? 1 : 0);
}

} /* namespace ui */
