/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "ui_debug_pmem.hpp"

namespace ui::external_app::debug_pmem {

// Dump pmem, receiver and transmitter models internals in human readable format
bool DebugDumpView::debug_dump_func() {
    std::string debug_dir = "DEBUG";
    std::filesystem::path filename{};
    File pmem_dump_file{};
    // create new dump file name and DEBUG directory
    ensure_directory(debug_dir);
    filename = next_filename_matching_pattern(debug_dir + "/DEBUG_DUMP_????.TXT");
    if (filename.empty()) {
        dump_output.set("COULD NOT GET DUMP NAME !");
        dump_output.set_style(ui::Theme::getInstance()->fg_red);
        return false;
    }
    // dump data fo filename
    auto error = pmem_dump_file.create(filename);
    if (error) {
        dump_output.set("ERROR DUMPING " + filename.filename().string() + " !");
        dump_output.set_style(ui::Theme::getInstance()->fg_red);
        return false;
    }
    pmem_dump_file.write_line("FW version: " VERSION_STRING);
    pmem_dump_file.write_line("Ext APPS version req'd: 0x" + to_string_hex(VERSION_MD5));
    pmem_dump_file.write_line("GCC version: " + to_string_dec_int(__GNUC__) + "." + to_string_dec_int(__GNUC_MINOR__) + "." + to_string_dec_int(__GNUC_PATCHLEVEL__));

    // firmware checksum
    pmem_dump_file.write_line("Firmware calculated checksum: 0x" + to_string_hex(simple_checksum(FLASH_STARTING_ADDRESS, FLASH_ROM_SIZE), 8));

    // write persistent memory
    pmem_dump_file.write_line("\n[Persistent Memory]");

    // full variables
    pmem_dump_file.write_line("structure_version: 0x" + to_string_hex(get_data_structure_version(), 8));
    pmem_dump_file.write_line("target_frequency: " + to_string_dec_int(target_frequency()));
    pmem_dump_file.write_line("correction_ppb: " + to_string_dec_int(correction_ppb()));
    pmem_dump_file.write_line("modem_def_index: " + to_string_dec_uint(get_modem_def_index()));
    pmem_dump_file.write_line("serial_format.data_bit: " + to_string_dec_uint(serial_format().data_bits));
    pmem_dump_file.write_line("serial_format.parity: " + to_string_dec_uint(serial_format().parity));
    pmem_dump_file.write_line("serial_format.stop_bits: " + to_string_dec_uint(serial_format().stop_bits));
    pmem_dump_file.write_line("serial_format.bit_order: " + to_string_dec_uint(serial_format().bit_order));
    pmem_dump_file.write_line("modem_bw: " + to_string_dec_int(modem_bw()));
    pmem_dump_file.write_line("afsk_mark_freq: " + to_string_dec_int(afsk_mark_freq()));
    pmem_dump_file.write_line("afsk_space_freq: " + to_string_dec_int(afsk_space_freq()));
    pmem_dump_file.write_line("modem_baudrate: " + to_string_dec_int(modem_baudrate()));
    pmem_dump_file.write_line("modem_repeat: " + to_string_dec_int(modem_repeat()));
    pmem_dump_file.write_line("pocsag_last_address: " + to_string_dec_uint(pocsag_last_address()));
    pmem_dump_file.write_line("pocsag_ignore_address: " + to_string_dec_uint(pocsag_ignore_address()));
    pmem_dump_file.write_line("tone_mix: " + to_string_dec_uint(tone_mix()));
    pmem_dump_file.write_line("hardware_config: " + to_string_dec_uint(config_cpld()));
    pmem_dump_file.write_line("recon_config: 0x" + to_string_hex(get_recon_config(), 16));
    pmem_dump_file.write_line("recon_repeat_nb: " + to_string_dec_int(recon_repeat_nb()));
    pmem_dump_file.write_line("recon_repeat_gain: " + to_string_dec_int(recon_repeat_gain()));
    pmem_dump_file.write_line("recon_repeat_delay: " + to_string_dec_int(recon_repeat_delay()));
    pmem_dump_file.write_line("converter: " + to_string_dec_int(config_converter()));
    pmem_dump_file.write_line("updown_converter: " + to_string_dec_int(config_updown_converter()));
    pmem_dump_file.write_line("updown_frequency_rx_correction: " + to_string_dec_int(config_freq_rx_correction_updown()));
    pmem_dump_file.write_line("updown_frequency_tx_correction: " + to_string_dec_int(config_freq_tx_correction_updown()));
    pmem_dump_file.write_line("lcd_normally_black: " + to_string_dec_uint(config_lcd_normally_black()));
    pmem_dump_file.write_line("converter_frequency_offset: " + to_string_dec_int(config_converter_freq()));
    pmem_dump_file.write_line("frequency_rx_correction: " + to_string_dec_uint(config_freq_rx_correction()));
    pmem_dump_file.write_line("frequency_tx_correction: " + to_string_dec_uint(config_freq_tx_correction()));
    pmem_dump_file.write_line("encoder_dial_sensitivity: " + to_string_dec_uint(encoder_dial_sensitivity()));
    pmem_dump_file.write_line("encoder_rate_multiplier: " + to_string_dec_uint(encoder_rate_multiplier()));
    pmem_dump_file.write_line("encoder_dial_direction: " + to_string_dec_uint(encoder_dial_direction()));  // 0 = normal, 1 = reverse
    pmem_dump_file.write_line("config_mode_storage: 0x" + to_string_hex(config_mode_storage_direct(), 8));
    pmem_dump_file.write_line("dst_config: 0x" + to_string_hex((uint32_t)config_dst().v, 8));
    pmem_dump_file.write_line("fake_brightness_level: " + to_string_dec_uint(fake_brightness_level()));
    pmem_dump_file.write_line("menu_color: 0x" + to_string_hex(menu_color().v, 4));
    pmem_dump_file.write_line("touchscreen_threshold: " + to_string_dec_uint(touchscreen_threshold()));

    // ui_config bits
    const auto backlight_timer = portapack::persistent_memory::config_backlight_timer();
    pmem_dump_file.write_line("ui_config clkout_freq: " + to_string_dec_uint(clkout_freq()));
    pmem_dump_file.write_line("ui_config backlight_timer.timeout_enabled: " + to_string_dec_uint(backlight_timer.timeout_enabled()));
    pmem_dump_file.write_line("ui_config backlight_timer.timeout_seconds: " + to_string_dec_uint(backlight_timer.timeout_seconds()));
    pmem_dump_file.write_line("ui_config show_gui_return_icon: " + to_string_dec_uint(show_gui_return_icon()));
    pmem_dump_file.write_line("ui_config load_app_settings: " + to_string_dec_uint(load_app_settings()));
    pmem_dump_file.write_line("ui_config save_app_settings: " + to_string_dec_uint(save_app_settings()));
    pmem_dump_file.write_line("ui_config disable_touchscreen: " + to_string_dec_uint(disable_touchscreen()));
    pmem_dump_file.write_line("ui_config hide_clock: " + to_string_dec_uint(hide_clock()));
    pmem_dump_file.write_line("ui_config clock_with_date: " + to_string_dec_uint(clock_with_date()));
    pmem_dump_file.write_line("ui_config clkout_enabled: " + to_string_dec_uint(clkout_enabled()));
    pmem_dump_file.write_line("ui_config apply_fake_brightness: " + to_string_dec_uint(apply_fake_brightness()));
    pmem_dump_file.write_line("ui_config stealth_mode: " + to_string_dec_uint(stealth_mode()));
    pmem_dump_file.write_line("ui_config config_login: " + to_string_dec_uint(config_login()));
    pmem_dump_file.write_line("ui_config config_splash: " + to_string_dec_uint(config_splash()));

    // ui_config2 bits
    pmem_dump_file.write_line("ui_config2 hide_speaker: " + to_string_dec_uint(ui_hide_speaker()));
    pmem_dump_file.write_line("ui_config2 hide_converter: " + to_string_dec_uint(ui_hide_converter()));
    pmem_dump_file.write_line("ui_config2 hide_stealth: " + to_string_dec_uint(ui_hide_stealth()));
    pmem_dump_file.write_line("ui_config2 hide_camera: " + to_string_dec_uint(ui_hide_camera()));
    pmem_dump_file.write_line("ui_config2 hide_sleep: " + to_string_dec_uint(ui_hide_sleep()));
    pmem_dump_file.write_line("ui_config2 hide_bias_tee: " + to_string_dec_uint(ui_hide_bias_tee()));
    pmem_dump_file.write_line("ui_config2 hide_clock: " + to_string_dec_uint(ui_hide_clock()));
    pmem_dump_file.write_line("ui_config2 hide_sd_card: " + to_string_dec_uint(ui_hide_sd_card()));
    pmem_dump_file.write_line("ui_config2 hide_mute: " + to_string_dec_uint(ui_hide_mute()));
    pmem_dump_file.write_line("ui_config2 hide_fake_brightness: " + to_string_dec_uint(ui_hide_fake_brightness()));
    pmem_dump_file.write_line("ui_config2 hide_battery_icon: " + to_string_dec_uint(ui_hide_battery_icon()));
    pmem_dump_file.write_line("ui_config2 hide_numeric_battery: " + to_string_dec_uint(ui_hide_numeric_battery()));
    pmem_dump_file.write_line("ui_config2 theme_id: " + to_string_dec_uint(ui_theme_id()));
    pmem_dump_file.write_line("ui_config2 override_batt_calc: " + to_string_dec_uint(ui_override_batt_calc()));
    pmem_dump_file.write_line("ui_config2 button_repeat_delay: " + to_string_dec_uint(ui_button_repeat_delay()));
    pmem_dump_file.write_line("ui_config2 button_repeat_speed: " + to_string_dec_uint(ui_button_repeat_speed()));
    pmem_dump_file.write_line("ui_config2 button_long_press_delay: " + to_string_dec_uint(ui_button_long_press_delay()));
    pmem_dump_file.write_line("ui_config2 battery_charge_hint: " + to_string_dec_uint(ui_battery_charge_hint()));

    // misc_config bits
    pmem_dump_file.write_line("misc_config config_audio_mute: " + to_string_dec_int(config_audio_mute()));
    pmem_dump_file.write_line("misc_config config_speaker_disable: " + to_string_dec_int(config_speaker_disable()));
    pmem_dump_file.write_line("misc_config config_disable_external_tcxo: " + to_string_dec_uint(config_disable_external_tcxo()));
    pmem_dump_file.write_line("misc_config config_sdcard_high_speed_io: " + to_string_dec_uint(config_sdcard_high_speed_io()));
    pmem_dump_file.write_line("misc_config config_disable_config_mode: " + to_string_dec_uint(config_disable_config_mode()));
    pmem_dump_file.write_line("misc_config beep_on_packets: " + to_string_dec_int(beep_on_packets()));

    // receiver_model
    pmem_dump_file.write_line("\n[Receiver Model]");
    pmem_dump_file.write_line("target_frequency: " + to_string_dec_uint(receiver_model.target_frequency()));
    pmem_dump_file.write_line("frequency_step: " + to_string_dec_uint(receiver_model.frequency_step()));
    pmem_dump_file.write_line("lna: " + to_string_dec_int(receiver_model.lna()));
    pmem_dump_file.write_line("vga: " + to_string_dec_int(receiver_model.vga()));
    pmem_dump_file.write_line("rf_amp: " + to_string_dec_int(receiver_model.rf_amp()));
    pmem_dump_file.write_line("baseband_bandwidth: " + to_string_dec_uint(receiver_model.baseband_bandwidth()));
    pmem_dump_file.write_line("sampling_rate: " + to_string_dec_uint(receiver_model.sampling_rate()));
    switch (receiver_model.modulation()) {
        case ReceiverModel::Mode::AMAudio:
            pmem_dump_file.write_line("modulation: Mode::AMAudio");
            break;
        case ReceiverModel::Mode::NarrowbandFMAudio:
            pmem_dump_file.write_line("modulation: Mode::NarrowbandFMAudio");
            break;
        case ReceiverModel::Mode::WidebandFMAudio:
            pmem_dump_file.write_line("modulation: Mode::WidebandFMAudio");
            break;
        case ReceiverModel::Mode::WFMAudioAMApt:
            pmem_dump_file.write_line("modulation: Mode::WFMAudioAMApt");
            break;
        case ReceiverModel::Mode::SpectrumAnalysis:
            pmem_dump_file.write_line("modulation: Mode::SpectrumAnalysis");
            break;
        case ReceiverModel::Mode::Capture:
            pmem_dump_file.write_line("modulation: Mode::Capture");
            break;
        case ReceiverModel::Mode::AMAudioFMApt:
            pmem_dump_file.write_line("modulation: Mode::AMAudioFMApt");
            break;
        default:
            pmem_dump_file.write_line("modulation: !!unknown mode!!");
            break;
    }
    pmem_dump_file.write_line("headphone_volume.centibel: " + to_string_dec_int(receiver_model.headphone_volume().centibel()));
    pmem_dump_file.write_line("normalized_headphone_volume: " + to_string_dec_uint(receiver_model.normalized_headphone_volume()));
    pmem_dump_file.write_line("am_configuration: " + to_string_dec_uint(receiver_model.am_configuration()));
    pmem_dump_file.write_line("nbfm_configuration: " + to_string_dec_uint(receiver_model.nbfm_configuration()));
    pmem_dump_file.write_line("wfm_configuration: " + to_string_dec_uint(receiver_model.wfm_configuration()));

    // transmitter_model
    pmem_dump_file.write_line("\n[Transmitter Model]");
    pmem_dump_file.write_line("target_frequency: " + to_string_dec_uint(transmitter_model.target_frequency()));
    pmem_dump_file.write_line("rf_amp: " + to_string_dec_int(transmitter_model.rf_amp()));
    pmem_dump_file.write_line("baseband_bandwidth: " + to_string_dec_uint(transmitter_model.baseband_bandwidth()));
    pmem_dump_file.write_line("sampling_rate: " + to_string_dec_uint(transmitter_model.sampling_rate()));
    pmem_dump_file.write_line("tx_gain: " + to_string_dec_int(transmitter_model.tx_gain()));
    pmem_dump_file.write_line("channel_bandwidth: " + to_string_dec_uint(transmitter_model.channel_bandwidth()));
    // on screen information
    dump_output.set(filename.filename().string() + " DUMPED !");
    dump_output.set_style(ui::Theme::getInstance()->fg_green);

    return true;
}

DebugDumpView::DebugDumpView(NavigationView& nav)
    : nav_{nav} {
    add_children({&dump_output,
                  &button_exit});

    debug_dump_func();

    button_exit.on_select = [this](Button&) {
        nav_.pop();
    };
}

void DebugDumpView::focus() {
    button_exit.focus();
}

}  // namespace ui::external_app::debug_pmem
