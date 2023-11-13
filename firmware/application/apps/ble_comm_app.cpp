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
            logger->append(LOG_ROOT_DIR "/BLELOG_" + to_string_timestamp(rtc_time::now()) + ".TXT");
    };

    options_channel.on_change = [this](size_t, int32_t i) {
        field_frequency.set_value(get_freq_by_channel_number(i));
        channel_number_rx = i;
    };

    options_channel.set_selected_index(0, true);

    logger = std::make_unique<BLECommLogger>();

    // Generate new random Mac Address upon each new startup.
    generateRandomMacAddress(randomMac);
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

bool BLECommView::is_sending_tx() const {
    return (bool)is_running_tx;
}

void BLECommView::toggle() {
    if (is_sending_tx()) {
        stopTx();
    } else {
        startTx(build_adv_packet(), PKT_TYPE_DISCOVERY);
    }
}

BLETxPacket BLECommView::build_adv_packet() {
    BLETxPacket bleTxPacket;
    memset(&bleTxPacket, 0, sizeof(BLETxPacket));

    std::string dataString = "11094861636b524620506f7274617061636b";

    strncpy(bleTxPacket.macAddress, randomMac, 12);
    strncpy(bleTxPacket.advertisementData, dataString.c_str(), dataString.length());

    // Little note that, at 64 timer, 40 packets is around 1 second. So this should advertise for 5 seconds for 200 packets.
    strncpy(bleTxPacket.packetCount, "200", 4);
    bleTxPacket.packet_count = 200;

    return bleTxPacket;
}

void BLECommView::startTx(BLETxPacket packetToSend, PKT_TYPE pduType) {
    if (!is_sending_tx()) {
        switch_rx_tx(false);

        packet_counter = packetToSend.packet_count;

        button_send_adv.set_bitmap(&bitmap_stop);
        baseband::set_btletx(channel_number_tx, packetToSend.macAddress, packetToSend.advertisementData, pduType);
        transmitter_model.enable();

        is_running_tx = true;
    } else {
        baseband::set_btletx(channel_number_tx, packetToSend.macAddress, packetToSend.advertisementData, pduType);
    }

    if ((packet_counter % 10) == 0) {
        text_packets_sent.set(to_string_dec_uint(packet_counter));
    }

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
    std::string str_console = "";

    if (!logging) {
        str_log = "";
    }

    switch ((ADV_PDU_TYPE)packet->type) {
        case ADV_IND:
            str_console += "ADV_IND";
            break;
        case ADV_DIRECT_IND:
            str_console += "ADV_DIRECT_IND";
            break;
        case ADV_NONCONN_IND:
            str_console += "ADV_NONCONN_IND";
            break;
        case SCAN_REQ:
            str_console += "SCAN_REQ";
            break;
        case SCAN_RSP:
            str_console += "SCAN_RSP";
            break;
        case CONNECT_REQ:
            str_console += "CONNECT_REQ";
            break;
        case ADV_SCAN_IND:
            str_console += "ADV_SCAN_IND";
            break;
        case RESERVED0:
        case RESERVED1:
        case RESERVED2:
        case RESERVED3:
        case RESERVED4:
        case RESERVED5:
        case RESERVED6:
        case RESERVED7:
        case RESERVED8:
            str_console += "RESERVED";
            break;
        default:
            str_console += "UNKNOWN";
            break;
    }

    str_console += " Len:";
    str_console += to_string_dec_uint(packet->size);

    str_console += "\n";

    str_console += "Mac:";
    str_console += to_string_mac_address(packet->macAddress, 6, false);

    str_console += "\n";
    str_console += "Data:";

    int i;

    for (i = 0; i < packet->dataLen; i++) {
        str_console += to_string_hex(packet->data[i], 2);
    }

    str_console += "\n";

    console.write(str_console);

    // uint64_t macAddressEncoded = copy_mac_address_to_uint64(packet->macAddress);

    parse_received_packet(packet, (ADV_PDU_TYPE)packet->type);

    // Log at End of Packet.
    if (logger && logging) {
        logger->log_raw_data(str_console);
    }
}

void BLECommView::on_tx_progress(const bool done) {
    if (done) {
        if (is_sending_tx()) {
            // Reached end of current packet repeats.
            if (packet_counter == 0) {
                stopTx();
            } else {
                if ((timer_count % timer_period) == 0) {
                    startTx(build_adv_packet(), PKT_TYPE_DISCOVERY);
                }
            }

            timer_count++;
        }
    }
}

void BLECommView::parse_received_packet(const BlePacketData* packet, ADV_PDU_TYPE pdu_type) {
    std::string data_string;

    int i;

    for (i = 0; i < packet->dataLen; i++) {
        data_string += to_string_hex(packet->data[i], 2);
    }

    currentPacket.dbValue = packet->max_dB;
    currentPacket.timestamp = to_string_timestamp(rtc_time::now());
    currentPacket.dataString = data_string;

    currentPacket.packetData.type = packet->type;
    currentPacket.packetData.size = packet->size;
    currentPacket.packetData.dataLen = packet->dataLen;

    currentPacket.packetData.macAddress[0] = packet->macAddress[0];
    currentPacket.packetData.macAddress[1] = packet->macAddress[1];
    currentPacket.packetData.macAddress[2] = packet->macAddress[2];
    currentPacket.packetData.macAddress[3] = packet->macAddress[3];
    currentPacket.packetData.macAddress[4] = packet->macAddress[4];
    currentPacket.packetData.macAddress[5] = packet->macAddress[5];

    currentPacket.numHits++;

    for (int i = 0; i < packet->dataLen; i++) {
        currentPacket.packetData.data[i] = packet->data[i];
    }

    std::string nameString;

    // Only parse name for advertisment packets and empty name entries
    if ((pdu_type == ADV_IND || pdu_type == ADV_NONCONN_IND || pdu_type == SCAN_RSP || pdu_type == ADV_SCAN_IND) && nameString.empty()) {
        ADV_PDU_PAYLOAD_TYPE_0_2_4_6* advertiseData = (ADV_PDU_PAYLOAD_TYPE_0_2_4_6*)packet->data;

        uint8_t currentByte = 0;
        uint8_t length = 0;
        uint8_t type = 0;

        std::string decoded_data;
        for (currentByte = 0; (currentByte < packet->dataLen);) {
            length = advertiseData->Data[currentByte++];
            type = advertiseData->Data[currentByte++];

            // Subtract 1 because type is part of the length.
            for (int i = 0; i < length - 1; i++) {
                if (type == 0x08 || type == 0x09) {
                    decoded_data += (char)advertiseData->Data[currentByte];
                }
                currentByte++;
            }
            if (!decoded_data.empty()) {
                nameString = std::move(decoded_data);
                break;
            }
        }
    }
}

} /* namespace ui */
