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

inline float BTLERxProcessor::get_phase_diff(const complex16_t& sample0, const complex16_t& sample1) {
    // Calculate the phase difference between two samples.
    float dI = sample1.real() * sample0.real() + sample1.imag() * sample0.imag();
    float dQ = sample1.imag() * sample0.real() - sample1.real() * sample0.imag();
    float phase_diff = atan2f(dQ, dI);

    return phase_diff;
}

inline uint32_t BTLERxProcessor::crc_init_reorder(uint32_t crc_init) {
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

inline uint_fast32_t BTLERxProcessor::crc_update(uint_fast32_t crc, const void* data, size_t data_len) {
    const unsigned char* d = (const unsigned char*)data;
    unsigned int tbl_idx;

    while (data_len--) {
        tbl_idx = (crc ^ *d) & 0xff;
        crc = (crc_table[tbl_idx] ^ (crc >> 8)) & 0xffffff;

        d++;
    }

    return crc & 0xffffff;
}

inline uint_fast32_t BTLERxProcessor::crc24_byte(uint8_t* byte_in, int num_byte, uint32_t init_hex) {
    uint_fast32_t crc = init_hex;
    crc = crc_update(crc, byte_in, num_byte);
    return (crc);
}

inline bool BTLERxProcessor::crc_check(uint8_t* tmp_byte, int body_len, uint32_t crc_init) {
    int crc24_checksum = crc24_byte(tmp_byte, body_len, crc_init);  // 0x555555 --> 0xaaaaaa. maybe because byte order
    checksumReceived = 0;
    checksumReceived = ((checksumReceived << 8) | tmp_byte[body_len + 2]);
    checksumReceived = ((checksumReceived << 8) | tmp_byte[body_len + 1]);
    checksumReceived = ((checksumReceived << 8) | tmp_byte[body_len + 0]);

    return (crc24_checksum != checksumReceived);
}

inline void BTLERxProcessor::scramble_byte(uint8_t* byte_in, int num_byte, const uint8_t* scramble_table_byte, uint8_t* byte_out) {
    for (int i = 0; i < num_byte; i++) {
        byte_out[i] = byte_in[i] ^ scramble_table_byte[i];
    }
}

inline int BTLERxProcessor::verify_payload_byte(int num_payload_byte, ADV_PDU_TYPE pdu_type) {
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

inline void BTLERxProcessor::resetOffsetTracking() {
    frequency_offset = 0.0f;
    frequency_offset_estimate = 0.0f;
    phase_buffer_index = 0;
    memset(phase_buffer, 0, sizeof(phase_buffer));
}

inline void BTLERxProcessor::resetBitPacketIndex() {
    memset(rb_buf, 0, sizeof(rb_buf));
    packet_index = 0;
    bit_index = 0;
}

inline void BTLERxProcessor::resetToDefaultState() {
    parseState = Parse_State_Begin;
    resetOffsetTracking();
    resetBitPacketIndex();
}

inline void BTLERxProcessor::demodulateFSKBits(int num_demod_byte) {
    for (; packet_index < num_demod_byte; packet_index++) {
        for (; bit_index < 8; bit_index++) {
            if (samples_eaten >= (int)dst_buffer.count) {
                return;
            }

            float phaseSum = 0.0f;
            for (int k = 0; k < SAMPLE_PER_SYMBOL; ++k) {
                float phase = get_phase_diff(
                    dst_buffer.p[samples_eaten + k],
                    dst_buffer.p[samples_eaten + k + 1]);
                phaseSum += phase;
            }

            // phaseSum /= (SAMPLE_PER_SYMBOL);
            // phaseSum -= frequency_offset;

            bool bitDecision = (phaseSum > 0.0f);
            rb_buf[packet_index] = rb_buf[packet_index] | (bitDecision << bit_index);

            samples_eaten += SAMPLE_PER_SYMBOL;
        }

        bit_index = 0;
    }
}

inline void BTLERxProcessor::handleBeginState() {
    uint32_t validAccessAddress = DEFAULT_ACCESS_ADDR;
    uint32_t accesssAddress = 0;

    int hit_idx = (-1);

    for (int i = samples_eaten; i < (int)dst_buffer.count; i += SAMPLE_PER_SYMBOL) {
        float phaseDiff = 0;

        for (int j = 0; j < SAMPLE_PER_SYMBOL; j++) {
            phaseDiff += get_phase_diff(dst_buffer.p[i + j], dst_buffer.p[i + j + 1]);
        }

        phase_buffer[phase_buffer_index] = phaseDiff / (SAMPLE_PER_SYMBOL);
        phase_buffer_index = (phase_buffer_index + 1) % ROLLING_WINDOW;

        bool bitDecision = (phaseDiff > 0);

        accesssAddress = (accesssAddress >> 1 | (bitDecision << 31));

        int errors = __builtin_popcount(accesssAddress ^ validAccessAddress) & 0xFFFFFFFF;

        if (errors <= 4) {
            hit_idx = i + SAMPLE_PER_SYMBOL;

            for (int k = 0; k < ROLLING_WINDOW; k++) {
                frequency_offset_estimate += phase_buffer[k];
            }

            frequency_offset = frequency_offset_estimate / ROLLING_WINDOW;

            break;
        }
    }

    if (hit_idx == -1) {
        // Process more samples.
        samples_eaten = dst_buffer.count + 1;
        return;
    }

    samples_eaten += hit_idx;

    parseState = Parse_State_PDU_Header;
}

inline void BTLERxProcessor::handlePDUHeaderState() {
    if (samples_eaten > (int)dst_buffer.count) {
        return;
    }

    demodulateFSKBits(NUM_PDU_HEADER_BYTE);

    if (packet_index < NUM_PDU_HEADER_BYTE || bit_index != 0) {
        return;
    }

    scramble_byte(rb_buf, 2, scramble_table[channel_number], rb_buf);

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

inline void BTLERxProcessor::handlePDUPayloadState() {
    const int num_demod_byte = (payload_len + 3);

    if (samples_eaten > (int)dst_buffer.count) {
        return;
    }

    demodulateFSKBits(num_demod_byte + NUM_PDU_HEADER_BYTE);

    if (packet_index < (num_demod_byte + NUM_PDU_HEADER_BYTE) || bit_index != 0) {
        return;
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

            int i;

            for (i = 0; i < payload_len - 6; i++) {
                blePacketData.data[i] = rb_buf[startIndex++];
            }

            blePacketData.dataLen = i;

            BLEPacketMessage data_message{&blePacketData};

            shared_memory.application_queue.push(data_message);
        }
    }

    resetToDefaultState();
}

void BTLERxProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    max_dB = -128;

    auto* ptr = buffer.p;
    auto* end = &buffer.p[buffer.count];

    uint32_t maxMag = 0;
    int8_t maxMaxReal = 0;
    int8_t maxMaxImag = 0;
    while (ptr < end) {
        uint32_t mag = (uint32_t)(ptr->real() * ptr->real() + ptr->imag() * ptr->imag());
        if (mag > maxMag) {
            maxMag = mag;
            maxMaxReal = ptr->real();
            maxMaxImag = ptr->imag();
        }
        ptr++;
    }
    max_dB = mag2_to_dbm_8bit_normalized(maxMaxReal, maxMaxImag, 1.0f, 50.0f);

    // 4Mhz 2048 samples
    // Decimated by 4 to achieve 2048/4 = 512 samples at 1 sample per symbol.
    decim_0.execute(buffer, dst_buffer);
    feed_channel_stats(dst_buffer);

    samples_eaten = 0;

    while (samples_eaten < (int)dst_buffer.count) {
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
}

void BTLERxProcessor::on_message(const Message* const message) {
    if (message->id == Message::ID::BTLERxConfigure)
        configure(*reinterpret_cast<const BTLERxConfigureMessage*>(message));
}

void BTLERxProcessor::configure(const BTLERxConfigureMessage& message) {
    channel_number = message.channel_number;
    decim_0.configure(taps_BTLE_2M_PHY_decim_0.taps);

    configured = true;

    crc_init_internal = crc_init_reorder(crc_initalVale);
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<BTLERxProcessor>()};
    event_dispatcher.run();
    return 0;
}
