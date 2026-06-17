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

#ifndef __UI_CONSTELLATION_APP_H__
#define __UI_CONSTELLATION_APP_H__

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

namespace ui::external_app::constellation {

constexpr size_t constellation_max_points = 128;

/* Scatter-plot widget. Plots up to 128 corrected I/Q points per frame as
 * pixels on a square grid (X = I, Y = Q). The M0 has no FPU, so everything
 * here is integer-only.
 *
 * Anti-flicker follows the Time Sink approach (the screen is never cleared
 * wholesale -- only pixels that actually disappear are erased), but generalised
 * from 1-D columns to scattered 2-D points: each frame keeps the list of pixels
 * it lit, and when a frame ages out, one of its pixels is erased only if no
 * retained newer frame -- and not the current frame -- lit that exact pixel.
 */
class ConstellationWidget : public Widget {
   public:
    ConstellationWidget(Rect parent_rect, const uint8_t* iq_data, size_t point_count);
    ConstellationWidget(const ConstellationWidget&) = delete;
    ConstellationWidget(ConstellationWidget&&) = delete;
    ConstellationWidget& operator=(const ConstellationWidget&) = delete;
    ConstellationWidget& operator=(ConstellationWidget&&) = delete;

    void set_parent_rect(const Rect new_parent_rect) override;
    void on_show() override;
    void paint(Painter& painter) override;
    void set_persistence_frames(uint8_t frames);

   private:
    static constexpr size_t max_points = constellation_max_points;
    static constexpr size_t max_persistence_frames = 8;

    void reset_cache();
    void recompute_geometry();
    Color background_at(Coord x, Coord y) const;

    const uint8_t* iq_data_;
    size_t point_count_;

    // Plot geometry (absolute screen coordinates), recomputed on resize.
    Coord plot_left_{0};
    Coord plot_top_{0};
    Dim plot_side_{0};
    Coord center_x_{0};
    Coord center_y_{0};

    // Current frame's plotted pixels (absolute coordinates).
    std::array<Coord, max_points> cur_x_{};
    std::array<Coord, max_points> cur_y_{};
    size_t cur_count_{0};

    // Per-frame history of plotted pixels for the persistence/erase logic.
    std::array<std::array<Coord, max_points>, max_persistence_frames> hist_x_{};
    std::array<std::array<Coord, max_points>, max_persistence_frames> hist_y_{};
    std::array<uint8_t, max_persistence_frames> hist_len_{};
    size_t history_count_{0};
    size_t history_head_{0};
    uint8_t persistence_frames_{1};
    bool needs_clear_{true};
};

class ConstellationView : public View {
   public:
    ConstellationView(NavigationView& nav);
    ~ConstellationView();

    ConstellationView(const ConstellationView&) = delete;
    ConstellationView(ConstellationView&&) = delete;
    ConstellationView& operator=(const ConstellationView&) = delete;
    ConstellationView& operator=(ConstellationView&&) = delete;

    std::string title() const override { return "Constellation"; };
    void focus() override;
    void on_show() override;
    void on_hide() override;
    void set_parent_rect(const Rect new_parent_rect) override;

   private:
    static constexpr Dim header_height = 3 * 16;

    NavigationView& nav_;
    RxRadioState radio_state_{
        433'920'000,
        1'750'000,
        4'000'000,
        ReceiverModel::Mode::SpectrumAnalysis};

    uint32_t sampling_rate{4'000'000};
    uint32_t decimation{8};
    uint32_t order{4};
    uint32_t loop_bw{1};
    uint8_t correct_frequency{0};
    uint8_t correct_phase{0};
    uint8_t persistence_frames{1};
    app_settings::SettingsManager settings_{
        "rx_constellation"sv,
        app_settings::Mode::RX,
        {
            {"sampling_rate"sv, &sampling_rate},
            {"decimation"sv, &decimation},
            {"order"sv, &order},
            {"loop_bw"sv, &loop_bw},
            {"correct_frequency"sv, &correct_frequency},
            {"correct_phase"sv, &correct_phase},
            {"persistence_frames"sv, &persistence_frames},
        }};

    uint8_t iq_buffer[2 * constellation_max_points]{0};
    ChannelSpectrumFIFO* fifo = nullptr;

    Labels labels{
        {{UI_POS_X(0), UI_POS_Y(1)}, "SR:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(10), UI_POS_Y(1)}, "DEC:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(18), UI_POS_Y(1)}, "M:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(2)}, "PST:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(8), UI_POS_Y(2)}, "BW:", Theme::getInstance()->fg_light->foreground},
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
            {"2.0M  ", 2'000'000},
            {"4.0M  ", 4'000'000},
            {"8.0M  ", 8'000'000},
            {"10.0M ", 10'000'000},
            {"20.0M ", 20'000'000},
        }};

    OptionsField options_decimation{
        {UI_POS_X(14), UI_POS_Y(1)},
        3,
        {
            {"x4 ", 4},
            {"x8 ", 8},
            {"x16", 16},
            {"x32", 32},
            {"x64", 64},
        }};

    OptionsField options_order{
        {UI_POS_X(20), UI_POS_Y(1)},
        2,
        {
            {"2 ", 2},
            {"4 ", 4},
            {"8 ", 8},
        }};

    OptionsField options_persistence{
        {UI_POS_X(4), UI_POS_Y(2)},
        3,
        {
            {"1  ", 1},
            {"2  ", 2},
            {"4  ", 4},
            {"8  ", 8},
        }};

    OptionsField options_loop_bw{
        {UI_POS_X(11), UI_POS_Y(2)},
        4,
        {
            {"Slow", 0},
            {"Med ", 1},
            {"Fast", 2},
        }};

    Checkbox check_frequency{
        {UI_POS_X(16), UI_POS_Y(2)},
        3,
        "FRQ",
        true};

    Checkbox check_phase{
        {UI_POS_X(22), UI_POS_Y(2)},
        3,
        "PHS",
        true};

    ConstellationWidget plot{
        {0, header_height, screen_width, screen_height - header_height},
        iq_buffer,
        constellation_max_points};

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
                bool has_data = false;
                while (fifo->out(spectrum)) {
                    has_data = true;
                }
                if (has_data) {
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

    void apply_config();
    void on_channel_spectrum(const ChannelSpectrum& spectrum);
    void on_freqchg(int64_t freq);
};

}  // namespace ui::external_app::constellation

#endif  // __UI_CONSTELLATION_APP_H__
