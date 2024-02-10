/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2023 gullradriel, Nilorea Studio Inc.
 * Copyright (C) 2023 Kyle Reed
 * Copyright (C) 2024 Mark Thompson
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

#ifndef __UI_SETTINGS_H__
#define __UI_SETTINGS_H__

#include "ui_widget.hpp"
#include "ui_menu.hpp"
#include "ui_receiver.hpp"
#include "ui_navigation.hpp"
#include "bitmap.hpp"
#include "ff.h"
#include "portapack_persistent_memory.hpp"

#include <cstdint>

namespace ui {

struct SetDateTimeModel {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    portapack::persistent_memory::dst_config_t dst;
};

class SetDateTimeView : public View {
   public:
    SetDateTimeView(NavigationView& nav);

    void focus() override;

    std::string title() const override { return "Date/Time"; };

   private:
    using option_t = std::pair<std::string, int32_t>;
    std::vector<option_t> which_options = {{"1st", 0}, {"2nd", 1}, {"3rd", 2}, {"4th", 3}, {"Last", 4}};
    std::vector<option_t> weekday_options = {{"Sun", 0}, {"Mon", 1}, {"Tue", 2}, {"Wed", 3}, {"Thu", 4}, {"Fri", 5}, {"Sat", 6}};
    std::vector<option_t> month_options = {{"Jan", 1}, {"Feb", 2}, {"Mar", 3}, {"Apr", 4}, {"May", 5}, {"Jun", 6}, {"Jul", 7}, {"Aug", 8}, {"Sep", 9}, {"Oct", 10}, {"Nov", 11}, {"Dec", 12}};

    Labels labels{
        {{1 * 8, 1 * 16}, "Adjust the RTC clock date &", Color::light_grey()},
        {{1 * 8, 2 * 16}, "time. If clock resets after", Color::light_grey()},
        {{1 * 8, 3 * 16}, "reboot, coin batt. is dead. ", Color::light_grey()},
        {{1 * 8, 5 * 16 - 2}, "YYYY-MM-DD HH:MM:SS  DoW DoY", Color::grey()},
        {{5 * 8, 6 * 16}, "-  -     :  :", Color::light_grey()},
        {{1 * 8, 11 * 16}, "DST adds 1 hour to RTC time.", Color::light_grey()},
        {{0 * 8, 12 * 16}, "Start: 0:00 on Nth  DDD in", Color::light_grey()},
        {{0 * 8, 13 * 16}, "End:   1:00 on Nth  DDD in", Color::light_grey()}};

    NumberField field_year{
        {1 * 8, 6 * 16},
        4,
        {2015, 2099},
        1,
        '0',
        true,
    };
    NumberField field_month{
        {6 * 8, 6 * 16},
        2,
        {1, 12},
        1,
        '0',
        true,
    };
    NumberField field_day{
        {9 * 8, 6 * 16},
        2,
        {1, 31},
        1,
        '0',
        true,
    };

    NumberField field_hour{
        {12 * 8, 6 * 16},
        2,
        {0, 23},
        1,
        '0',
        true,
    };
    NumberField field_minute{
        {15 * 8, 6 * 16},
        2,
        {0, 59},
        1,
        '0',
        true,
    };
    NumberField field_second{
        {18 * 8, 6 * 16},
        2,
        {0, 59},
        1,
        '0',
        true,
    };
    Text text_weekday{
        {22 * 8, 6 * 16, 3 * 8, 16},
        ""};
    Text text_day_of_year{
        {26 * 8, 6 * 16, 3 * 8, 16},
        ""};
    Text text_in_dst_range{
        {17 * 8, 7 * 16, 3 * 8, 16},
        ""};

    Checkbox checkbox_dst_enable{
        {2 * 8, 9 * 16},
        23,
        "Enable Daylight Savings"};

    OptionsField options_dst_start_which{
        {15 * 8, 12 * 16},
        4,
        {}};

    OptionsField options_dst_start_weekday{
        {20 * 8, 12 * 16},
        3,
        {}};

    OptionsField options_dst_start_month{
        {27 * 8, 12 * 16},
        3,
        {}};

    OptionsField options_dst_end_which{
        {15 * 8, 13 * 16},
        4,
        {}};

    OptionsField options_dst_end_weekday{
        {20 * 8, 13 * 16},
        3,
        {}};

    OptionsField options_dst_end_month{
        {27 * 8, 13 * 16},
        3,
        {}};

    Button button_save{
        {2 * 8, 16 * 16, 12 * 8, 32},
        "Save"};
    Button button_cancel{
        {16 * 8, 16 * 16, 12 * 8, 32},
        "Cancel"};

    void form_init(const SetDateTimeModel& model);
    SetDateTimeModel form_collect();
    portapack::persistent_memory::dst_config_t dst_collect();
    void handle_date_field_update();
};

struct SetFrequencyCorrectionModel {
    int8_t ppm;
    uint32_t freq;
};

class SetRadioView : public View {
   public:
    SetRadioView(NavigationView& nav);

    void focus() override;

    std::string title() const override { return "Radio"; };

   private:
    uint8_t freq_step_khz = 3;

    Text label_source{
        {1 * 8, 1 * 16, 17 * 8, 16},
        "Reference Source:"};

    Text value_source{
        {18 * 8, 1 * 16, 11 * 8, 16},
        ""};

    Text value_source_frequency{
        {18 * 8, 2 * 16, 11 * 8, 16},
        ""};

    Labels labels_correction{
        {{2 * 8, 3 * 16}, "Frequency correction:", Color::light_grey()},
        {{6 * 8, 4 * 16}, "PPM", Color::light_grey()},
    };

    NumberField field_ppm{
        {2 * 8, 4 * 16},
        3,
        {-50, 50},
        1,
        '0',
    };

    Checkbox check_clkout{
        {18, (6 * 16 - 4)},
        13,
        "Enable CLKOUT"};

    SymField field_clkout_freq{
        {20 * 8, 6 * 16},
        5,
        SymField::Type::Dec};

    Labels labels_clkout_khz{
        {{26 * 8, 6 * 16}, "kHz", Color::light_grey()}};

    Labels labels_bias{
        {{4 * 8 + 4, 8 * 16}, "CAUTION: Ensure that all", Color::red()},
        {{5 * 8 + 0, 9 * 16}, "devices attached to the", Color::red()},
        {{6 * 8 + 0, 10 * 16}, "antenna connector can", Color::red()},
        {{6 * 8 + 4, 11 * 16}, "accept a DC voltage!", Color::red()}};

    Checkbox check_bias{
        {18, 12 * 16},
        5,
        "Enable DC bias voltage"};

    Checkbox disable_external_tcxo{
        {18, 14 * 16},
        5,
        "Disable external TCXO"};

    Button button_save{
        {2 * 8, 16 * 16, 12 * 8, 32},
        "Save"};
    Button button_cancel{
        {16 * 8, 16 * 16, 12 * 8, 32},
        "Cancel",
    };

    void form_init(const SetFrequencyCorrectionModel& model);
    SetFrequencyCorrectionModel form_collect();
};

using portapack::persistent_memory::backlight_timeout_t;

class SetUIView : public View {
   public:
    SetUIView(NavigationView& nav);

    void focus() override;

    std::string title() const override { return "UI"; };

   private:
    Checkbox checkbox_disable_touchscreen{
        {3 * 8, 1 * 16},
        20,
        "Disable touchscreen"};

    Checkbox checkbox_bloff{
        {3 * 8, 3 * 16},
        20,
        "Backlight off after:"};
    OptionsField options_bloff{
        {60, 4 * 16 + 8},
        20,
        {
            {"5 seconds", backlight_timeout_t::Timeout5Sec},
            {"15 seconds", backlight_timeout_t::Timeout15Sec},
            {"30 seconds", backlight_timeout_t::Timeout30Sec},
            {"1 minute", backlight_timeout_t::Timeout60Sec},
            {"3 minutes", backlight_timeout_t::Timeout180Sec},
            {"5 minutes", backlight_timeout_t::Timeout300Sec},
            {"10 minutes", backlight_timeout_t::Timeout600Sec},
            {"1 hour", backlight_timeout_t::Timeout3600Sec},
        }};

    Checkbox checkbox_showsplash{
        {3 * 8, 6 * 16},
        20,
        "Show splash"};

    Checkbox checkbox_showclock{
        {3 * 8, 8 * 16},
        20,
        "Show clock with:"};

    OptionsField options_clockformat{
        {60, 9 * 16 + 8},
        20,
        {{"time only", 0},
         {"time and date", 1}}};

    Checkbox checkbox_guireturnflag{
        {3 * 8, 11 * 16},
        20,
        "Back button in menu"};

    Labels labels{
        {{3 * 8, 13 * 16}, "Show/Hide Status Icons", Color::light_grey()},
    };

    ImageToggle toggle_camera{
        {6 * 8, 14 * 16 + 2, 16, 16},
        &bitmap_icon_camera};

    ImageToggle toggle_sleep{
        {8 * 8, 14 * 16 + 2, 16, 16},
        &bitmap_icon_sleep};

    ImageToggle toggle_stealth{
        {10 * 8, 14 * 16 + 2, 16, 16},
        &bitmap_icon_stealth};

    ImageToggle toggle_converter{
        {12 * 8, 14 * 16 + 2, 16, 16},
        &bitmap_icon_upconvert};

    ImageToggle toggle_bias_tee{
        {14 * 8, 14 * 16 + 2, 16, 16},
        &bitmap_icon_biast_off};

    ImageToggle toggle_clock{
        {16 * 8, 14 * 16 + 2, 8, 16},
        &bitmap_icon_clk_ext};

    ImageToggle toggle_mute{
        {17 * 8, 14 * 16 + 2, 16, 16},
        &bitmap_icon_speaker_and_headphones_mute};

    ImageToggle toggle_speaker{
        {19 * 8, 14 * 16 + 2, 16, 16},
        &bitmap_icon_speaker_mute};

    ImageToggle toggle_fake_brightness{
        {21 * 8, 14 * 16 + 2, 16, 16},
        &bitmap_icon_brightness};

    ImageToggle toggle_sd_card{
        {23 * 8, 14 * 16 + 2, 16, 16},
        &bitmap_sd_card_ok};

    Button button_save{
        {2 * 8, 16 * 16, 12 * 8, 32},
        "Save"};

    Button button_cancel{
        {16 * 8, 16 * 16, 12 * 8, 32},
        "Cancel"};
};

class SetSDCardView : public View {
   public:
    SetSDCardView(NavigationView& nav);

    void focus() override;

    std::string title() const override { return "SD Card"; };

   private:
    Labels labels{
        // 01234567890123456789012345678
        {{1 * 8, 120 - 48}, "    HIGH SPEED SDCARD IO     ", Color::light_grey()},
        {{1 * 8, 120 - 32}, "   May or may not work !!    ", Color::light_grey()}};

    Checkbox checkbox_sdcard_speed{
        {2 * 8, 120},
        20,
        "enable high speed IO"};

    Button button_test_sdcard_high_speed{
        {2 * 8, 152, 27 * 8, 32},
        "TEST BUTTON (NO PMEM SAVE)"};

    Text text_sdcard_test_status{
        {2 * 8, 198, 28 * 8, 16},
        ""};

    Button button_save{
        {2 * 8, 16 * 16, 12 * 8, 32},
        "Save"};

    Button button_cancel{
        {16 * 8, 16 * 16, 12 * 8, 32},
        "Cancel"};
};

class SetConverterSettingsView : public View {
   public:
    SetConverterSettingsView(NavigationView& nav);

    void focus() override;

    std::string title() const override { return "Converter"; };

   private:
    Labels labels{
        {{1 * 8, 1 * 16}, "Options for working with", Color::light_grey()},
        {{1 * 8, 2 * 16}, "up/down converter hardware", Color::light_grey()},
        {{1 * 8, 3 * 16}, "like a Ham It Up.", Color::light_grey()},
        {{2 * 8, 9 * 16 - 2}, "Conversion frequency:", Color::light_grey()},
        {{18 * 8, 10 * 16}, "MHz", Color::light_grey()},
    };

    Checkbox check_show_converter{
        {2 * 8, 5 * 16},
        19,
        "Show converter icon"};

    Checkbox check_converter{
        {2 * 8, 7 * 16},
        16,
        "Enable converter"};

    OptionsField opt_converter_mode{
        {5 * 8, 10 * 16},
        3,
        {
            {" + ", 0},  // up converter
            {" - ", 1},  // down converter
        }};

    FrequencyField field_converter_freq{
        {8 * 8, 10 * 16}};

    Button button_return{
        {16 * 8, 16 * 16, 12 * 8, 32},
        "Return",
    };
};

class SetFrequencyCorrectionView : public View {
   public:
    SetFrequencyCorrectionView(NavigationView& nav);

    void focus() override;

    std::string title() const override { return "Freq Correct"; };

   private:
    Labels labels{
        {{1 * 8, 1 * 16}, "Frequency correction allows", Color::light_grey()},
        {{1 * 8, 2 * 16}, "RX and TX frequencies to be", Color::light_grey()},
        {{1 * 8, 3 * 16}, "adjusted for all apps.", Color::light_grey()},
        {{2 * 8, 6 * 16}, "RX Adjustment Frequency", Color::light_grey()},
        {{18 * 8, 7 * 16}, "MHz", Color::light_grey()},
        {{2 * 8, 9 * 16}, "TX Adjustment Frequency", Color::light_grey()},
        {{18 * 8, 10 * 16}, "MHz", Color::light_grey()},
    };

    OptionsField opt_rx_correction_mode{
        {5 * 8, 7 * 16},
        3,
        {{" + ", 0},
         {" - ", 1}}};

    FrequencyField field_rx_correction{
        {8 * 8, 7 * 16}};

    OptionsField opt_tx_correction_mode{
        {5 * 8, 10 * 16},
        3,
        {{" + ", 0},
         {" - ", 1}}};

    FrequencyField field_tx_correction{
        {8 * 8, 10 * 16}};

    Button button_return{
        {16 * 8, 16 * 16, 12 * 8, 32},
        "Return",
    };
};

class SetAudioView : public View {
   public:
    SetAudioView(NavigationView& nav);

    void focus() override;

    std::string title() const override { return "Audio"; };

   private:
    Labels labels{
        {{1 * 8, 1 * 16}, "Controls the volume of the", Color::light_grey()},
        {{1 * 8, 2 * 16}, "tone when transmitting in", Color::light_grey()},
        {{1 * 8, 3 * 16}, "Soundboard or Mic apps.", Color::light_grey()},
        {{2 * 8, 5 * 16}, "Tone key mix:   %", Color::light_grey()},
    };

    NumberField field_tone_mix{
        {16 * 8, 5 * 16},
        2,
        {10, 99},
        1,
        '0'};

    Button button_save{
        {2 * 8, 16 * 16, 12 * 8, 32},
        "Save"};

    Button button_cancel{
        {16 * 8, 16 * 16, 12 * 8, 32},
        "Cancel",
    };
};

class SetQRCodeView : public View {
   public:
    SetQRCodeView(NavigationView& nav);

    void focus() override;

    std::string title() const override { return "QR Code"; };

   private:
    Labels labels{
        {{1 * 8, 1 * 16}, "Change the size of the QR", Color::light_grey()},
        {{1 * 8, 2 * 16}, "code shown in Radiosonde.", Color::light_grey()},
    };

    Checkbox checkbox_bigger_qr{
        {3 * 8, 4 * 16},
        20,
        "Show large QR code"};

    Button button_save{
        {2 * 8, 16 * 16, 12 * 8, 32},
        "Save"};

    Button button_cancel{
        {16 * 8, 16 * 16, 12 * 8, 32},
        "Cancel",
    };
};

using portapack::persistent_memory::encoder_dial_sensitivity;
using portapack::persistent_memory::encoder_rate_multiplier;

class SetEncoderDialView : public View {
   public:
    SetEncoderDialView(NavigationView& nav);

    void focus() override;

    std::string title() const override { return "Encoder Dial"; };

   private:
    Labels labels{
        {{1 * 8, 1 * 16}, "Adjusts sensitivity to dial", Color::light_grey()},
        {{1 * 8, 2 * 16}, "rotation position (number of", Color::light_grey()},
        {{1 * 8, 3 * 16}, "steps per full rotation):", Color::light_grey()},
        {{2 * 8, 5 * 16}, "Dial sensitivity:", Color::light_grey()},
        {{1 * 8, 8 * 16}, "Adjusts sensitivity to dial", Color::light_grey()},
        {{1 * 8, 9 * 16}, "rotation rate (default 1", Color::light_grey()},
        {{1 * 8, 10 * 16}, "means no rate dependency):", Color::light_grey()},
        {{3 * 8, 12 * 16}, "Rate multiplier:", Color::light_grey()},
    };

    OptionsField field_encoder_dial_sensitivity{
        {20 * 8, 5 * 16},
        6,
        {{"LOW", encoder_dial_sensitivity::DIAL_SENSITIVITY_LOW},
         {"NORMAL", encoder_dial_sensitivity::DIAL_SENSITIVITY_NORMAL},
         {"HIGH", encoder_dial_sensitivity::DIAL_SENSITIVITY_HIGH}}};

    NumberField field_encoder_rate_multiplier{
        {20 * 8, 12 * 16},
        2,
        {1, 10},
        1,
        ' '};

    Button button_save{
        {2 * 8, 16 * 16, 12 * 8, 32},
        "Save"};

    Button button_cancel{
        {16 * 8, 16 * 16, 12 * 8, 32},
        "Cancel",
    };
};

class SetPersistentMemoryView : public View {
   public:
    SetPersistentMemoryView(NavigationView& nav);

    void focus() override;

    std::string title() const override { return "P.Mem Mgmt"; };

   private:
    Labels labels{
        {{1 * 8, 1 * 16}, "Save persistent memory on SD", Color::light_grey()},
        {{1 * 8, 2 * 16}, "card. Needed when device has", Color::light_grey()},
        {{1 * 8, 3 * 16}, "dead/missing coin battery.", Color::light_grey()},
    };

    Text text_pmem_status{
        {1 * 8, 4 * 16 + 8, 28 * 8, 16},
        ""};

    Checkbox check_use_sdcard_for_pmem{
        {2 * 8, 6 * 16},
        21,
        "Use SD card for P.Mem"};

    Button button_save_mem_to_file{
        {1 * 8, 8 * 16, 28 * 8, 2 * 16},
        "Save P.Mem to SD card"};

    Button button_load_mem_from_file{
        {1 * 8, 10 * 16 + 2, 28 * 8, 2 * 16},
        "Load P.Mem from SD Card"};

    Button button_load_mem_defaults{
        {1 * 8, 12 * 16 + 4, 28 * 8, 2 * 16},
        "Reset P.Mem to defaults"};

    Button button_return{
        {16 * 8, 16 * 16, 12 * 8, 32},
        "Return",
    };
};

class AppSettingsView : public View {
   public:
    AppSettingsView(NavigationView& nav);
    std::string title() const override { return "App Settings"; };
    void focus() override;

   private:
    NavigationView& nav_;

    Labels labels{
        {{0, 4}, "Select file to edit:", Color::white()}};

    MenuView menu_view{
        {0, 2 * 8, 240, 26 * 8},
        true};
};

class SetConfigModeView : public View {
   public:
    SetConfigModeView(NavigationView& nav);

    void focus() override;

    std::string title() const override { return "Config Mode"; };

   private:
    Labels labels{
        {{1 * 8, 1 * 16}, "Controls whether firmware", Color::light_grey()},
        {{1 * 8, 2 * 16}, "will enter Config Mode", Color::light_grey()},
        {{1 * 8, 3 * 16}, "after a boot failure.", Color::light_grey()},
    };

    Checkbox checkbox_config_mode_enabled{
        {2 * 8, 6 * 16},
        16,
        "Config Mode enable"};

    Button button_save{
        {2 * 8, 16 * 16, 12 * 8, 32},
        "Save"};

    Button button_cancel{
        {16 * 8, 16 * 16, 12 * 8, 32},
        "Cancel",
    };
};

using portapack::persistent_memory::fake_brightness_level_options;

class SetFakeBrightnessView : public View {
   public:
    SetFakeBrightnessView(NavigationView& nav);

    void focus() override;

    std::string title() const override { return "Brightness"; };

   private:
    Labels labels{
        {{1 * 8, 1 * 16}, "Limits screen brightness", Color::light_grey()},
        {{1 * 8, 2 * 16}, "(has a small performance", Color::light_grey()},
        {{1 * 8, 3 * 16}, "impact when enabled).", Color::light_grey()},
        {{2 * 8, 8 * 16}, "Brightness:", Color::light_grey()},
    };

    OptionsField field_fake_brightness{
        {20 * 8, 8 * 16},
        6,
        {{"12.5%", fake_brightness_level_options::BRIGHTNESS_12p5},
         {"25%", fake_brightness_level_options::BRIGHTNESS_25},
         {"50%", fake_brightness_level_options::BRIGHTNESS_50}}};

    Checkbox checkbox_brightness_switch{
        {1 * 8, 5 * 16},
        16,
        "Enable brightness adjust"};

    Button button_save{
        {2 * 8, 16 * 16, 12 * 8, 32},
        "Save"};

    Button button_cancel{
        {16 * 8, 16 * 16, 12 * 8, 32},
        "Cancel",
    };
};

class SettingsMenuView : public BtnGridView {
   public:
    SettingsMenuView(NavigationView& nav);

    std::string title() const override { return "Settings"; };
};

} /* namespace ui */

#endif /*__UI_SETTINGS_H__*/
