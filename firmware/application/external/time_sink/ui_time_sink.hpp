/*
 * Copyleft zxkmm (>) 2026
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

#ifndef __UI_TIME_SINK_APP_H__
#define __UI_TIME_SINK_APP_H__

#include "app_settings.hpp"
#include "baseband_api.hpp"
#include "message.hpp"
#include "radio_state.hpp"
#include "ui.hpp"
#include "ui_freq_field.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_widget.hpp"

#include <array>

namespace ui::external_app::time_sink {

constexpr size_t time_sink_waveform_points = 240;

class TimeSinkWaveformWidget : public Widget {
   public:
    TimeSinkWaveformWidget(Rect parent_rect, const int8_t* data, size_t length, Color color);
    TimeSinkWaveformWidget(const TimeSinkWaveformWidget&) = delete;
    TimeSinkWaveformWidget(TimeSinkWaveformWidget&&) = delete;
    TimeSinkWaveformWidget& operator=(const TimeSinkWaveformWidget&) = delete;
    TimeSinkWaveformWidget& operator=(TimeSinkWaveformWidget&&) = delete;

    void set_parent_rect(const Rect new_parent_rect) override;
    void on_show() override;
    void paint(Painter& painter) override;
    void set_persistence_frames(uint8_t frames);

   private:
    static constexpr size_t max_columns = time_sink_waveform_points;
    static constexpr size_t max_persistence_frames = 16;  // this is sad that we cant have 32 histories in ext app due to memory constraints

    void reset_cache();
    Coord sample_to_y(const Rect& r, int8_t sample) const;

    const int8_t* data_;
    size_t length_;
    Color color_;
    std::array<Coord, max_columns> current_y_{};
    std::array<std::array<int8_t, max_columns>, max_persistence_frames> history_samples_{};
    size_t history_count_{0};
    size_t history_head_{0};
    uint8_t persistence_frames_{1};
    bool needs_clear_{true};
};

class TimeSinkView : public View {
   public:
    TimeSinkView(NavigationView& nav);
    ~TimeSinkView();

    TimeSinkView(const TimeSinkView&) = delete;
    TimeSinkView(TimeSinkView&&) = delete;
    TimeSinkView& operator=(const TimeSinkView&) = delete;
    TimeSinkView& operator=(TimeSinkView&&) = delete;

    std::string title() const override { return "Time Sink"; };
    void focus() override;
    void on_show() override;
    void on_hide() override;
    void set_parent_rect(const Rect new_parent_rect) override;

   private:
    enum class TriggerMode : uint8_t {
        Off = 0,
        Rising = 1,
        Falling = 2,
    };

    static constexpr Dim header_height = 3 * 16;
    static constexpr size_t waveform_points = time_sink_waveform_points;

    NavigationView& nav_;
    RxRadioState radio_state_{
        433'920'000,
        1'750'000,
        2'000'000,
        ReceiverModel::Mode::SpectrumAnalysis};

    uint32_t sampling_rate{2'000'000};
    uint8_t trigger{0};
    uint8_t persistence_frames{1};
    uint8_t trigger_mode{static_cast<uint8_t>(TriggerMode::Rising)};
    int32_t trigger_level{0};
    size_t trigger_lock_index{0};
    bool trigger_lock_valid{false};
    app_settings::SettingsManager settings_{
        "rx_time_sink"sv,
        app_settings::Mode::RX,
        {
            {"sampling_rate"sv, &sampling_rate},
            {"trigger"sv, &trigger},
            {"persistence_frames"sv, &persistence_frames},
            {"trigger_mode"sv, &trigger_mode},
            {"trigger_level"sv, &trigger_level},
        }};

    int8_t waveform_buffer[waveform_points]{0};
    ChannelSpectrumFIFO* fifo = nullptr;

    Labels labels{
        {{UI_POS_X(0), UI_POS_Y(1)}, "SR:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(11), UI_POS_Y(1)}, "DEC:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(2)}, "PST:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(9), UI_POS_Y(2)}, "TRM:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(19), UI_POS_Y(2)}, "LVL:", Theme::getInstance()->fg_light->foreground},
    };

    RxFrequencyField field_frequency{
        {UI_POS_X(0), UI_POS_Y(0)},
        nav_};

    FrequencyStepView field_frequency_step{
        {10 * 8, UI_POS_Y(0)}};

    RFAmpField field_rf_amp{
        {16 * 8, UI_POS_Y(0)}};

    LNAGainField field_lna{
        {18 * 8, UI_POS_Y(0)}};

    VGAGainField field_vga{
        {21 * 8, UI_POS_Y(0)}};

    OptionsField options_sample_rate{
        {UI_POS_X(3), UI_POS_Y(1)},
        6,
        {
            {"1.0M  ", 1'000'000},
            {"2.0M  ", 2'000'000},
            {"5.0M  ", 5'000'000},
            {"10.0M ", 10'000'000},
            {"20.0M ", 20'000'000},
        }};

    NumberField field_trigger{
        {UI_POS_X(17), UI_POS_Y(1)},
        3,
        {0, 128},
        1,
        ' '};

    OptionsField options_persistence{
        {UI_POS_X(3), UI_POS_Y(2)},
        3,
        {
            {"1  ", 1},
            {"2  ", 2},
            {"4  ", 4},
            {"8  ", 8},
            {"16 ", 16},
        }};

    OptionsField options_trigger_mode{
        {UI_POS_X(13), UI_POS_Y(2)},
        4,
        {
            {"Off ", static_cast<int32_t>(TriggerMode::Off)},
            {"Rise", static_cast<int32_t>(TriggerMode::Rising)},
            {"Fall", static_cast<int32_t>(TriggerMode::Falling)},
        }};

    NumberField field_trigger_level{
        {UI_POS_X(23), UI_POS_Y(2)},
        4,
        {-127, 127},
        1,
        ' '};

    TimeSinkWaveformWidget waveform{
        {0, header_height, screen_width, screen_height - header_height},
        waveform_buffer,
        waveform_points,
        Theme::getInstance()->fg_light->foreground};

    MessageHandlerRegistration message_handler_spectrum_config{
        Message::ID::ChannelSpectrumConfig,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const ChannelSpectrumConfigMessage*>(p);
            this->fifo = message.fifo;
        }};

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            if (this->fifo) {
                ChannelSpectrum spectrum{};
                bool has_spectrum = false;
                while (fifo->out(spectrum)) {
                    has_spectrum = true;
                }
                if (has_spectrum) {
                    this->on_channel_spectrum(spectrum);
                }
            }
        }};

    MessageHandlerRegistration message_handler_freqchg{
        Message::ID::FreqChangeCommand,
        [this](Message* const p) {
            const auto message = static_cast<const FreqChangeCommandMessage*>(p);
            this->on_freqchg(message->freq);
        }};

    void apply_spectrum_config();
    size_t find_stable_trigger_index(const ChannelSpectrum& spectrum);
    void on_channel_spectrum(const ChannelSpectrum& spectrum);
    void on_freqchg(int64_t freq);
};

}  // namespace ui::external_app::time_sink

#endif  // __UI_TIME_SINK_APP_H__