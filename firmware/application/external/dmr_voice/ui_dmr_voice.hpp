/*
 * Copyright (C) 2026 Belousov Oleg aka R1CBU
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

#ifndef UI_DMR_VOICE_HPP
#define UI_DMR_VOICE_HPP

#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "radio_state.hpp"
#include "app_settings.hpp"
#include "message.hpp"

namespace ui::external_app::dmr_voice {
class DmrVoiceView : public View {
   public:
    explicit DmrVoiceView(NavigationView& nav);
    ~DmrVoiceView();

    void focus() override { field_frequency.focus(); }
    std::string title() const override { return "DMR Voice"; }

   private:
    NavigationView& nav_;
    rf::Frequency frequency{433450000};
    RxRadioState radio_state_{};
    RxFrequencyField field_frequency{{UI_POS_X(0), UI_POS_Y(0)}, nav_};
    RFAmpField field_rf_amp{{UI_POS_X(13), UI_POS_Y(0)}};
    LNAGainField field_lna{{UI_POS_X(15), UI_POS_Y(0)}};
    VGAGainField field_vga{{UI_POS_X(18), UI_POS_Y(0)}};
    AudioVolumeField volume{{UI_POS_X(26), UI_POS_Y(0)}};
    Text status{{UI_POS_X(0), UI_POS_Y(2), UI_POS_WIDTH(30), 16}, "Starting..."};
    Text color{{UI_POS_X(0), UI_POS_Y(4), UI_POS_WIDTH(30), 16}, "CC --"};
    Text source{{UI_POS_X(0), UI_POS_Y(6), UI_POS_WIDTH(30), 16}, "SRC --"};
    Text destination{{UI_POS_X(0), UI_POS_Y(8), UI_POS_WIDTH(30), 16}, "DST --"};

    bool output_enabled = false;

    app_settings::SettingsManager settings_{
        "rx_dmr_voice",
        app_settings::Mode::RX,
        {{"frequency", &frequency}}};

    void on_stats(const DmrVoiceStatusMessage& result);

    MessageHandlerRegistration handler{
        Message::ID::DmrVoiceStatus,
        [this](const Message* message) { on_stats(*static_cast<const DmrVoiceStatusMessage*>(message)); }};
};

}  // namespace ui::external_app::dmr_voice
#endif
