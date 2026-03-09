/*
 * Copyright (C) 2024 Sarah Rose
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
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_freq_field.hpp"
#include "ui_rssi.hpp"
#include "radio_state.hpp"
#include "app_settings.hpp"
#include "message.hpp"
#include "baseband_api.hpp"
#include "portapack.hpp"
#include "receiver_model.hpp"
#include "aprs_packet.hpp"
#include "usb_serial_host_to_device.hpp"

#include <optional>

namespace ui::external_app::kiss_tnc {

class KissTncView : public View {
   public:
    KissTncView(NavigationView& nav);
    ~KissTncView();

    void focus() override;
    std::string title() const override { return "KISS TNC"; }

   private:
    static constexpr rf::Frequency APRS_FREQ_DEFAULT = 144390000;
    static constexpr uint32_t APRS_RX_SAMPLE_RATE = 3072000U;
    static constexpr uint32_t APRS_RX_BASEBAND_BW = 1750000U;
    static constexpr uint32_t AFSK_SAMPLE_RATE = 1536000U;
    static constexpr uint32_t AFSK_BASEBAND_BW = 1750000U;

    NavigationView& nav_;

    // CRITICAL: RxRadioState must be declared before SettingsManager
    RxRadioState radio_state_{
        APRS_FREQ_DEFAULT,
        APRS_RX_BASEBAND_BW,
        APRS_RX_SAMPLE_RATE};

    app_settings::SettingsManager settings_{"kiss_tnc", app_settings::Mode::RX};

    RxFrequencyField field_frequency{
        {0, 0},
        nav_};
    RFAmpField field_rf_amp{{13 * 8, 0}};
    LNAGainField field_lna{{15 * 8, 0}};
    VGAGainField field_vga{{18 * 8, 0}};
    RSSI rssi{{21 * 8, 0, 6 * 8, 4}};
    Channel channel{{21 * 8, 5, 6 * 8, 4}};

    Text text_status{{0, 16, 240, 16}, "Listening"};
    Text text_usb_status{{4 * 8, 2 * 16, 14 * 8, 16}, "Disconnected"};
    Text text_rx_count{{3 * 8, 3 * 16, 5 * 8, 16}, "0"};
    Text text_tx_count{{11 * 8, 3 * 16, 5 * 8, 16}, "0"};

    Labels labels{{
        {{0, 2 * 16}, "USB:", Color::light_grey()},
        {{0, 3 * 16}, "RX:", Color::light_grey()},
        {{8 * 8, 3 * 16}, "TX:", Color::light_grey()},
    }};

    Console console{{0, 4 * 16, 240, 180}};

    enum class KissState : uint8_t { IDLE,
                                     CMD,
                                     DATA,
                                     ESC };
    KissState kiss_state_{KissState::IDLE};
    uint8_t kiss_buf_[350]{};
    size_t kiss_idx_{0};

    bool tx_active_{false};
    rf::Frequency rx_frequency_{APRS_FREQ_DEFAULT};
    uint32_t rx_count_{0};
    uint32_t tx_count_{0};

    void on_packet(const APRSPacketMessage* message);
    void on_kiss_bytes(const uint8_t* data, size_t len);
    void process_kiss_frame();
    void send_kiss_frame(const uint8_t* data, size_t len);
    void start_tx();
    void finish_tx();
    void update_stats();

    uint32_t frame_counter_{0};

    static void kiss_input_trampoline(const uint8_t* data, size_t len);
    std::optional<UsbSerialInputHandler> usb_input_handler_{};

    MessageHandlerRegistration message_handler_packet{
        Message::ID::APRSPacket,
        [this](Message* const p) {
            on_packet(reinterpret_cast<const APRSPacketMessage*>(p));
        }};

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](Message* const p) {
            auto* msg = reinterpret_cast<const TXProgressMessage*>(p);
            if (msg->done) finish_tx();
        }};

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            if (++frame_counter_ == 60) {
                frame_counter_ = 0;
                text_usb_status.set(portapack::usb_serial.serial_connected() ? "Connected" : "Disconnected");
            }
        }};
};

}  // namespace ui::external_app::kiss_tnc
