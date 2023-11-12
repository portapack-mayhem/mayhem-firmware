/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2020 Shao
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

#include "proc_btlerx.hpp"
#include "portapack_shared_memory.hpp"

#include "event_m4.hpp"

uint32_t BTLERxProcessor::crc_init_reorder(uint32_t crc_init) {
    int i;
    uint32_t crc_init_tmp, crc_init_input, crc_init_input_tmp;

    crc_init_input_tmp = crc_init;
    crc_init_input = 0;

    crc_init_input = crc_init_input_tmp & 0xFF;

    crc_init_input_tmp = (crc_init_input_tmp >> 8);
    crc_init_input = ((crc_init_input << 8) | (crc_init_input_tmp & 0xFF));

    crc_init_input_tmp = (crc_init_input_tmp >> 8);
    crc_init_input = ((crc_init_input << 8) | (crc_init_input_tmp & 0xFF));

    crc_init_input = (crc_init_input << 1);
    crc_init_tmp = 0;

    for (i = 0; i < 24; i++) {
        crc_init_input = (crc_init_input >> 1);
        crc_init_tmp = ((crc_init_tmp << 1) | (crc_init_input & 0x01));
    }

    return (crc_init_tmp);
}

uint_fast32_t BTLERxProcessor::crc_update(uint_fast32_t crc, const void* data, size_t data_len) {
    const unsigned char* d = (const unsigned char*)data;
    unsigned int tbl_idx;

    while (data_len--) {
        tbl_idx = (crc ^ *d) & 0xff;
        crc = (crc_table[tbl_idx] ^ (crc >> 8)) & 0xffffff;

        d++;
    }

    return crc & 0xffffff;
}

uint_fast32_t BTLERxProcessor::crc24_byte(uint8_t* byte_in, int num_byte, uint32_t init_hex) {
    uint_fast32_t crc = init_hex;

    crc = crc_update(crc, byte_in, num_byte);

    return (crc);
}

bool BTLERxProcessor::crc_check(uint8_t* tmp_byte, int body_len, uint32_t crc_init) {
    int crc24_checksum;

    crc24_checksum = crc24_byte(tmp_byte, body_len, crc_init);  // 0x555555 --> 0xaaaaaa. maybe because byte order
    checksumReceived = 0;
    checksumReceived = ((checksumReceived << 8) | tmp_byte[body_len + 2]);
    checksumReceived = ((checksumReceived << 8) | tmp_byte[body_len + 1]);
    checksumReceived = ((checksumReceived << 8) | tmp_byte[body_len + 0]);

    return (crc24_checksum != checksumReceived);
}

void BTLERxProcessor::scramble_byte(uint8_t* byte_in, int num_byte, const uint8_t* scramble_table_byte, uint8_t* byte_out) {
    int i;

    for (i = 0; i < num_byte; i++) {
        byte_out[i] = byte_in[i] ^ scramble_table_byte[i];
    }
}

int BTLERxProcessor::verify_payload_byte(int num_payload_byte, ADV_PDU_TYPE pdu_type) {
    // Should at least have 6 bytes for the MAC Address.
    // Also ensuring that there is at least 1 byte of data.
    if (num_payload_byte <= 6) {
        // printf("Error: Payload Too Short (only %d bytes)!\n", num_payload_byte);
        return -1;
    }

    if (pdu_type == ADV_IND || pdu_type == ADV_NONCONN_IND || pdu_type == SCAN_RSP || pdu_type == ADV_SCAN_IND) {
        return 0;
    } else if (pdu_type == ADV_DIRECT_IND || pdu_type == SCAN_REQ) {
        if (num_payload_byte != 12) {
            // printf("Error: Payload length %d bytes. Need to be 12 for PDU Type %s!\n", num_payload_byte, ADV_PDU_TYPE_STR[pdu_type]);
            return -1;
        }
    } else if (pdu_type == CONNECT_REQ) {
        if (num_payload_byte != 34) {
            // printf("Error: Payload length %d bytes. Need to be 34 for PDU Type %s!\n", num_payload_byte, ADV_PDU_TYPE_STR[pdu_type]);
            return -1;
        }
    } else {
        return -1;
    }

    return 0;
}

void BTLERxProcessor::handleBeginState() {
    int num_symbol_left = dst_buffer.count / SAMPLE_PER_SYMBOL;  // One buffer sample consist of I and Q.

    static uint8_t demod_buf_access[SAMPLE_PER_SYMBOL][LEN_DEMOD_BUF_ACCESS];

    uint32_t uint32_tmp = DEFAULT_ACCESS_ADDR;
    uint8_t accessAddrBits[LEN_DEMOD_BUF_ACCESS];

    uint32_t accesssAddress = 0;

    // Filling up addressBits with the access address we are looking to find.
    for (int i = 0; i < 32; i++) {
        accessAddrBits[i] = 0x01 & uint32_tmp;
        uint32_tmp = (uint32_tmp >> 1);
    }

    const int demod_buf_len = LEN_DEMOD_BUF_ACCESS;  // For AA
    int demod_buf_offset = 0;
    int hit_idx = (-1);
    bool unequal_flag = false;

    memset(demod_buf_access, 0, SAMPLE_PER_SYMBOL * demod_buf_len);

    for (int i = 0; i < num_symbol_left * SAMPLE_PER_SYMBOL; i += SAMPLE_PER_SYMBOL) {
        int sp = ((demod_buf_offset - demod_buf_len + 1) & (demod_buf_len - 1));

        for (int j = 0; j < SAMPLE_PER_SYMBOL; j++) {
            // Sample and compare with the adjacent next sample.
            int I0 = dst_buffer.p[i + j].real();
            int Q0 = dst_buffer.p[i + j].imag();
            int I1 = dst_buffer.p[i + j + 1].real();
            int Q1 = dst_buffer.p[i + j + 1].imag();

            int phase_idx = j;

            demod_buf_access[phase_idx][demod_buf_offset] = (I0 * Q1 - I1 * Q0) > 0 ? 1 : 0;

            int k = sp;
            unequal_flag = false;

            accesssAddress = 0;

            for (int p = 0; p < demod_buf_len; p++) {
                if (demod_buf_access[phase_idx][k] != accessAddrBits[p]) {
                    unequal_flag = true;
                    hit_idx = (-1);
                    break;
                }

                accesssAddress = (accesssAddress & (~(1 << p))) | (demod_buf_access[phase_idx][k] << p);

                k = ((k + 1) & (demod_buf_len - 1));
            }

            if (unequal_flag == false) {
                hit_idx = (i + j - (demod_buf_len - 1) * SAMPLE_PER_SYMBOL);
                break;
            }
        }

        if (unequal_flag == false) {
            break;
        }

        demod_buf_offset = ((demod_buf_offset + 1) & (demod_buf_len - 1));
    }

    if (hit_idx == -1) {
        // Process more samples.
        return;
    }

    symbols_eaten += hit_idx;

    symbols_eaten += (8 * NUM_ACCESS_ADDR_BYTE * SAMPLE_PER_SYMBOL);  // move to the beginning of PDU header

    num_symbol_left = num_symbol_left - symbols_eaten;

    parseState = Parse_State_PDU_Header;
}

void BTLERxProcessor::handlePDUHeaderState() {
    int num_demod_byte = 2;  // PDU header has 2 octets

    symbols_eaten += 8 * num_demod_byte * SAMPLE_PER_SYMBOL;

    if (symbols_eaten > (int)dst_buffer.count) {
        return;
    }

    // Jump back down to the beginning of PDU header.
    sample_idx = symbols_eaten - (8 * num_demod_byte * SAMPLE_PER_SYMBOL);

    packet_index = 0;

    for (int i = 0; i < num_demod_byte; i++) {
        rb_buf[packet_index] = 0;

        for (int j = 0; j < 8; j++) {
            int I0 = dst_buffer.p[sample_idx].real();
            int Q0 = dst_buffer.p[sample_idx].imag();
            int I1 = dst_buffer.p[sample_idx + 1].real();
            int Q1 = dst_buffer.p[sample_idx + 1].imag();

            bit_decision = (I0 * Q1 - I1 * Q0) > 0 ? 1 : 0;
            rb_buf[packet_index] = rb_buf[packet_index] | (bit_decision << j);

            sample_idx += SAMPLE_PER_SYMBOL;
        }

        packet_index++;
    }

    scramble_byte(rb_buf, num_demod_byte, scramble_table[channel_number], rb_buf);

    pdu_type = (ADV_PDU_TYPE)(rb_buf[0] & 0x0F);
    // uint8_t tx_add = ((rb_buf[0] & 0x40) != 0);
    // uint8_t rx_add = ((rb_buf[0] & 0x80) != 0);
    payload_len = (rb_buf[1] & 0x3F);

    // Not a valid Advertise Payload.
    if ((payload_len < 6) || (payload_len > 37)) {
        parseState = Parse_State_Begin;
        return;
    } else {
        parseState = Parse_State_PDU_Payload;
    }
}

void BTLERxProcessor::handlePDUPayloadState() {
    int i;
    int num_demod_byte = (payload_len + 3);
    symbols_eaten += 8 * num_demod_byte * SAMPLE_PER_SYMBOL;

    if (symbols_eaten > (int)dst_buffer.count) {
        return;
    }

    for (i = 0; i < num_demod_byte; i++) {
        rb_buf[packet_index] = 0;

        for (int j = 0; j < 8; j++) {
            int I0 = dst_buffer.p[sample_idx].real();
            int Q0 = dst_buffer.p[sample_idx].imag();
            int I1 = dst_buffer.p[sample_idx + 1].real();
            int Q1 = dst_buffer.p[sample_idx + 1].imag();

            bit_decision = (I0 * Q1 - I1 * Q0) > 0 ? 1 : 0;
            rb_buf[packet_index] = rb_buf[packet_index] | (bit_decision << j);

            sample_idx += SAMPLE_PER_SYMBOL;
        }

        packet_index++;
    }

    scramble_byte(rb_buf + 2, num_demod_byte, scramble_table[channel_number] + 2, rb_buf + 2);

    // Check CRC
    bool crc_flag = crc_check(rb_buf, payload_len + 2, crc_init_internal);
    // pkt_count++;

    // This should be the flag that determines if the data should be sent to the application layer.
    bool sendPacket = false;

    // Checking CRC and excluding Reserved PDU types.
    if (pdu_type < RESERVED0 && !crc_flag) {
        if (verify_payload_byte(payload_len, (ADV_PDU_TYPE)pdu_type) == 0) {
            sendPacket = true;
        }

        // TODO: Make this a packet builder function?
        if (sendPacket) {
            blePacketData.max_dB = max_dB;

            blePacketData.type = pdu_type;
            blePacketData.size = payload_len;

            blePacketData.macAddress[0] = rb_buf[7];
            blePacketData.macAddress[1] = rb_buf[6];
            blePacketData.macAddress[2] = rb_buf[5];
            blePacketData.macAddress[3] = rb_buf[4];
            blePacketData.macAddress[4] = rb_buf[3];
            blePacketData.macAddress[5] = rb_buf[2];

            // Skip Header Byte and MAC Address
            uint8_t startIndex = 8;

            for (i = 0; i < payload_len - 6; i++) {
                blePacketData.data[i] = rb_buf[startIndex++];
            }

            blePacketData.dataLen = i;

            BLEPacketMessage data_message{&blePacketData};

            shared_memory.application_queue.push(data_message);
        }
    }

    parseState = Parse_State_Begin;
}

void BTLERxProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    // Pulled this implementation from channel_stats_collector.c to time slice a specific packet's dB.
    uint32_t max_squared = 0;

    void* src_p = buffer.p;

    while (src_p < &buffer.p[buffer.count]) {
        const uint32_t sample = *__SIMD32(src_p)++;
        const uint32_t mag_sq = __SMUAD(sample, sample);
        if (mag_sq > max_squared) {
            max_squared = mag_sq;
        }
    }

    const float max_squared_f = max_squared;
    max_dB = mag2_to_dbv_norm(max_squared_f * (1.0f / (32768.0f * 32768.0f)));

    // 4Mhz 2048 samples
    // Decimated by 4 to achieve 2048/4 = 512 samples at 1 sample per symbol.
    decim_0.execute(buffer, dst_buffer);
    feed_channel_stats(dst_buffer);

    symbols_eaten = 0;

    // Handle parsing based on parseState
    if (parseState == Parse_State_Begin) {
        handleBeginState();
    }

    if (parseState == Parse_State_PDU_Header) {
        handlePDUHeaderState();
    }

    if (parseState == Parse_State_PDU_Payload) {
        handlePDUPayloadState();
    }
}

void BTLERxProcessor::on_message(const Message* const message) {
    if (message->id == Message::ID::BTLERxConfigure)
        configure(*reinterpret_cast<const BTLERxConfigureMessage*>(message));
}

void BTLERxProcessor::configure(const BTLERxConfigureMessage& message) {
    channel_number = message.channel_number;
    decim_0.configure(taps_BTLE_1M_PHY_decim_0.taps);

    configured = true;

    crc_init_internal = crc_init_reorder(crc_initalVale);
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<BTLERxProcessor>()};
    event_dispatcher.run();
    return 0;
}
