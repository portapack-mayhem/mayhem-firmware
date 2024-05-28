/*
 * Copyright (C) 2023 Bernd Herzog
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

#pragma once

#include "ui.hpp"
#include "ui_language.hpp"
#include "ui_widget.hpp"

#include "ui_navigation.hpp"
#include "ui_tabview.hpp"
#include "capture_app.hpp"
#include "baseband_api.hpp"

#include "app_settings.hpp"
#include "radio_state.hpp"
#include "portapack.hpp"
#include "message.hpp"

#include "ui_spectrum_painter_image.hpp"
#include "ui_spectrum_painter_text.hpp"

namespace ui::external_app::spainter {

class SpectrumPainterView : public View {
   public:
    SpectrumPainterView(NavigationView& nav);
    ~SpectrumPainterView();

    SpectrumPainterView(const SpectrumPainterView&) = delete;
    SpectrumPainterView(SpectrumPainterView&&) = delete;
    SpectrumPainterView& operator=(const SpectrumPainterView&) = delete;
    SpectrumPainterView& operator=(SpectrumPainterView&&) = delete;

    void focus() override;
    void paint(Painter& painter) override;

    std::string title() const override { return "Spec.Painter"; };

   private:
    NavigationView& nav_;
    TxRadioState radio_state_{};
    app_settings::SettingsManager settings_{
        "tx_painter", app_settings::Mode::TX};

    bool image_input_available{false};
    bool tx_active{false};
    uint32_t tx_mode{0};
    uint16_t tx_current_line{0};
    uint16_t tx_current_max_lines{0};
    uint16_t tx_current_width{0};
    systime_t tx_timestamp_start{0};

    inline uint32_t tx_time_elapsed() {
        auto now = chTimeNow();
        return now - tx_timestamp_start;
    }

    int32_t tx_gain{47};
    bool rf_amp{false};

    SpectrumInputImageView input_image{nav_};
    SpectrumInputTextView input_text{nav_};

    std::array<View*, 2> input_views{{&input_image, &input_text}};

    TabView tab_view{
        {"Image", Theme::getInstance()->bg_darkest->foreground, input_views[0]},
        {"Text", Theme::getInstance()->bg_darkest->foreground, input_views[1]}};

    static constexpr int32_t footer_location = 15 * 16 + 8;
    ProgressBar progressbar{
        {4, footer_location - 16, 240 - 8, 16}};

    Labels labels{
        {{10 * 8, footer_location + 1 * 16}, "GAIN   A:", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, footer_location + 2 * 16}, "BW:      Du:    P:", Theme::getInstance()->fg_light->foreground},
    };

    TxFrequencyField field_frequency{
        {0 * 8, footer_location + 1 * 16},
        nav_};

    NumberField field_rfgain{
        {14 * 8, footer_location + 1 * 16},
        2,
        {0, 47},
        1,
        ' '};

    NumberField field_rfamp{
        {19 * 8, footer_location + 1 * 16},
        2,
        {0, 14},
        14,
        ' '};

    Checkbox check_loop{
        {21 * 8, footer_location + 1 * 16},
        4,
        LanguageHelper::currentMessages[LANG_LOOP],
        true};

    ImageButton button_play{
        {28 * 8, footer_location + 1 * 16, 2 * 8, 1 * 16},
        &bitmap_play,
        Theme::getInstance()->fg_green->foreground,
        Theme::getInstance()->fg_green->background};

    OptionsField option_bandwidth{
        {4 * 8, footer_location + 2 * 16},
        5,
        {}};

    NumberField field_duration{
        {13 * 8, footer_location + 2 * 16},
        3,
        {1, 999},
        1,
        ' '};

    NumberField field_pause{
        {19 * 8, footer_location + 2 * 16},
        2,
        {0, 99},
        1,
        ' '};

    SpectrumPainterFIFO* fifo{nullptr};
    void start_tx();
    void frame_sync();
    void stop_tx();

    MessageHandlerRegistration message_handler_fifo_signal{
        Message::ID::SpectrumPainterBufferResponseConfigure,
        [this](const Message* const p) {
            const auto message = static_cast<const SpectrumPainterBufferConfigureResponseMessage*>(p);
            this->fifo = message->fifo;
            this->start_tx();
        }};

    MessageHandlerRegistration message_handler_sample{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->frame_sync();
        }};
};

}  // namespace ui::external_app::spainter
