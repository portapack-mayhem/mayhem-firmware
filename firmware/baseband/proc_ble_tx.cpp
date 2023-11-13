/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#include "proc_ble_tx.hpp"
#include "portapack_shared_memory.hpp"
#include "sine_table_int8.hpp"
#include "event_m4.hpp"

#include <cstdint>

#define new_way

int BTLETxProcessor::gen_sample_from_phy_bit(char* bit, char* sample, int num_bit) {
    int num_sample = (num_bit * SAMPLE_PER_SYMBOL) + (LEN_GAUSS_FILTER * SAMPLE_PER_SYMBOL);

    int8_t* tmp_phy_bit_over_sampling_int8 = (int8_t*)tmp_phy_bit_over_sampling;

    int i, j;

    for (i = 0; i < (LEN_GAUSS_FILTER * SAMPLE_PER_SYMBOL - 1); i++) {
        tmp_phy_bit_over_sampling_int8[i] = 0;
    }
    for (i = (LEN_GAUSS_FILTER * SAMPLE_PER_SYMBOL - 1 + num_bit * SAMPLE_PER_SYMBOL); i < (2 * LEN_GAUSS_FILTER * SAMPLE_PER_SYMBOL - 2 + num_bit * SAMPLE_PER_SYMBOL); i++) {
        tmp_phy_bit_over_sampling_int8[i] = 0;
    }
    for (i = 0; i < (num_bit * SAMPLE_PER_SYMBOL); i++) {
        if (i % SAMPLE_PER_SYMBOL == 0) {
            tmp_phy_bit_over_sampling_int8[i + (LEN_GAUSS_FILTER * SAMPLE_PER_SYMBOL - 1)] = (bit[i / SAMPLE_PER_SYMBOL]) * 2 - 1;
        } else {
            tmp_phy_bit_over_sampling_int8[i + (LEN_GAUSS_FILTER * SAMPLE_PER_SYMBOL - 1)] = 0;
        }
    }

    int16_t tmp = 0;
    sample[0] = cos_table_int8[tmp];
    sample[1] = sin_table_int8[tmp];

    int len_conv_result = num_sample - 1;
    for (i = 0; i < len_conv_result; i++) {
        int16_t acc = 0;
        for (j = 3; j < (LEN_GAUSS_FILTER * SAMPLE_PER_SYMBOL - 4); j++) {
            acc = acc + gauss_coef_int8[(LEN_GAUSS_FILTER * SAMPLE_PER_SYMBOL) - j - 1] * tmp_phy_bit_over_sampling_int8[i + j];
        }

        tmp = (tmp + acc) & 1023;
        sample[(i + 1) * 2 + 0] = cos_table_int8[tmp];
        sample[(i + 1) * 2 + 1] = sin_table_int8[tmp];
    }

    return (num_sample);
}

void BTLETxProcessor::octet_hex_to_bit(char* hex, char* bit) {
    char tmp_hex[3];

    tmp_hex[0] = hex[0];
    tmp_hex[1] = hex[1];
    tmp_hex[2] = 0;

    int n = strtol(tmp_hex, NULL, 16);

    bit[0] = 0x01 & (n >> 0);
    bit[1] = 0x01 & (n >> 1);
    bit[2] = 0x01 & (n >> 2);
    bit[3] = 0x01 & (n >> 3);
    bit[4] = 0x01 & (n >> 4);
    bit[5] = 0x01 & (n >> 5);
    bit[6] = 0x01 & (n >> 6);
    bit[7] = 0x01 & (n >> 7);
}

int BTLETxProcessor::convert_hex_to_bit(char* hex, char* bit, int stream_flip, int octet_limit) {
    int num_hex_orig = strlen(hex);

    int i, num_hex;
    num_hex = num_hex_orig;
    for (i = 0; i < num_hex_orig; i++) {
        if (!((hex[i] >= 48 && hex[i] <= 57) || (hex[i] >= 65 && hex[i] <= 70) || (hex[i] >= 97 && hex[i] <= 102)))  // not a hex
            num_hex--;
    }

    if (num_hex % 2 != 0) {
        return (-1);
    }

    if (num_hex > (octet_limit * 2)) {
        return (-1);
    }
    if (num_hex <= 1) {  // NULL data
        return (-1);
    }

    char tmp_str[max_char];

    if (stream_flip == 1) {
        strcpy(tmp_str, hex);
        for (i = 0; i < num_hex; i = i + 2) {
            hex[num_hex - i - 2] = tmp_str[i];
            hex[num_hex - i - 1] = tmp_str[i + 1];
        }
    }

    int num_bit = num_hex * 4;

    int j;
    for (i = 0; i < num_hex; i = i + 2) {
        j = i * 4;
        octet_hex_to_bit(hex + i, bit + j);
    }

    return (num_bit);
}

void BTLETxProcessor::crc24(char* bit_in, int num_bit, char* init_hex, char* crc_result) {
    char bit_store[24], bit_store_update[24];
    int i;
    convert_hex_to_bit(init_hex, bit_store, 0, 3);

    for (i = 0; i < num_bit; i++) {
        char new_bit = (bit_store[23] + bit_in[i]) % 2;
        bit_store_update[0] = new_bit;
        bit_store_update[1] = (bit_store[0] + new_bit) % 2;
        bit_store_update[2] = bit_store[1];
        bit_store_update[3] = (bit_store[2] + new_bit) % 2;
        bit_store_update[4] = (bit_store[3] + new_bit) % 2;
        bit_store_update[5] = bit_store[4];
        bit_store_update[6] = (bit_store[5] + new_bit) % 2;

        bit_store_update[7] = bit_store[6];
        bit_store_update[8] = bit_store[7];

        bit_store_update[9] = (bit_store[8] + new_bit) % 2;
        bit_store_update[10] = (bit_store[9] + new_bit) % 2;

        memcpy(bit_store_update + 11, bit_store + 10, 13);

        memcpy(bit_store, bit_store_update, 24);
    }

    for (i = 0; i < 24; i++) {
        crc_result[i] = bit_store[23 - i];
    }
}

void BTLETxProcessor::scramble(char* bit_in, int num_bit, int channel_number, char* bit_out) {
    char bit_store[7], bit_store_update[7];
    int i;

    bit_store[0] = 1;
    bit_store[1] = 0x01 & (channel_number >> 5);
    bit_store[2] = 0x01 & (channel_number >> 4);
    bit_store[3] = 0x01 & (channel_number >> 3);
    bit_store[4] = 0x01 & (channel_number >> 2);
    bit_store[5] = 0x01 & (channel_number >> 1);
    bit_store[6] = 0x01 & (channel_number >> 0);

    for (i = 0; i < num_bit; i++) {
        bit_out[i] = (bit_store[6] + bit_in[i]) % 2;

        bit_store_update[0] = bit_store[6];

        bit_store_update[1] = bit_store[0];
        bit_store_update[2] = bit_store[1];
        bit_store_update[3] = bit_store[2];

        bit_store_update[4] = (bit_store[3] + bit_store[6]) % 2;

        bit_store_update[5] = bit_store[4];
        bit_store_update[6] = bit_store[5];

        memcpy(bit_store, bit_store_update, 7);
    }
}

void BTLETxProcessor::disp_bit_in_hex(char* bit, int num_bit) {
    int i, a;

    for (i = 0; i < num_bit; i = i + 8) {
        a = bit[i] + bit[i + 1] * 2 + bit[i + 2] * 4 + bit[i + 3] * 8 + bit[i + 4] * 16 + bit[i + 5] * 32 + bit[i + 6] * 64 + bit[i + 7] * 128;

        data_message.is_data = true;
        data_message.value = (uint8_t)a;
        shared_memory.application_queue.push(data_message);
    }
}

void BTLETxProcessor::crc24_and_scramble_to_gen_phy_bit(char* crc_init_hex, PKT_INFO* pkt) {
    crc24(pkt->info_bit + 5 * 8, pkt->num_info_bit - 5 * 8, crc_init_hex, pkt->info_bit + pkt->num_info_bit);

    // disp_bit_in_hex(pkt->info_bit, pkt->num_info_bit + 3*8);

    scramble(pkt->info_bit + 5 * 8, pkt->num_info_bit - 5 * 8 + 24, pkt->channel_number, pkt->phy_bit + 5 * 8);
    memcpy(pkt->phy_bit, pkt->info_bit, 5 * 8);
    pkt->num_phy_bit = pkt->num_info_bit + 24;

    // disp_bit_in_hex(pkt->phy_bit, pkt->num_phy_bit);
}

void BTLETxProcessor::fill_adv_pdu_header(PKT_INFO* pkt, int txadd, int rxadd, int payload_len, char* bit_out) {
    if (pkt->pkt_type == ADV_IND || pkt->pkt_type == IBEACON) {
        bit_out[3] = 0;
        bit_out[2] = 0;
        bit_out[1] = 0;
        bit_out[0] = 0;
    } else if (pkt->pkt_type == ADV_DIRECT_IND) {
        bit_out[3] = 0;
        bit_out[2] = 0;
        bit_out[1] = 0;
        bit_out[0] = 1;
    } else if (pkt->pkt_type == ADV_NONCONN_IND || pkt->pkt_type == DISCOVERY) {
        bit_out[3] = 0;
        bit_out[2] = 0;
        bit_out[1] = 1;
        bit_out[0] = 0;
    } else if (pkt->pkt_type == SCAN_REQ) {
        bit_out[3] = 0;
        bit_out[2] = 0;
        bit_out[1] = 1;
        bit_out[0] = 1;
    } else if (pkt->pkt_type == SCAN_RSP) {
        bit_out[3] = 0;
        bit_out[2] = 1;
        bit_out[1] = 0;
        bit_out[0] = 0;
    } else if (pkt->pkt_type == CONNECT_REQ) {
        bit_out[3] = 0;
        bit_out[2] = 1;
        bit_out[1] = 0;
        bit_out[0] = 1;
    } else if (pkt->pkt_type == ADV_SCAN_IND) {
        bit_out[3] = 0;
        bit_out[2] = 1;
        bit_out[1] = 1;
        bit_out[0] = 0;
    } else {
        bit_out[3] = 1;
        bit_out[2] = 1;
        bit_out[1] = 1;
        bit_out[0] = 1;
    }

    bit_out[4] = 0;
    bit_out[5] = 0;

    bit_out[6] = txadd;
    bit_out[7] = rxadd;

    bit_out[8] = 0x01 & (payload_len >> 0);
    bit_out[9] = 0x01 & (payload_len >> 1);
    bit_out[10] = 0x01 & (payload_len >> 2);
    bit_out[11] = 0x01 & (payload_len >> 3);
    bit_out[12] = 0x01 & (payload_len >> 4);
    bit_out[13] = 0x01 & (payload_len >> 5);

    bit_out[14] = 0;
    bit_out[15] = 0;
}

int BTLETxProcessor::calculate_sample_for_ADV(PKT_INFO* pkt) {
    pkt->num_info_bit = 0;

    // gen preamble and access address
    const char* AA = "AA";
    const char* AAValue = "D6BE898E";
    pkt->num_info_bit = pkt->num_info_bit + convert_hex_to_bit((char*)AA, pkt->info_bit, 0, 1);
    pkt->num_info_bit = pkt->num_info_bit + convert_hex_to_bit((char*)AAValue, pkt->info_bit + pkt->num_info_bit, 0, 4);

    // get txadd and rxadd
    int txadd = 0, rxadd = 0;

    pkt->num_info_bit = pkt->num_info_bit + 16;  // 16 is header length

    // get AdvA and AdvData
    pkt->num_info_bit = pkt->num_info_bit + convert_hex_to_bit(macAddress, pkt->info_bit + pkt->num_info_bit, 1, 6);

    pkt->num_info_bit = pkt->num_info_bit + convert_hex_to_bit(advertisementData, pkt->info_bit + pkt->num_info_bit, 0, 31);

    int payload_len = (pkt->num_info_bit / 8) - 7;

    fill_adv_pdu_header(pkt, txadd, rxadd, payload_len, pkt->info_bit + 5 * 8);
    const char* checksumInit = "555555";
    crc24_and_scramble_to_gen_phy_bit((char*)checksumInit, pkt);

#ifdef new_way
    pkt->num_phy_sample = gen_sample_from_phy_bit(pkt->phy_bit, pkt->phy_sample, pkt->num_phy_bit);
#endif

    // disp_bit_in_hex(pkt->phy_sample, pkt->num_phy_sample);

    return (0);
}

int BTLETxProcessor::calculate_sample_from_pkt_type(PKT_INFO* pkt) {
    // Todo: Handle other Enum values.
    // if (packetType == ADV_IND);

    if (calculate_sample_for_ADV(pkt) == -1) {
        return (-1);
    }

    return (0);
}

int BTLETxProcessor::calculate_pkt_info(PKT_INFO* pkt) {
    if (calculate_sample_from_pkt_type(pkt) == -1) {
        return (-1);
    }

    return (0);
}

void BTLETxProcessor::execute(const buffer_c8_t& buffer) {
    int8_t re, im;

    // This is called at 4M/2048 = 1953Hz
    for (size_t i = 0; i < buffer.count; i++) {
        if (configured) {
            // This is going to loop through each sample bit and push it to the output buffer.
            if (sample_count > length) {
                configured = false;
                sample_count = 0;

                txprogress_message.done = true;
                shared_memory.application_queue.push(txprogress_message);
            } else {
                // Real and imaginary was already calculated in gen_sample_from_phy_bit.
                // It was processed from each data bit, run through a Gaussian Filter, and then ran through sin and cos table to get each IQ bit.
                re = (int8_t)packets.phy_sample[sample_count++];
                im = (int8_t)packets.phy_sample[sample_count++];

                buffer.p[i] = {re, im};

                if (progress_count >= progress_notice) {
                    progress_count = 0;
                    txprogress_message.progress++;
                    txprogress_message.done = false;
                    shared_memory.application_queue.push(txprogress_message);
                } else {
                    progress_count++;
                }
            }
        } else {
            re = 0;
            im = 0;

            buffer.p[i] = {re, im};
        }
    }
}

void BTLETxProcessor::on_message(const Message* const message) {
    if (message->id == Message::ID::BTLETxConfigure) {
        configure(*reinterpret_cast<const BTLETxConfigureMessage*>(message));
    }
}

void BTLETxProcessor::configure(const BTLETxConfigureMessage& message) {
    channel_number = message.channel_number;
    packetType = (PKT_TYPE)message.pduType;

    memcpy(macAddress, message.macAddress, sizeof(macAddress));
    memcpy(advertisementData, message.advertisementData, sizeof(advertisementData));

    packets.channel_number = channel_number;
    packets.pkt_type = packetType;

    // Calculates the samples based on the BLE packet data and generates IQ values into an array to be sent out.
    calculate_pkt_info(&packets);

#ifdef new_way
    // This is because each sample contains I and Q, but packet.num_phy_samples just returns the total samples.
    length = (uint32_t)(packets.num_phy_sample * 2);
#else
    length = (uint32_t)packets.num_phy_bit;
#endif

    // Starting at sample_count 0 since packets.num_phy_sample contains every sample needed to be sent out.
    sample_count = 0;
    progress_count = 0;
    progress_notice = 64;

    txprogress_message.progress = 0;
    txprogress_message.done = false;
    configured = true;
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<BTLETxProcessor>()};
    event_dispatcher.run();
    return 0;
}
