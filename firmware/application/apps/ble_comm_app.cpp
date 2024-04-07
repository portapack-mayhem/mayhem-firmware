/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
 * Copyright (C) 2023 TJ Baginski
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

#include "ble_comm_app.hpp"

#include "ui_modemsetup.hpp"

#include "modems.hpp"
#include "audio.hpp"
#include "rtc_time.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"
#include "ui_text.hpp"
#include "file_path.hpp"

using namespace portapack;
using namespace modems;

void BLECommLogger::log_raw_data(const std::string& data) {
    log_file.write_entry(data);
}

namespace ui {
static std::uint64_t get_freq_by_channel_number(uint8_t channel_number) {
    uint64_t freq_hz;

    switch (channel_number) {
        case 37:
            freq_hz = 2'402'000'000ull;
            break;
        case 38:
            freq_hz = 2'426'000'000ull;
            break;
        case 39:
            freq_hz = 2'480'000'000ull;
            break;
        case 0 ... 10:
            freq_hz = 2'404'000'000ull + channel_number * 2'000'000ull;
            break;
        case 11 ... 36:
            freq_hz = 2'428'000'000ull + (channel_number - 11) * 2'000'000ull;
            break;
        default:
            freq_hz = UINT64_MAX;
    }

    return freq_hz;
}

void BLECommView::focus() {
    options_channel.focus();
}

BLECommView::BLECommView(NavigationView& nav)
    : nav_{nav} {
    add_children({&rssi,
                  &channel,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &options_channel,
                  &field_frequency,
                  &label_send_adv,
                  &button_send_adv,
                  &check_log,
                  &label_packets_sent,
                  &text_packets_sent,
                  &console});

    field_frequency.set_step(0);

    button_send_adv.on_select = [this](ImageButton&) {
        this->toggle();
    };

    check_log.set_value(logging);

    check_log.on_select = [this](Checkbox&, bool v) {
        str_log = "";
        logging = v;

        if (logger && logging)
            logger->append(logs_dir.string() + "/BLELOG_" + to_string_timestamp(rtc_time::now()) + ".TXT");
    };

    options_channel.on_change = [this](size_t, int32_t i) {
        // If we selected Auto don't do anything and Auto will handle changing.
        if (i == 40) {
            auto_channel = true;
            return;
        } else {
            auto_channel = false;
        }

        field_frequency.set_value(get_freq_by_channel_number(i));
        channel_number_rx = i;
        channel_number_tx = i;
    };

    options_channel.set_selected_index(3, true);

    logger = std::make_unique<BLECommLogger>();

    // Generate new random Mac Address upon each new startup.
    generateRandomMacAddress(randomMac);

    // Setup Initial Advertise Packet.
    advertisePacket = build_adv_packet();
}

void BLECommView::set_parent_rect(const Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);
    const Rect content_rect{0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height};
    console.set_parent_rect(content_rect);
}

BLECommView::~BLECommView() {
    receiver_model.disable();
    transmitter_model.disable();
    baseband::shutdown();
}

bool BLECommView::in_tx_mode() const {
    return (bool)is_running_tx;
}

void BLECommView::toggle() {
    if (in_tx_mode()) {
        sendAdvertisement(false);
    } else {
        sendAdvertisement(true);
    }
}

BLETxPacket BLECommView::build_adv_packet() {
    BLETxPacket bleTxPacket;
    memset(&bleTxPacket, 0, sizeof(BLETxPacket));

    std::string dataString = "11094861636b524620506f7274617061636b";

    strncpy(bleTxPacket.macAddress, randomMac, 12);
    strncpy(bleTxPacket.advertisementData, dataString.c_str(), dataString.length());

    // Duty cycle of 40% per 100ms advertisment periods.
    strncpy(bleTxPacket.packetCount, "80", 3);
    bleTxPacket.packet_count = 80;

    bleTxPacket.packetType = PKT_TYPE_DISCOVERY;

    return bleTxPacket;
}

void BLECommView::sendAdvertisement(bool enable) {
    if (enable) {
        startTx(advertisePacket);
        is_adv = true;
    } else {
        is_adv = false;
        stopTx();
    }
}

void BLECommView::startTx(BLETxPacket packetToSend) {
    int randomChannel = channel_number_tx;

    if (auto_channel) {
        int min = 37;
        int max = 39;

        randomChannel = min + std::rand() % (max - min + 1);

        field_frequency.set_value(get_freq_by_channel_number(randomChannel));
    }

    if (!in_tx_mode()) {
        switch_rx_tx(false);

        currentPacket = packetToSend;
        packet_counter = currentPacket.packet_count;

        button_send_adv.set_bitmap(&bitmap_stop);
        baseband::set_btletx(randomChannel, currentPacket.macAddress, currentPacket.advertisementData, currentPacket.packetType);
        transmitter_model.set_tx_gain(47);
        transmitter_model.enable();

        is_running_tx = true;
    } else {
        baseband::set_btletx(randomChannel, currentPacket.macAddress, currentPacket.advertisementData, currentPacket.packetType);
    }

    if ((packet_counter % 10) == 0) {
        text_packets_sent.set(to_string_dec_uint(packet_counter));
    }

    is_sending = true;

    packet_counter--;
}

void BLECommView::stopTx() {
    button_send_adv.set_bitmap(&bitmap_play);
    text_packets_sent.set(to_string_dec_uint(packet_counter));

    switch_rx_tx(true);

    baseband::set_btlerx(channel_number_rx);
    receiver_model.enable();

    is_running_tx = false;
}

void BLECommView::switch_rx_tx(bool inRxMode) {
    if (inRxMode) {
        // Start Rx
        transmitter_model.disable();
        baseband::shutdown();
        baseband::run_image(portapack::spi_flash::image_tag_btle_rx);
    } else {
        // Start Tx
        receiver_model.disable();
        baseband::shutdown();
        baseband::run_image(portapack::spi_flash::image_tag_btle_tx);
    }
}

void BLECommView::on_data(BlePacketData* packet) {
    parse_received_packet(packet, (ADV_PDU_TYPE)packet->type);
}

// called each 1/60th of second, so 6 = 100ms
void BLECommView::on_timer() {
    // Send advertise burst only once every 100ms
    if (++timer_counter == timer_period) {
        timer_counter = 0;

        if (!is_adv) {
            sendAdvertisement(true);
        }
    }
}

void BLECommView::on_tx_progress(const bool done) {
    if (done) {
        if (in_tx_mode()) {
            is_sending = false;

            if (packet_counter == 0) {
                if (is_adv) {
                    sendAdvertisement(false);
                } else {
                    stopTx();
                }
            } else {
                startTx(currentPacket);
            }
        }
    }
}

void BLECommView::parse_received_packet(const BlePacketData* packet, ADV_PDU_TYPE pdu_type) {
    std::string data_string;

    int i;

    for (i = 0; i < packet->dataLen; i++) {
        data_string += to_string_hex(packet->data[i], 2);
    }

    receivedPacket.dbValue = packet->max_dB;
    receivedPacket.timestamp = to_string_timestamp(rtc_time::now());
    receivedPacket.dataString = data_string;

    receivedPacket.packetData.type = packet->type;
    receivedPacket.packetData.size = packet->size;
    receivedPacket.packetData.dataLen = packet->dataLen;

    receivedPacket.packetData.macAddress[0] = packet->macAddress[0];
    receivedPacket.packetData.macAddress[1] = packet->macAddress[1];
    receivedPacket.packetData.macAddress[2] = packet->macAddress[2];
    receivedPacket.packetData.macAddress[3] = packet->macAddress[3];
    receivedPacket.packetData.macAddress[4] = packet->macAddress[4];
    receivedPacket.packetData.macAddress[5] = packet->macAddress[5];

    receivedPacket.numHits++;

    for (int i = 0; i < packet->dataLen; i++) {
        receivedPacket.packetData.data[i] = packet->data[i];
    }

    if (pdu_type == SCAN_REQ || pdu_type == CONNECT_REQ) {
        ADV_PDU_PAYLOAD_TYPE_1_3* directed_mac_data = (ADV_PDU_PAYLOAD_TYPE_1_3*)packet->data;

        std::reverse(directed_mac_data->A1, directed_mac_data->A1 + 6);
        console.clear(true);
        std::string str_console = "";
        std::string pduTypeStr = "";

        if (pdu_type == SCAN_REQ) {
            pduTypeStr += "SCAN_REQ";
        } else if (pdu_type == CONNECT_REQ) {
            pduTypeStr += "CONNECT_REQ";
        }

        str_console += "PACKET TYPE:" + pduTypeStr + "\n";
        str_console += "MY MAC:" + to_string_formatted_mac_address(randomMac) + "\n";
        str_console += "SCAN MAC:" + to_string_mac_address(directed_mac_data->A1, 6, false) + "\n";
        console.write(str_console);
    }
}

} /* namespace ui */
