/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __UI_RECORD_VIEW_H__
#define __UI_RECORD_VIEW_H__

#include "ui_iq_trim.hpp"
#include "ui_widget.hpp"

#include "bitmap.hpp"
#include "capture_thread.hpp"
#include "iq_trim.hpp"
#include "signal.hpp"

#include <cstddef>
#include <string>
#include <memory>

namespace ui {

class RecordView : public View {
   public:
    std::function<void(std::string)> on_error{};

    enum FileType {
        RawS8 = 1,
        RawS16 = 2,
        WAV = 3,
    };

    RecordView(
        const Rect parent_rect,
        const std::filesystem::path& filename_stem_pattern,
        const std::filesystem::path& folder,
        FileType file_type,
        const size_t write_size,
        const size_t buffer_count);
    ~RecordView();

    void focus() override;

    /* Sets the sampling rate for the baseband.
     * NB: Do not pre-apply any oversampling. This function will determine
     * the correct amount of oversampling and return the actual sample rate
     * that can be used to configure the radio or other UI element. */
    uint32_t set_sampling_rate(uint32_t new_sampling_rate);

    void set_file_type(const FileType v) { file_type = v; }
    void set_auto_trim(bool v) { auto_trim = v; }

    void start();
    void stop();
    void on_hide() override;

    bool is_active() const;

    void set_filename_date_frequency(bool set);
    void set_filename_as_is(bool set);

   private:
    void toggle();
    // void toggle_pitch_rssi();

    void on_tick_second();
    void update_status_display();
    void trim_capture();

    void handle_capture_thread_done(const File::Error error);
    void handle_error(const File::Error error);

    OversampleRate get_oversample_rate(uint32_t sample_rate);

    void on_gps(const GPSPosDataMessage* msg);
    // bool pitch_rssi_enabled = false;

    // Time Stamp
    bool filename_date_frequency = false;
    bool filename_as_is = false;
    rtc::RTC datetime{};

    float latitude = 0;  // for wardriwing with ext module
    float longitude = 0;
    uint8_t satinuse = 0;  // to see if there was enough sats used or not

    const std::filesystem::path filename_stem_pattern;
    const std::filesystem::path folder;
    FileType file_type;
    const size_t write_size;
    const size_t buffer_count;
    uint32_t sampling_rate{0};
    SignalToken signal_token_tick_second{};

    bool auto_trim = false;
    std::filesystem::path trim_path{};
    TrimProgressUI trim_ui{};

    Rectangle rect_background{
        Theme::getInstance()->bg_darkest->background};

    /*ImageButton button_pitch_rssi {
                { 2, 0 * 16, 3 * 8, 1 * 16 },
                &bitmap_rssipwm,
                Theme::getInstance()->fg_orange->foreground,
                Theme::getInstance()->fg_orange->background
        };*/

    ImageButton button_record{
        //{ 4 * 8, 0 * 16, 2 * 8, 1 * 16 },
        {0 * 8, 0 * 16, 2 * 8, 1 * 16},
        &bitmap_record,
        Theme::getInstance()->fg_red->foreground,
        Theme::getInstance()->fg_red->background};

    Text text_record_filename{
        {7 * 8, 0 * 16, 8 * 8, 16},
        "",
    };

    Text text_record_dropped{
        {16 * 8, 0 * 16, 3 * 8, 16},
        "",
    };

    Text text_time_available{
        {21 * 8, 0 * 16, 9 * 8, 16},
        "",
    };

    Image gps_icon{
        {2 * 8 + 1, 0 * 16, 2 * 8, 1 * 16},
        &bitmap_target,
        Theme::getInstance()->bg_darkest->foreground,
        Theme::getInstance()->bg_darkest->background};

    std::unique_ptr<CaptureThread> capture_thread{};

    MessageHandlerRegistration message_handler_capture_thread_error{
        Message::ID::CaptureThreadDone,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const CaptureThreadDoneMessage*>(p);
            this->handle_capture_thread_done(message.error);
        }};

    MessageHandlerRegistration message_handler_gps{
        Message::ID::GPSPosData,
        [this](Message* const p) {
            const auto message = static_cast<const GPSPosDataMessage*>(p);
            this->on_gps(message);
        }};
};

} /* namespace ui */

#endif /*__UI_RECORD_VIEW_H__*/
