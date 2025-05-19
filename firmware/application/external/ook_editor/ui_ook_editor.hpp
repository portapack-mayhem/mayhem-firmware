/*
 * Copyright (C) 2024 Samir Sánchez Garnica @sasaga92
 * copyleft Elliot Alderson from F society
 * copyleft Darlene Alderson from F society
 *
 * This file is part of PortaPack.
 *
 */

#ifndef __UI_OOK_EDITOR_H__
#define __UI_OOK_EDITOR_H__

#include "ui.hpp"
#include "ui_language.hpp"
#include "ui_freq_field.hpp"
#include "ui_textentry.hpp"
#include "ui_fileman.hpp"
#include "ui_transmitter.hpp"
#include "radio_state.hpp"
#include "app_settings.hpp"
#include "file_path.hpp"
#include "ook_file.hpp"

using namespace ui;

namespace ui::external_app::ook_editor {

#define PADDING_LEFT 1                             // waveform padding
#define PADDING_RIGHT 1                            // waveform padding
#define OOK_SAMPLERATE_DEFAULT 2280000U            // Set the default Sample Rate
#define TRANSMISSION_FREQUENCY_DEFAULT 433920000U  // Sets the default transmission frequency (27 MHz).
#define WAVEFORM_BUFFER_SIZE 550

/*****************Editor View ******************/

class OOKEditorAppView : public View {
   public:
    void focus() override;
    OOKEditorAppView(NavigationView& nav);

    ~OOKEditorAppView();

    std::string title() const override {
        return "OOKEditor";
    };

   private:
    NavigationView& nav_;                           // Reference to the navigation system.
    uint32_t progress = 0;                          // Stores the current transmission progress.
    int16_t waveform_buffer[WAVEFORM_BUFFER_SIZE];  // Buffer for waveform data.
    bool is_transmitting = false;                   // State of transmission.
    rf::Frequency ook_editor_tx_freq{24000000};     // last used transmit frequency
    std::string outputFileBuffer{};                 // buffer for output file
    ook_file_data ook_data = {0, 0, 0, 0, 0, ""};   // ook files handle

    // draw current payload waveform
    void draw_waveform();

    // update ook_data from GUI selection
    void update_ook_data_from_app();

    // start transmission
    void start_tx();
    // stop transmission
    void stop_tx();

    // Updates the transmission progress on the progress bar.
    void on_tx_progress(const uint32_t progress, const bool done);

    // Updates data when a new file is loaded.
    void on_file_changed(const std::filesystem::path& new_file_path);

    // Prepare a new std::filesystem::path from string value and call saveFile
    void on_save_file(const std::string value);

    // Called by on_save_file to save the current OOK parameters to the file
    bool save_ook_to_file(const std::filesystem::path& path);

    // Registers a message handler for transmission progress updates.
    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,          // Transmission progress message ID.
        [this](const Message* const p) {  // Callback to handle the message.
            const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
            this->on_tx_progress(message.progress, message.done);
        }};

    // Sets transmission frequency, bandwidth, and default sample rate for OOK.
    TxRadioState radio_state_{TRANSMISSION_FREQUENCY_DEFAULT, 1750000, OOK_SAMPLERATE_DEFAULT};

    // Settings manager for app configuration.
    app_settings::SettingsManager settings_{
        "ook_editor",
        app_settings::Mode::TX,
        {{"ook_editor_tx_freq"sv, &ook_editor_tx_freq}}};

    // UI components for frequency and transmitter view.
    FrequencyField field_frequency{{0 * 8, 0 * 16}};
    TransmitterView2 tx_view{{20 * 7, 0 * 16}, true};

    // Labels for various fields such as sample rate and repeat count.
    Labels label_step{{{170, 20}, "Step:", Theme::getInstance()->fg_light->foreground}};
    Labels label_sample_rate{{{0, 20}, "SampleRate:", Theme::getInstance()->fg_light->foreground}};
    Labels label_symbol_rate{{{0, 40}, "SymbolRate:", Theme::getInstance()->fg_light->foreground}};
    Labels label_symbol_rate_unit{{{132, 40}, "/s", Theme::getInstance()->fg_light->foreground}};
    Labels label_repeat{{{154, 40}, "Repeat:", Theme::getInstance()->fg_light->foreground}};
    Labels label_pause_symbol_duration{{{0, 60}, "PauseSymbol:", Theme::getInstance()->fg_light->foreground}};
    Labels label_pause_symbol_duration_unit{{{132, 60}, "us", Theme::getInstance()->fg_light->foreground}};
    Labels label_payload{{{0, 80}, "Payload:", Theme::getInstance()->fg_light->foreground}};
    Labels label_waveform{{{0, 188}, "Waveform:", Theme::getInstance()->fg_light->foreground}};

    // Text field to display the various status message of the app
    Text text_app_status{{0, 160, 30 * 8, 16}, ""};

    // OptionsField for selectable sample rates.
    OptionsField field_sample_rate{{96, 20}, 7, {{"250k", 250000U}, {"1M", 1000000U}, {"2M", 2000000U}, {"5M", 5000000U}, {"10M", 10000000U}, {"20M", 20000000U}}};
    // OptionsField for step symbol rates.
    OptionsField field_step{{210, 20}, 7, {{"1", 1}, {"10", 10}, {"100", 100}}};
    // Number fields for symbols, pause_symbol between repeat, and repeat count.
    NumberField field_symbol_rate{{96, 40}, 4, {0, 9999}, 1, '0', false};
    NumberField field_pause_symbol_duration{{96, 60}, 4, {0, 9999}, 1, '0', false};
    NumberField field_repeat{{210, 40}, 3, {1, 999}, 1, '0', false};

    // Text field to display the payload data.
    Text text_payload{{0 * 8, 100, 30 * 8, 16}, ""};

    // Buttons for setting configurations, opening files, and starting transmission.
    Button button_set{{0, 125, 60, 28}, LanguageHelper::currentMessages[LANG_SET]};
    Button button_open{{68, 125, 80, 28}, LanguageHelper::currentMessages[LANG_OPEN_FILE]};
    Button button_save{{154, 125, 80, 28}, LanguageHelper::currentMessages[LANG_SAVE_FILE]};
    Button button_bug_key{{0, 125 + 28 + 3, screen_width, 28}, "Bug Key"};
    Button button_send_stop{{80, 273, 80, 32}, LanguageHelper::currentMessages[LANG_SEND]};

    // Progress bar to display transmission progress.
    ProgressBar progressbar{{2 * 8, 250, 208, 16}};

    // Waveform display using waveform buffer and yellow theme color.
    Waveform waveform{{0, 208, 240, 32}, waveform_buffer, 0, 0, true, Theme::getInstance()->fg_yellow->foreground};
};

/******** bug key input view **********/

enum InsertType {
    LOW_LEVEL_SHORT,
    LOW_LEVEL_LONG,
    HIGH_LEVEL_SHORT,
    HIGH_LEVEL_LONG
};

class OOKEditorBugKeyView : public View {
   public:
    std::function<void(std::string)> on_save{};

    OOKEditorBugKeyView(NavigationView& nav, std::string payload);

    std::string title() const override { return "Bug.K"; };
    void focus() override;

   private:
    NavigationView& nav_;
    std::string payload_ = "";
    std::string path_ = "";
    uint32_t delay_{0};
    std::string delay_str{""};  // needed by text_prompt

    void on_insert(InsertType type);
    void on_delete();
    void update_console();
    std::string build_payload();

    Labels labels{
        {{0 * 8, 0 * 16}, "Primary Step", Theme::getInstance()->fg_light->foreground},
        {{(screen_width / 2), 0 * 16}, "Secondary Step", Theme::getInstance()->fg_light->foreground}};

    NumberField field_primary_step{
        {0 * 8, 1 * 16},
        3,
        {0, 550},
        1,
        ' '};

    NumberField field_secondary_step{
        {(screen_width / 2), 1 * 16},
        3,
        {0, 550},
        1,
        ' '};

    Console console{
        {0, 3 * 16, screen_width, screen_height - 10 * 16}};

    Button button_insert_low_level_long{
        {0 * 8, 13 * 16, screen_width / 2, 2 * 16},
        "00"};

    Button button_insert_low_level_short{
        {0 * 8, 15 * 16, screen_width / 2, 2 * 16},
        "0"};

    Button button_insert_high_level_long{
        {(screen_width / 2), 13 * 16, screen_width / 2, 2 * 16},
        "11"};

    Button button_insert_high_level_short{
        {(screen_width / 2), 15 * 16, screen_width / 2, 2 * 16},
        "1"};

    Button button_delete{
        {1, 17 * 16, screen_width / 2 - 4, 2 * 16},
        "<Backspace"};

    Button button_save{
        {1 + screen_width / 2 + 1, 17 * 16, screen_width / 2 - 4, 2 * 16},
        "Save"};
};

};  // namespace ui::external_app::ook_editor

#endif /*__UI_OOK_EDITOR_H__*/
