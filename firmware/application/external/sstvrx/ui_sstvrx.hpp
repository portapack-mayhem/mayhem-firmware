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
#include "ui_channel.hpp"
#include "baseband_api.hpp"
#include "event_m0.hpp"
#include "message.hpp"
#include "sstv.hpp"
#include "file.hpp"
#include "bmpfile.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "oversample.hpp"
#include "string_format.hpp"
#include "log_file.hpp"
#include "utility.hpp"
#include "audio.hpp"
#include "portapack.hpp"
#include <array>
#include <ch.h>

#ifndef SSTVRX_ENABLE_LOGGER
#define SSTVRX_ENABLE_LOGGER 0
#endif

using namespace sstv;

namespace ui::external_app::sstvrx {

#define FMR_BTNGRID_TOP 60

#if SSTVRX_ENABLE_LOGGER
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
#endif

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
#if SSTVRX_ENABLE_LOGGER
    std::unique_ptr<SstvRxLogger> logger{};
#endif

    // Phase and slant adjustments (runtime only, not persisted)
    int16_t phase_adjustment{0};  // Horizontal offset in pixels (-50 to +50)
    int16_t slant_adjustment{0};  // Timing adjustment in 0.1% units (-100 to +100)

    // Settings must be declared before UI controls
    app_settings::SettingsManager settings_{
        "rx_sstv",
        app_settings::Mode::RX};

    ReceiverModel::Mode receiver_mode = ReceiverModel::Mode::WidebandFMAudio;
    AudioSpectrum* audio_spectrum_data{nullptr};
    int16_t audio_spectrum[128]{0};
    RxRadioState radio_state_{};
    audio::Rate audio_sampling_rate = audio::Rate::Hz_48000;
    uint8_t radio_bw = 0;
    bool is_receiving = false;
    const sstv_mode* rx_sstv_mode{};

    // Image data storage - only store current line to save memory
    static constexpr uint16_t IMAGE_WIDTH = 320;
    static constexpr uint16_t IMAGE_HEIGHT = 256;
    static constexpr uint16_t PIXELS_PER_LINE = 320;
    uint16_t DISPLAY_WIDTH = 240;                      // Scaled display width
    uint16_t DISPLAY_HEIGHT = 192;                     // Scaled display height
    static constexpr uint16_t SSTV_IMG_START_ROW = 7;  // Start drawing at row 7 (after controls)
    static constexpr size_t SHARED_BUFFER_BYTES = 512;
    static constexpr size_t CHUNK_FLAG_INDEX = SHARED_BUFFER_BYTES - 1;
    static constexpr size_t CHUNK_HEADER_BYTES = 2;
    static constexpr size_t CHUNK_COPY_BYTES = CHUNK_FLAG_INDEX;  // Exclude flag byte
    static constexpr uint16_t MAX_CHUNK_PIXELS = (CHUNK_COPY_BYTES - CHUNK_HEADER_BYTES) / 3;

    uint16_t current_line_rx{0};
    BMPFile bmp{};
    std::filesystem::path current_image_path{};
    ui::Color line_buffer[320];
    uint16_t line_num{0}, file_line_num{0};
    std::array<uint8_t, IMAGE_WIDTH * 3> pending_line_rgb{};
    uint16_t pending_line_number{0};
    uint8_t pending_chunk_mask{0};
    bool pending_line_valid{false};

    // Note: Post-reception phase/slant adjustment disabled due to M0 memory constraints
    // The 245KB image buffer exceeds available heap memory
    uint16_t max_received_line{0};

    MessageHandlerRegistration message_handler_progress{
        Message::ID::SSTVRXProgress,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const SSTVRXProgressMessage*>(p);
            this->on_progress(message.line, message.total_lines);
        }};

    MessageHandlerRegistration message_handler_calibration{
        Message::ID::SSTVRXCalibration,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const SSTVRXCalibrationMessage*>(p);
            this->on_calibration(message.suggested_phase, message.suggested_slant, message.sync_count);
        }};

    // UI Elements
    RFAmpField field_rf_amp{{UI_POS_X(13), UI_POS_Y(0)}};
    LNAGainField field_lna{{UI_POS_X(15), UI_POS_Y(0)}};
    VGAGainField field_vga{{UI_POS_X(18), UI_POS_Y(0)}};

    RSSI rssi{{UI_POS_X(21), 0, UI_POS_WIDTH_REMAINING(24), 4}};
    Channel channel{{UI_POS_X(21), 5, UI_POS_WIDTH_REMAINING(24), 4}};
    RxFrequencyField field_frequency{{UI_POS_X(0), UI_POS_Y(0)}, nav_};
    AudioVolumeField field_volume{{UI_POS_X_RIGHT(2), UI_POS_Y(0)}};

    OptionsField options_mode{{UI_POS_X(6), UI_POS_Y(1)}, 16, {}};

    NumberField field_phase{{UI_POS_X(4), UI_POS_Y(2)}, 3, {-50, 50}, 1, ' '};
    NumberField field_slant{{UI_POS_X(13), UI_POS_Y(2)}, 4, {-100, 100}, 1, ' '};

    Labels labels{
        {{UI_POS_X(1), UI_POS_Y(1)}, "Mode:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(1), UI_POS_Y(2)}, "Ph:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(8), UI_POS_Y(2)}, "Slnt:", Theme::getInstance()->fg_light->foreground}};

    Audio audio{{UI_POS_X(21), 10, UI_POS_WIDTH(6), 4}};
    ui::Button start_stop_btn{{UI_POS_X_RIGHT(12), UI_POS_Y(3), UI_POS_WIDTH(11), UI_POS_HEIGHT(2)}, "Start RX"};
    // ui::Button redraw_btn{{16 * 8, UI_POS_Y(5), UI_POS_WIDTH(12), UI_POS_HEIGHT(3)}, "Redraw"};

    // Calibration suggestion display
    Text text_calibration{
        {UI_POS_X(1), UI_POS_Y(3), UI_POS_WIDTH(17), UI_POS_HEIGHT(1)},
        "Calib: N/A"};

    void on_audio_spectrum();
    void update_display(uint16_t line_num, const uint8_t* rgb_line);
    void redraw_image();  // Disabled due to memory constraints
    void start_audio();
    void on_start_stop();  // Combined start/stop handler
    void on_stop();
    void on_mode_changed(const size_t index);
    void on_progress(uint16_t line, uint16_t total_lines);
    void on_calibration(int16_t suggested_phase, int16_t suggested_slant, uint16_t sync_count);
    void write_bmp_header();
    void write_line_to_file(uint16_t line_num, const uint8_t* rgb_line);
    void finish_image();
};

}  // namespace ui::external_app::sstvrx

#endif  // __SSTVRX_H__