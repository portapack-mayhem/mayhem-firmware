/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
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

#include "receiver_model.hpp"

#include "ui_receiver.hpp"

namespace ui {

class ToneSearchView : public View {
   public:
    ToneSearchView(NavigationView& nav);
    ~ToneSearchView();

    void focus() override;

    std::string title() const override { return "Tone search"; };

   private:
    NavigationView& nav_;

    Labels labels{
        {{0 * 8, 0 * 8}, "LNA:   VGA:   AMP:", Theme::getInstance()->fg_light->foreground}};

    LNAGainField field_lna{
        {4 * 8, 0 * 16}};

    VGAGainField field_vga{
        {11 * 8, 0 * 16}};

    RFAmpField field_rf_amp{
        {18 * 8, 0 * 16}};

    /*
        MessageHandlerRegistration message_handler_frame_sync {
                Message::ID::DisplayFrameSync,
                [this](const Message* const) {
                        if( this->fifo ) {
                                ChannelSpectrum channel_spectrum;
                                while( fifo->out(channel_spectrum) ) {
                                        this->on_channel_spectrum(channel_spectrum);
                                }
                        }
                        this->do_timers();
                }
        };*/
};

} /* namespace ui */
