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

#include "ui_kiss_tnc.hpp"
#include "baseband_api.hpp"
#include "portapack.hpp"
#include "receiver_model.hpp"
#include "transmitter_model.hpp"
#include "string_format.hpp"
#include "memory_map.hpp"
#include "ax25.hpp"
#include "spi_image.hpp"

extern "C" {
#include "usb_serial_device_to_host.h"
}

using namespace portapack;

namespace ui::external_app::kiss_tnc {

namespace {
KissTncView* g_kiss_tnc_instance = nullptr;
}

void KissTncView::kiss_input_trampoline(const uint8_t* data, size_t len) {
    if (g_kiss_tnc_instance) g_kiss_tnc_instance->on_kiss_bytes(data, len);
}

KissTncView::KissTncView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());
    add_children({&field_frequency,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &rssi,
                  &channel,
                  &text_status,
                  &text_usb_status,
                  &text_rx_count,
                  &text_tx_count,
                  &labels,
                  &console});
    receiver_model.enable();
    baseband::set_aprs(1200);
    g_kiss_tnc_instance = this;
    usb_input_handler_.emplace(kiss_input_trampoline);
    update_stats();
}

KissTncView::~KissTncView() {
    g_kiss_tnc_instance = nullptr;
    usb_input_handler_.reset();
    if (tx_active_) {
        transmitter_model.disable();
    } else {
        receiver_model.disable();
    }
    baseband::shutdown();
}

void KissTncView::focus() {
    field_frequency.focus();
}

void KissTncView::update_stats() {
    text_rx_count.set(to_string_dec_uint(rx_count_));
    text_tx_count.set(to_string_dec_uint(tx_count_));
    text_usb_status.set(portapack::usb_serial.serial_connected() ? "Connected" : "Disconnected");
}

void KissTncView::send_kiss_frame(const uint8_t* data, size_t len) {
    // Stack-allocate output buffer: max KISS overhead is 2x data + 2 FENDs + 1 cmd byte
    uint8_t buf[512];
    size_t i = 0;
    buf[i++] = 0xC0;  // FEND
    buf[i++] = 0x00;  // data command
    for (size_t j = 0; j < len; j++) {
        if (data[j] == 0xC0) {
            if (i + 3 > sizeof(buf)) break;  // need 2 for escape + 1 for final FEND
            buf[i++] = 0xDB;
            buf[i++] = 0xDC;
        } else if (data[j] == 0xDB) {
            if (i + 3 > sizeof(buf)) break;  // need 2 for escape + 1 for final FEND
            buf[i++] = 0xDB;
            buf[i++] = 0xDD;
        } else {
            if (i + 2 > sizeof(buf)) break;  // need 1 for data + 1 for final FEND
            buf[i++] = data[j];
        }
    }
    buf[i++] = 0xC0;  // FEND
    if (usb_input_handler_)
        usb_input_handler_->write(buf, i);
}

void KissTncView::on_packet(const APRSPacketMessage* message) {
    aprs::APRSPacket pkt = message->packet;

    uint8_t payload_size = pkt.size();
    if (payload_size > 2) {
        uint8_t raw[256];
        size_t raw_len = payload_size - 2;
        for (size_t i = 0; i < raw_len; i++)
            raw[i] = static_cast<uint8_t>(pkt[i]);
        send_kiss_frame(raw, raw_len);
    }

    console.writeln(pkt.get_source_formatted() + ">" + pkt.get_destination_formatted());
    rx_count_++;
    update_stats();
}

void KissTncView::on_kiss_bytes(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        uint8_t b = data[i];
        switch (kiss_state_) {
            case KissState::IDLE:
                if (b == 0xC0) kiss_state_ = KissState::CMD;
                break;
            case KissState::CMD:
                if (b == 0xC0) break;
                if (b == 0x00) {
                    kiss_idx_ = 0;
                    kiss_state_ = KissState::DATA;
                } else {
                    kiss_state_ = KissState::IDLE;
                }
                break;
            case KissState::DATA:
                if (b == 0xC0) {
                    if (kiss_idx_ > 0) process_kiss_frame();
                    kiss_idx_ = 0;
                    kiss_state_ = KissState::CMD;
                } else if (b == 0xDB) {
                    kiss_state_ = KissState::ESC;
                } else if (kiss_idx_ < sizeof(kiss_buf_)) {
                    kiss_buf_[kiss_idx_++] = b;
                }
                break;
            case KissState::ESC:
                if (b == 0xDC) {
                    if (kiss_idx_ < sizeof(kiss_buf_)) kiss_buf_[kiss_idx_++] = 0xC0;
                } else if (b == 0xDD) {
                    if (kiss_idx_ < sizeof(kiss_buf_)) kiss_buf_[kiss_idx_++] = 0xDB;
                }
                kiss_state_ = KissState::DATA;
                break;
        }
    }
}

void KissTncView::process_kiss_frame() {
    if (tx_active_ || kiss_idx_ == 0) return;
    // AFSK shared buffer is 512 bytes (256 uint16_t words). Each raw byte
    // produces ~10 encoded bits after bit-stuffing + flags, so cap at 200 bytes
    // to ensure the bitstream plus required 0-word terminator always fits.
    if (kiss_idx_ > 200) return;
    start_tx();
}

void KissTncView::start_tx() {
    tx_active_ = true;

    ax25::AX25Frame frame;
    frame.make_frame_from_raw(kiss_buf_, kiss_idx_);

    rx_frequency_ = receiver_model.target_frequency();
    receiver_model.disable();
    baseband::shutdown();

    baseband::run_image(portapack::spi_flash::image_tag_afsk);

    transmitter_model.set_target_frequency(rx_frequency_);
    transmitter_model.set_sampling_rate(AFSK_SAMPLE_RATE);
    transmitter_model.set_baseband_bandwidth(AFSK_BASEBAND_BW);
    transmitter_model.enable();

    baseband::set_afsk_data(AFSK_SAMPLE_RATE / 1200, 1200, 2200, 1, 10000, 8);

    text_status.set("Transmitting");
    tx_count_++;
    update_stats();
}

void KissTncView::finish_tx() {
    transmitter_model.disable();
    baseband::shutdown();

    // Must reload from SPI flash — run_image(afsk) overwrote m4_code.base()
    baseband::run_image(portapack::spi_flash::image_tag_aprs_rx);

    receiver_model.set_target_frequency(rx_frequency_);
    receiver_model.set_sampling_rate(APRS_RX_SAMPLE_RATE);
    receiver_model.set_baseband_bandwidth(APRS_RX_BASEBAND_BW);
    receiver_model.enable();
    baseband::set_aprs(1200);
    tx_active_ = false;
    text_status.set("Listening");
    update_stats();
}

}  // namespace ui::external_app::kiss_tnc
