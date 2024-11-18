/*
 * Copyright (C) 2024 Samir Sánchez Garnica @sasaga92
 *
 * This file is part of PortaPack.
 *
 */

#ifndef __UI_OOK_REMOTE_H__
#define __UI_OOK_REMOTE_H__

#include "ui.hpp"
#include "ui_language.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "string_format.hpp"
#include "radio_state.hpp"
#include "ui_freq_field.hpp"
#include "ui_transmitter.hpp"
#include "app_settings.hpp"
#include "transmitter_model.hpp"
#include <string>

using namespace ui;

namespace ui::external_app::ook_remote {

#define PROGRESS_MAX 100
#define OOK_SAMPLERATE_DEFAULT 2280000U            // Set the default Sample Rate
#define TRANSMISSION_FREQUENCY_DEFAULT 433920000U  // Sets the default transmission frequency (27 MHz).
#define WAVEFORM_BUFFER_SIZE 550

class OOKRemoteAppView : public View {
   public:
    void focus() override;
    OOKRemoteAppView(NavigationView& nav);

    ~OOKRemoteAppView();

    std::string title() const override {
        return "OOK Remote";
    };

   private:
    NavigationView& nav_;                           // Reference to the navigation system.
    std::string payload{""};                        // Holds the data payload as a string.
    uint32_t progress = 0;                          // Stores the current transmission progress.
    int16_t waveform_buffer[WAVEFORM_BUFFER_SIZE];  // Buffer for waveform data.
    bool is_transmitting = false;                   // State of transmission.

    void update();
    void draw_waveform();

    // Initiates data transmission with a specified message.
    void start_tx(const std::string& message);

    // Stops data transmission.
    void stop_tx();

    // Updates the transmission progress on the progress bar.
    void on_tx_progress(const uint32_t progress, const bool done);

    // Updates data when a new file is loaded.
    void on_file_changed(const std::filesystem::path& new_file_path);

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
    app_settings::SettingsManager settings_{"tx_ook_remote", app_settings::Mode::TX};

    // UI components for frequency and transmitter view.
    TxFrequencyField field_frequency{{0 * 8, 0 * 16}, nav_};
    TransmitterView2 tx_view{{20 * 7, 0 * 16}, true};

    // Labels for various fields such as sample rate and repeat count.
    Labels sample_rate{{{0, 40}, "S/Rate:     ", Theme::getInstance()->fg_light->foreground}};
    // Labels step_symbol_rate {{{160, 0}, "Step/Sym:    ",Theme::getInstance()->fg_light->foreground}};
    Labels step_symbol_rate{{{0, 18}, "Step/Symbols:    ", Theme::getInstance()->fg_light->foreground}};
    Labels repeat{{{0, 60}, "Repeat:     ", Theme::getInstance()->fg_light->foreground}};
    Labels field_symbol_rate{{{111, 40}, "Symbols:     ", Theme::getInstance()->fg_light->foreground}};
    Labels field_pause_symbol{{{111, 60}, "Pause/Sym:     ", Theme::getInstance()->fg_light->foreground}};
    Labels field_symbol_us_rate{{{210, 40}, "us     ", Theme::getInstance()->fg_light->foreground}};
    Labels label_waveform{{{0, 140}, "Waveform:", Theme::getInstance()->fg_light->foreground}};

    // OptionsField for selectable sample rates.
    OptionsField field_sample_rate{{55, 40}, 7, {{"250k", 250000U}, {"1M", 1000000U}, {"2M", 2000000U}, {"5M", 5000000U}, {"10M", 10000000U}, {"20M", 20000000U}}};

    // OptionsField for step symbol rates.
    OptionsField cant_step_symbol_rate{{105, 18}, 7, {{"1", 1}, {"10", 10}, {"100", 100}}};

    // Number fields for symbols, pause between symbols, and repeat count.
    NumberField cant_symbol_rate{{176, 40}, 4, {0, 9999}, 1, '0', false};
    NumberField cant_pause_symbol{{200, 60}, 4, {0, 200}, 1, '0', false};
    NumberField cant_repeat{{55, 60}, 3, {0, 100}, 1, '0', false};

    // Text field to display the payload data.
    Text text_payload{{0 * 8, 90, 30 * 8, 16}, "Payload:"};

    // Buttons for setting configurations, opening files, and starting transmission.
    Button button_set{{0, 110, 60, 28}, LanguageHelper::currentMessages[LANG_SET]};
    Button button_open{{100, 110, 80, 28}, LanguageHelper::currentMessages[LANG_OPEN_FILE]};
    Button button_start_stop{{80, 273, 80, 32}, LanguageHelper::currentMessages[LANG_START]};

    // Progress bar to display transmission progress.
    ProgressBar progressBar_progress{{2 * 8, 250, 208, 16}};

    // Waveform display using waveform buffer and yellow theme color.
    Waveform waveform{{0, 165, 240, 32}, waveform_buffer, 0, 0, true, Theme::getInstance()->fg_yellow->foreground};
};

};  // namespace ui::external_app::ook_remote

#endif /*__UI_OOK_REMOTE_H__*/
