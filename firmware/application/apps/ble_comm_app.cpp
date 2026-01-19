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
    field_frequency.focus();
}

BLECommView::BLECommView(NavigationView& nav)
    : nav_{nav} {
    add_children({&rssi,
                  &channel,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &field_frequency,
                  &check_log,
                  &label_packets_sent,
                  &text_packets_sent,
                  &console});

    field_frequency.set_step(0);

    check_log.set_value(logging);

    check_log.on_select = [this](Checkbox&, bool v) {
        str_log = "";
        logging = v;

        if (logger && logging)
            logger->append(logs_dir.string() + "/BLELOG_" + to_string_timestamp(rtc_time::now()) + ".TXT");
    };

    logger = std::make_unique<BLECommLogger>();

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

BLETxPacket BLECommView::build_adv_packet() {
    BLETxPacket bleTxPacket;
    memset(&bleTxPacket, 0, sizeof(BLETxPacket));

    std::string dataString = "02010603030F1807084861636B5246";
    strncpy(bleTxPacket.advertisementData, dataString.c_str(), dataString.length());

    strncpy(bleTxPacket.packetCount, "1", 2);
    bleTxPacket.packet_count = 1;

    bleTxPacket.pduType = PKT_TYPE_ADV_IND;

    return bleTxPacket;
}

void BLECommView::sendAdvertisement(void) {
    startTx(advertisePacket);
    ble_state = Ble_State_Advertising;
}

void BLECommView::startTx(BLETxPacket packetToSend) {
    switch_rx_tx(false);

    currentPacket = packetToSend;
    packet_counter = currentPacket.packet_count;

    switch (advCount) {
        case 0:
            channel_number_tx = 37;
            break;
        case 1:
            channel_number_tx = 38;
            break;
        case 2:
            channel_number_tx = 39;
            break;
    }

    field_frequency.set_value(get_freq_by_channel_number(37));

    advCount++;

    if (advCount == 3) {
        advCount = 0;
    }

    baseband::set_btletx(37, deviceMAC, currentPacket.advertisementData, currentPacket.pduType);
    transmitter_model.set_tx_gain(47);
    transmitter_model.enable();

    is_running_tx = true;
}

void BLECommView::stopTx() {
    text_packets_sent.set(to_string_dec_uint(packet_counter));

    switch_rx_tx(true);

    baseband::set_btlerx(channel_number_rx);
    field_frequency.set_value(get_freq_by_channel_number(channel_number_rx));
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

void BLECommView::on_timer() {
    if (++timer_counter == timer_period) {
        timer_counter = 0;

        if (ble_state == Ble_State_Receiving || ble_state == Ble_State_Idle) {
            sendAdvertisement();
        }
    }
}

void BLECommView::on_tx_progress(const bool done) {
    if (done) {
        if (in_tx_mode()) {
            ble_state = Ble_State_Receiving;

            timer_counter = 0;
            timer_period = 12;

            stopTx();

            // else
            // {
            //     startTx(advertisePacket);
            //     advCount++;
            // }
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
        if (pdu_type == SCAN_REQ) {
            ADV_PDU_PAYLOAD_TYPE_1_3* directed_mac_data = (ADV_PDU_PAYLOAD_TYPE_1_3*)packet->data;

            std::reverse(directed_mac_data->A1, directed_mac_data->A1 + 6);
            std::string directedMAC = to_string_mac_address(directed_mac_data->A1, 6, false);

            // Compare directed MAC Hex with the device MAC.
            if (1) {
                // std::string str_console = "Received SCAN_REQ from directed MAC: " + directedMAC + "\n";
                // console.clear(true);
                // console.writeln(str_console);
            }
        } else if (pdu_type == CONNECT_REQ) {
            ADV_PDU_PAYLOAD_TYPE_5* connectReq = (ADV_PDU_PAYLOAD_TYPE_5*)packet->data;

            std::reverse(connectReq->AdvA, connectReq->AdvA + 6);
            std::string directedMAC = to_string_mac_address(connectReq->AdvA, 6, false);

            std::string str_console = "Received CONNECT_REQ from directed MAC: " + directedMAC + "\n";
            console.clear(true);
            console.writeln(str_console);
        }
    }
}

} /* namespace ui */
