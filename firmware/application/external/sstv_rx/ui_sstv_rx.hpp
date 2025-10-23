/*
 * Copyright (C) 2025 Pezsma
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

#ifndef __SSTV_RX_H__
#define __SSTV_RX_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_language.hpp"
#include "ui_widget.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "ui_freqman.hpp"
#include "string_format.hpp"
#include "audio.hpp"
#include "radio_state.hpp"
#include "log_file.hpp"
#include "utility.hpp"
#include <ch.h>

namespace ui::external_app::sstv_rx {

#define FMR_BTNGRID_TOP 60

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
    //   void onPress();
    //   void onRelease();
      ui::NavigationView& nav_;

      ReceiverModel::Mode receiver_mode = ReceiverModel::Mode::WidebandFMAudio;
      AudioSpectrum* audio_spectrum_data{nullptr};
      int16_t audio_spectrum[128]{0};
      RxRadioState radio_state_{};
      audio::Rate audio_sampling_rate = audio::Rate::Hz_48000;
      uint8_t radio_bw = 0;
      bool is_receiving = false;

      //UI Elements
      const Rect options_view_rect{UI_POS_X(0), UI_POS_Y(1), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)};
      const Rect nbfm_view_rect{UI_POS_X(0), UI_POS_Y(1), UI_POS_WIDTH(18), UI_POS_HEIGHT(1)};
      RFAmpField field_rf_amp{{13 * 8, UI_POS_Y(0)}};
      LNAGainField field_lna{{15 * 8, UI_POS_Y(0)}};
      VGAGainField field_vga{{18 * 8, UI_POS_Y(0)}};
      RSSI rssi{{UI_POS_X(21), 0, UI_POS_WIDTH_REMAINING(24), 4}};
      RxFrequencyField field_frequency{{UI_POS_X(0), UI_POS_Y(0)}, nav_};
      AudioVolumeField field_volume{{screen_width - 2 * 8, UI_POS_Y(0)}};
      OptionsField field_bw{{10 * 8, FMR_BTNGRID_TOP + 6 * 34}, 6, {}};
      Audio audio{{21 * 8, 10, 6 * 8, 4}};
      ui::Button start_btn{{UI_POS_X_CENTER(12), UI_POS_Y(3), UI_POS_WIDTH(12), UI_POS_HEIGHT(3)}, "Start RX"};
      ui::Button stop_btn{{UI_POS_X_CENTER(12), UI_POS_Y(8), UI_POS_WIDTH(12), UI_POS_HEIGHT(3)}, "Stop RX"};
      OptionsField field_modulation{
        {26 * 8, FMR_BTNGRID_TOP + 6 * 34},
        4,
        {{"AM", static_cast<int32_t>(ReceiverModel::Mode::AMAudio)},
         {"NFM", static_cast<int32_t>(ReceiverModel::Mode::NarrowbandFMAudio)},
         {"WFM", static_cast<int32_t>(ReceiverModel::Mode::WidebandFMAudio)},
         {"USB", static_cast<int32_t>(ReceiverModel::Mode::AMAudio)},
         {"LSB", static_cast<int32_t>(ReceiverModel::Mode::AMAudio)}}};
      Text text_ctcss{
         {UI_POS_X(16), UI_POS_Y(1), UI_POS_WIDTH(14), UI_POS_HEIGHT(1)},
         ""};

      std::unique_ptr<Widget> options_widget{};
      
      void on_audio_spectrum();
      void start_audio();
      void on_start();
      void on_stop();
};

} // namespace ui::external_app::sstv_rx

#endif  // __SSTV_RX_H__