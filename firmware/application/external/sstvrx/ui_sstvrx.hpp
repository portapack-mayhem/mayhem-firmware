/*
 * Copyright (C) 2025 StarVore Labs
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

#ifndef __SSTVRX_H__
#define __SSTVRX_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_receiver.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "ui_freqman.hpp"
#include "baseband_api.hpp"
#include "event_m0.hpp"
#include "message.hpp"
#include "sstv.hpp"
#include "file.hpp"
#include "bmp.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "oversample.hpp"
#include "string_format.hpp"
#include "log_file.hpp"
#include "utility.hpp"
#include "audio.hpp"
#include "portapack.hpp"
#include <ch.h>

using namespace sstv;

namespace ui::external_app::sstvrx {

#define FMR_BTNGRID_TOP 60

class SstvRxLogger {
    public:
     Optional<File::Error> append(const std::filesystem::path& filename) {
        return log_file.append(filename);
     }

     void log_error(const std::string& error_message);
     void log_info(const std::string& info_message);

    private:
     LogFile log_file{};
};

class SstvRxView : public ui::View {
   public:
    SstvRxView(ui::NavigationView& nav);
    SstvRxView& operator=(const SstvRxView&) = delete;
    SstvRxView(const SstvRxView&) = delete;
    ~SstvRxView();

    std::string title() { return "SSTV RX"; }
    void focus() override;
    void on_show() override;

   private:
    ui::NavigationView& nav_;
    std::unique_ptr<SstvRxLogger> logger{};

    ReceiverModel::Mode receiver_mode = ReceiverModel::Mode::WidebandFMAudio;
    AudioSpectrum* audio_spectrum_data{nullptr};
    int16_t audio_spectrum[128]{0};
    RxRadioState radio_state_{};
    audio::Rate audio_sampling_rate = audio::Rate::Hz_48000;
    uint8_t radio_bw = 0;
    bool is_receiving = false;
    app_settings::SettingsManager settings_{
        "rx_sstv", app_settings::Mode::RX};
    const sstv_mode* rx_sstv_mode{};
    
    // Image data storage - only store current line to save memory
    static constexpr uint16_t IMAGE_WIDTH = 320;
    static constexpr uint16_t IMAGE_HEIGHT = 256;
    static constexpr uint16_t PIXELS_PER_LINE = 320;
    static constexpr uint16_t DISPLAY_WIDTH = 240;  // Scaled display width
    static constexpr uint16_t DISPLAY_HEIGHT = 192; // Scaled display height
    static constexpr uint16_t SSTV_IMG_START_ROW = 7;  // Start drawing at row 7 (after controls)
    
    uint16_t current_line_rx{0};
    std::unique_ptr<File> image_file{};
    std::filesystem::path current_image_path{};
    ui::Color line_buffer[DISPLAY_WIDTH];
    uint16_t line_num{0};

    MessageHandlerRegistration message_handler_progress{
        Message::ID::SSTVRXProgress,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const SSTVRXProgressMessage*>(p);
            this->on_progress(message.line, message.total_lines);
        }
    };

    // UI Elements
    RFAmpField field_rf_amp{{13 * 8, UI_POS_Y(0)}};
    LNAGainField field_lna{{15 * 8, UI_POS_Y(0)}};
    VGAGainField field_vga{{18 * 8, UI_POS_Y(0)}};
    
    RSSI rssi{{UI_POS_X(21), 0, UI_POS_WIDTH_REMAINING(24), 4}};
    RxFrequencyField field_frequency{{UI_POS_X(0), UI_POS_Y(0)}, nav_};
    AudioVolumeField field_volume{{screen_width - 2 * 8, UI_POS_Y(0)}};
    OptionsField options_mode {
        {6 * 8, 3 * 8},
        16,
        {}};
    Labels labels{
        {{1 * 8, 3 * 8}, "Mode:", Theme::getInstance()->fg_light->foreground}
    };
    Audio audio{{21 * 8, 10, 6 * 8, 4}};
    ui::Button start_btn{{2 * 8, UI_POS_Y(3), UI_POS_WIDTH(12), UI_POS_HEIGHT(3)}, "Start RX"};
    ui::Button stop_btn{{16 * 8, UI_POS_Y(3), UI_POS_WIDTH(12), UI_POS_HEIGHT(3)}, "Stop RX"};

    void on_audio_spectrum();
    void update_display(uint16_t line_num, const uint8_t* data_ptr);
    void start_audio();
    void on_start();
    void on_stop();
    void on_mode_changed(const size_t index);
    void on_progress(uint16_t line, uint16_t total_lines);
    void write_bmp_header();
    void finish_image();
    void save_image();  // Deprecated
};

}  // namespace ui::external_app::sstvrx

#endif  // __SSTVRX_H__