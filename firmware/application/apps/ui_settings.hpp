/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2023 gullradriel, Nilorea Studio Inc.
 * Copyright (C) 2023 Kyle Reed
 * Copyright (C) 2024 Mark Thompson
 * Copyright (C) 2024 u-foka
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
        {{1 * 8, 1 * 16}, "Adjust the RTC clock date &", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 2 * 16}, "time. If clock resets after", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 3 * 16}, "reboot, coin batt. is dead. ", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 5 * 16 - 2}, "YYYY-MM-DD HH:MM:SS  DoW DoY", Theme::getInstance()->fg_medium->foreground},
        {{5 * 8, 6 * 16}, "-  -     :  :", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 11 * 16}, "DST adds 1 hour to RTC time.", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 12 * 16}, "Start: 0:00 on Nth  DDD in", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 13 * 16}, "End:   1:00 on Nth  DDD in", Theme::getInstance()->fg_light->foreground}};

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
        {{2 * 8, 3 * 16}, "Frequency correction:", Theme::getInstance()->fg_light->foreground},
        {{6 * 8, 4 * 16}, "PPM", Theme::getInstance()->fg_light->foreground},
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
        {{26 * 8, 6 * 16}, "kHz", Theme::getInstance()->fg_light->foreground}};

    Labels labels_bias{
        {{4 * 8 + 4, 8 * 16}, "CAUTION: Ensure that all", Theme::getInstance()->error_dark->foreground},
        {{5 * 8 + 0, 9 * 16}, "devices attached to the", Theme::getInstance()->error_dark->foreground},
        {{6 * 8 + 0, 10 * 16}, "antenna connector can", Theme::getInstance()->error_dark->foreground},
        {{6 * 8 + 4, 11 * 16}, "accept a DC voltage!", Theme::getInstance()->error_dark->foreground}};

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
        {{3 * 8, 13 * 16}, "Show/Hide Status Icons", Theme::getInstance()->fg_light->foreground},
    };

    ImageToggle toggle_camera{
        {2 * 8, 14 * 16 + 2, 16, 16},
        &bitmap_icon_camera};

    ImageToggle toggle_sleep{
        {4 * 8, 14 * 16 + 2, 16, 16},
        &bitmap_icon_sleep};

    ImageToggle toggle_stealth{
        {6 * 8, 14 * 16 + 2, 16, 16},
        &bitmap_icon_stealth};

    ImageToggle toggle_converter{
        {8 * 8, 14 * 16 + 2, 16, 16},
        &bitmap_icon_upconvert};

    ImageToggle toggle_bias_tee{
        {10 * 8, 14 * 16 + 2, 16, 16},
        &bitmap_icon_biast_off};

    ImageToggle toggle_clock{
        {12 * 8, 14 * 16 + 2, 8, 16},
        &bitmap_icon_clk_ext};

    ImageToggle toggle_mute{
        {13 * 8, 14 * 16 + 2, 16, 16},
        &bitmap_icon_speaker_and_headphones_mute};

    ImageToggle toggle_speaker{
        {15 * 8, 14 * 16 + 2, 16, 16},
        &bitmap_icon_speaker_mute};

    ImageToggle toggle_battery_icon{
        {17 * 8, 14 * 16 + 2, 16, 16},
        &bitmap_icon_batt_icon};

    ImageToggle toggle_battery_text{
        {19 * 8, 14 * 16 + 2, 16, 16},
        &bitmap_icon_batt_text};

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
        {{1 * 8, 120 - 48}, "    HIGH SPEED SDCARD IO     ", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 120 - 32}, "   May or may not work !!    ", Theme::getInstance()->fg_light->foreground}};

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
        {{1 * 8, 1 * 16}, "Options for working with", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 2 * 16}, "up/down converter hardware", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 3 * 16}, "like a Ham It Up.", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 9 * 16 - 2}, "Conversion frequency:", Theme::getInstance()->fg_light->foreground},
        {{18 * 8, 10 * 16}, "MHz", Theme::getInstance()->fg_light->foreground},
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
        {{1 * 8, 1 * 16}, "Frequency correction allows", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 2 * 16}, "RX and TX frequencies to be", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 3 * 16}, "adjusted for all apps.", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 6 * 16}, "RX Adjustment Frequency", Theme::getInstance()->fg_light->foreground},
        {{18 * 8, 7 * 16}, "MHz", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 9 * 16}, "TX Adjustment Frequency", Theme::getInstance()->fg_light->foreground},
        {{18 * 8, 10 * 16}, "MHz", Theme::getInstance()->fg_light->foreground},
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
        {{1 * 8, 1 * 16}, "Controls the volume of the", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 2 * 16}, "tone when transmitting in", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 3 * 16}, "Soundboard or Mic apps:", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 5 * 16}, "Tone key mix:   %", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 8 * 16}, "Controls whether apps should", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 9 * 16}, "beep on speaker & headphone", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 10 * 16}, "when a packet is received", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 11 * 16}, "(not all apps support this):", Theme::getInstance()->fg_light->foreground},
    };

    NumberField field_tone_mix{
        {16 * 8, 5 * 16},
        2,
        {10, 99},
        1,
        '0'};

    Checkbox checkbox_beep_on_packets{
        {3 * 8, 13 * 16},
        16,
        "Beep on RX packets"};

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
        {{1 * 8, 1 * 16}, "Change the size of the QR", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 2 * 16}, "code shown in Radiosonde.", Theme::getInstance()->fg_light->foreground},
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
        {{1 * 8, 1 * 16}, "Adjusts sensitivity to dial", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 2 * 16}, "rotation position (number of", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 3 * 16}, "steps per full rotation):", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 5 * 16}, "Dial sensitivity:", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 8 * 16}, "Adjusts sensitivity to dial", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 9 * 16}, "rotation rate (default 1", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 10 * 16}, "means no rate dependency):", Theme::getInstance()->fg_light->foreground},
        {{3 * 8, 12 * 16}, "Rate multiplier:", Theme::getInstance()->fg_light->foreground},
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
        {1, 15},
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
        {{1 * 8, 1 * 16}, "Save persistent memory on SD", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 2 * 16}, "card. Needed when device has", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 3 * 16}, "dead/missing coin battery.", Theme::getInstance()->fg_light->foreground},
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
        {{0, 4}, "Select file to edit:", Theme::getInstance()->bg_darkest->foreground}};

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
        {{1 * 8, 1 * 16}, "Controls whether firmware", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 2 * 16}, "will enter Config Mode", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 3 * 16}, "after a boot failure.", Theme::getInstance()->fg_light->foreground},
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
        {{1 * 8, 1 * 16}, "Limits screen brightness", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 2 * 16}, "(has a small performance", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 3 * 16}, "impact when enabled).", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 8 * 16}, "Brightness:", Theme::getInstance()->fg_light->foreground},
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

class SetMenuColorView : public View {
   public:
    SetMenuColorView(NavigationView& nav);

    void focus() override;

    std::string title() const override { return "Menu Color"; };

   private:
    void paint_sample();

    Labels labels{
        {{3 * 8, 1 * 16}, "Menu Button Color Scheme", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 8 * 16}, "Red Level:", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 9 * 16}, "Green Level:", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 10 * 16}, "Blue Level:", Theme::getInstance()->fg_light->foreground},
    };

    NewButton button_sample{
        {8 * 8, 4 * 16, 14 * 8, 3 * 16},
        "New Color",
        &bitmap_icon_brightness,
    };

    NumberField field_red_level{
        {15 * 8, 8 * 16},
        3,
        {8, 248},
        8,
        ' ',
    };

    NumberField field_green_level{
        {15 * 8, 9 * 16},
        3,
        {8, 248},
        8,
        ' ',
    };

    NumberField field_blue_level{
        {15 * 8, 10 * 16},
        3,
        {8, 248},
        8,
        ' ',
    };

    Button button_reset{
        {2 * 8, 13 * 16, 12 * 8, 32},
        "Reset",
    };

    Button button_save{
        {2 * 8, 16 * 16, 12 * 8, 32},
        "Save"};

    Button button_cancel{
        {16 * 8, 16 * 16, 12 * 8, 32},
        "Cancel",
    };
};

class SetAutostartView : public View {
   public:
    SetAutostartView(NavigationView& nav);

    void focus() override;

    std::string title() const override { return "Autostart"; };

   private:
    int32_t i = 0;
    std::string autostart_app{""};
    OptionsField::options_t opts{};
    std::map<int32_t, std::string> full_app_list{};  // looking table
    int32_t selected = 0;
    SettingsStore nav_setting{
        "nav"sv,
        {{"autostart_app"sv, &autostart_app}}};
    Labels labels{
        {{1 * 8, 1 * 16}, "Select app to start on boot", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 2 * 16}, "(an SD Card is required)", Theme::getInstance()->fg_light->foreground}};

    Button button_save{
        {2 * 8, 16 * 16, 12 * 8, 32},
        "Save"};

    OptionsField options{
        {8 * 8, 4 * 16},
        30,
        {}};

    Button button_cancel{
        {16 * 8, 16 * 16, 12 * 8, 32},
        "Cancel",
    };
};

class SetThemeView : public View {
   public:
    SetThemeView(NavigationView& nav);

    void focus() override;

    std::string title() const override { return "Theme"; };

   private:
    int32_t selected = 0;
    Labels labels{
        {{1 * 8, 1 * 16}, "Select a theme.", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 2 * 16}, "Restart PP to fully apply!", Theme::getInstance()->fg_light->foreground}};

    Button button_save{
        {2 * 8, 16 * 16, 12 * 8, 32},
        "Save"};

    OptionsField options{
        {8 * 8, 4 * 16},
        30,
        {
            {"Default - Grey", 0},
            {"Yellow", 1},
            {"Aqua", 2},
            {"Green", 3},
            {"Red", 4},
        }};

    Checkbox checkbox_menuset{
        {2 * 8, 6 * 16},
        23,
        "Set Menu color too"};

    Button button_cancel{
        {16 * 8, 16 * 16, 12 * 8, 32},
        "Cancel",
    };
};

class SettingsMenuView : public BtnGridView {
   public:
    SettingsMenuView(NavigationView& nav);
    std::string title() const override { return "Settings"; };

   private:
    NavigationView& nav_;
    void on_populate() override;
};

} /* namespace ui */

#endif /*__UI_SETTINGS_H__*/
