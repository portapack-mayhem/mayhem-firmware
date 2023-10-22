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

// void BTLERxProcessor::demod_byte(int num_byte, uint8_t *out_byte)
//{
//    int i, j;
//    int I0, Q0, I1, Q1;
//    uint8_t bit_decision;
//    int sample_idx = 0;

// for (i = 0; i < num_byte; i++)
// {
//   out_byte[i] = 0;

// for (j = 0; j < 8; j++)
// {
//     I0 = dst_buffer.p[sample_idx].real();
//     Q0 = dst_buffer.p[sample_idx].imag();
//     I1 = dst_buffer.p[sample_idx + 1].real();
//     Q1 = dst_buffer.p[sample_idx + 1].imag();

// bit_decision = (I0 * Q1 - I1 * Q0) > 0 ? 1 : 0;

// out_byte[i] = out_byte[i] | (bit_decision << j);

// sample_idx += SAMPLE_PER_SYMBOL;;}
//}

int BTLERxProcessor::parse_adv_pdu_payload_byte(uint8_t* payload_byte, int num_payload_byte, ADV_PDU_TYPE pdu_type, void* adv_pdu_payload) {
    // Should at least have 6 bytes for the MAC Address.
    // Also ensuring that there is at least 1 byte of data.
    if (num_payload_byte <= 6) {
        // printf("Error: Payload Too Short (only %d bytes)!\n", num_payload_byte);
        return -1;
    }

    if (pdu_type == ADV_IND || pdu_type == ADV_NONCONN_IND || pdu_type == SCAN_RSP || pdu_type == ADV_SCAN_IND) {
        payload_type_0_2_4_6 = (ADV_PDU_PAYLOAD_TYPE_0_2_4_6*)adv_pdu_payload;

        macAddress[0] = payload_byte[5];
        macAddress[1] = payload_byte[4];
        macAddress[2] = payload_byte[3];
        macAddress[3] = payload_byte[2];
        macAddress[4] = payload_byte[1];
        macAddress[5] = payload_byte[0];

        memcpy(payload_type_0_2_4_6->Data, payload_byte + 6, num_payload_byte - 6);
    }
    // Only processing advertisments for right now.
    else {
        return -1;
    }
    // else if (pdu_type == ADV_DIRECT_IND || pdu_type == SCAN_REQ)
    // {
    //     if (num_payload_byte != 12)
    //     {
    //         //printf("Error: Payload length %d bytes. Need to be 12 for PDU Type %s!\n", num_payload_byte, ADV_PDU_TYPE_STR[pdu_type]);
    //         return(-1);
    //     }

    // payload_type_1_3 = (ADV_PDU_PAYLOAD_TYPE_1_3 *)adv_pdu_payload;

    // //AdvA = reorder_bytes_str( payload_bytes(1 : (2*6)) );
    // macAddress[0] = payload_byte[5];
    // macAddress[1] = payload_byte[4];
    // macAddress[2] = payload_byte[3];
    // macAddress[3] = payload_byte[2];
    // macAddress[4] = payload_byte[1];
    // macAddress[5] = payload_byte[0];

    // //InitA = reorder_bytes_str( payload_bytes((2*6+1):end) );
    // payload_type_1_3->A1[0] = payload_byte[11];
    // payload_type_1_3->A1[1] = payload_byte[10];
    // payload_type_1_3->A1[2] = payload_byte[9];
    // payload_type_1_3->A1[3] = payload_byte[8];
    // payload_type_1_3->A1[4] = payload_byte[7];
    // payload_type_1_3->A1[5] = payload_byte[6];

    // //payload_parse_result_str = ['AdvA:' AdvA ' InitA:' InitA];
    // }
    // else if (pdu_type == CONNECT_REQ)
    // {
    // if (num_payload_byte != 34)
    // {
    //     //printf("Error: Payload length %d bytes. Need to be 34 for PDU Type %s!\n", num_payload_byte, ADV_PDU_TYPE_STR[pdu_type]);
    //     return(-1);
    // }

    // payload_type_5 = (ADV_PDU_PAYLOAD_TYPE_5 *)adv_pdu_payload;

    // //InitA = reorder_bytes_str( payload_bytes(1 : (2*6)) );
    // macAddress[0] = payload_byte[5];
    // macAddress[1] = payload_byte[4];
    // macAddress[2] = payload_byte[3];
    // macAddress[3] = payload_byte[2];
    // macAddress[4] = payload_byte[1];
    // macAddress[5] = payload_byte[0];

    // //AdvA = reorder_bytes_str( payload_bytes((2*6+1):(2*6+2*6)) );
    // payload_type_5->AdvA[0] = payload_byte[11];
    // payload_type_5->AdvA[1] = payload_byte[10];
    // payload_type_5->AdvA[2] = payload_byte[9];
    // payload_type_5->AdvA[3] = payload_byte[8];
    // payload_type_5->AdvA[4] = payload_byte[7];
    // payload_type_5->AdvA[5] = payload_byte[6];

    // //AA = reorder_bytes_str( payload_bytes((2*6+2*6+1):(2*6+2*6+2*4)) );
    // payload_type_5->AA[0] = payload_byte[15];
    // payload_type_5->AA[1] = payload_byte[14];
    // payload_type_5->AA[2] = payload_byte[13];
    // payload_type_5->AA[3] = payload_byte[12];

    // //CRCInit = payload_bytes((2*6+2*6+2*4+1):(2*6+2*6+2*4+2*3));
    // payload_type_5->CRCInit = ( payload_byte[16] );
    // payload_type_5->CRCInit = ( (payload_type_5->CRCInit << 8) | payload_byte[17] );
    // payload_type_5->CRCInit = ( (payload_type_5->CRCInit << 8) | payload_byte[18] );

    // //WinSize = payload_bytes((2*6+2*6+2*4+2*3+1):(2*6+2*6+2*4+2*3+2*1));
    // payload_type_5->WinSize = payload_byte[19];

    // //WinOffset = reorder_bytes_str( payload_bytes((2*6+2*6+2*4+2*3+2*1+1):(2*6+2*6+2*4+2*3+2*1+2*2)) );
    // payload_type_5->WinOffset = ( payload_byte[21] );
    // payload_type_5->WinOffset = ( (payload_type_5->WinOffset << 8) | payload_byte[20] );

    // //Interval = reorder_bytes_str( payload_bytes((2*6+2*6+2*4+2*3+2*1+2*2+1):(2*6+2*6+2*4+2*3+2*1+2*2+2*2)) );
    // payload_type_5->Interval = ( payload_byte[23] );
    // payload_type_5->Interval = ( (payload_type_5->Interval << 8) | payload_byte[22] );

    // //Latency = reorder_bytes_str( payload_bytes((2*6+2*6+2*4+2*3+2*1+2*2+2*2+1):(2*6+2*6+2*4+2*3+2*1+2*2+2*2+2*2)) );
    // payload_type_5->Latency = ( payload_byte[25] );
    // payload_type_5->Latency = ( (payload_type_5->Latency << 8) | payload_byte[24] );

    // //Timeout = reorder_bytes_str( payload_bytes((2*6+2*6+2*4+2*3+2*1+2*2+2*2+2*2+1):(2*6+2*6+2*4+2*3+2*1+2*2+2*2+2*2+2*2)) );
    // payload_type_5->Timeout = ( payload_byte[27] );
    // payload_type_5->Timeout = ( (payload_type_5->Timeout << 8) | payload_byte[26] );

    // //ChM = reorder_bytes_str( payload_bytes((2*6+2*6+2*4+2*3+2*1+2*2+2*2+2*2+2*2+1):(2*6+2*6+2*4+2*3+2*1+2*2+2*2+2*2+2*2+2*5)) );
    // payload_type_5->ChM[0] = payload_byte[32];
    // payload_type_5->ChM[1] = payload_byte[31];
    // payload_type_5->ChM[2] = payload_byte[30];
    // payload_type_5->ChM[3] = payload_byte[29];
    // payload_type_5->ChM[4] = payload_byte[28];

    // //tmp_bits = payload_bits((end-7) : end);
    // //Hop = num2str( bi2de(tmp_bits(1:5), 'right-msb') );
    // //SCA = num2str( bi2de(tmp_bits(6:end), 'right-msb') );
    // payload_type_5->Hop = (payload_byte[33]&0x1F);
    // payload_type_5->SCA = ((payload_byte[33]>>5)&0x07);
    // }
    // else
    // {
    // //TODO: Handle Unknown PDU.
    //  payload_type_R = (ADV_PDU_PAYLOAD_TYPE_R *)adv_pdu_payload;
    //  memcpy(payload_type_R->payload_byte, payload_byte, num_payload_byte);
    // return(-1);
    // }

    return 0;
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
    const int32_t max_dB = mag2_to_dbv_norm(max_squared_f * (1.0f / (32768.0f * 32768.0f)));

    decim_0.execute(buffer, dst_buffer);
    feed_channel_stats(dst_buffer);

    const buffer_c8_t iq_buffer{
        buffer.p,
        buffer.count,
        baseband_fs};

    // process++;

    // if ((process % 50) != 0) return;

    // 4Mhz 2048 samples

    //--------------Variable Defines---------------------------------//

    int i, sp, j = 0;
    int I0, Q0, I1, Q1 = 0;
    int k, p, phase_idx = 0;
    int num_demod_byte = 0;

    bool unequal_flag;

    const int demod_buf_len = LEN_DEMOD_BUF_ACCESS;  // For AA
    int demod_buf_offset = 0;
    int num_symbol_left = dst_buffer.count / SAMPLE_PER_SYMBOL;  // One buffer sample consist of I and Q.
    int symbols_eaten = 0;
    int hit_idx = (-1);

    //--------------Start Parsing For Access Address---------------//

    static uint8_t demod_buf_access[SAMPLE_PER_SYMBOL][LEN_DEMOD_BUF_ACCESS];

    uint32_t uint32_tmp = DEFAULT_ACCESS_ADDR;
    uint8_t accessAddrBits[LEN_DEMOD_BUF_ACCESS];

    uint32_t accesssAddress = 0;

    // Filling up addressBits with the access address we are looking to find.
    for (i = 0; i < 32; i++) {
        accessAddrBits[i] = 0x01 & uint32_tmp;
        uint32_tmp = (uint32_tmp >> 1);
    }

    memset(demod_buf_access, 0, SAMPLE_PER_SYMBOL * demod_buf_len);

    for (i = 0; i < num_symbol_left * SAMPLE_PER_SYMBOL; i += SAMPLE_PER_SYMBOL) {
        sp = ((demod_buf_offset - demod_buf_len + 1) & (demod_buf_len - 1));

        for (j = 0; j < SAMPLE_PER_SYMBOL; j++) {
            // Sample and compare with the adjacent next sample.
            I0 = dst_buffer.p[i + j].real();
            Q0 = dst_buffer.p[i + j].imag();
            I1 = dst_buffer.p[i + j + 1].real();
            Q1 = dst_buffer.p[i + j + 1].imag();

            phase_idx = j;

            demod_buf_access[phase_idx][demod_buf_offset] = (I0 * Q1 - I1 * Q0) > 0 ? 1 : 0;

            k = sp;
            unequal_flag = false;

            accesssAddress = 0;

            for (p = 0; p < demod_buf_len; p++) {
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

    symbols_eaten += (8 * NUM_ACCESS_ADDR_BYTE * SAMPLE_PER_SYMBOL);  // move to beginning of PDU header

    num_symbol_left = num_symbol_left - symbols_eaten;

    //--------------Start PDU Header Parsing-----------------------//

    num_demod_byte = 2;  // PDU header has 2 octets

    symbols_eaten += 8 * num_demod_byte * SAMPLE_PER_SYMBOL;

    if (symbols_eaten > (int)dst_buffer.count) {
        return;
    }

    // //Demod the PDU Header
    uint8_t bit_decision;

    // Jump back down to beginning of PDU header.
    int sample_idx = symbols_eaten - (8 * num_demod_byte * SAMPLE_PER_SYMBOL);

    uint16_t packet_index = 0;

    for (i = 0; i < num_demod_byte; i++) {
        rb_buf[packet_index] = 0;

        for (j = 0; j < 8; j++) {
            I0 = dst_buffer.p[sample_idx].real();
            Q0 = dst_buffer.p[sample_idx].imag();
            I1 = dst_buffer.p[sample_idx + 1].real();
            Q1 = dst_buffer.p[sample_idx + 1].imag();

            bit_decision = (I0 * Q1 - I1 * Q0) > 0 ? 1 : 0;
            rb_buf[packet_index] = rb_buf[packet_index] | (bit_decision << j);

            sample_idx += SAMPLE_PER_SYMBOL;
            ;
        }

        packet_index++;
    }

    // demod_byte(num_demod_byte, rb_buf);

    scramble_byte(rb_buf, num_demod_byte, scramble_table[channel_number], rb_buf);

    uint8_t pdu_type = (ADV_PDU_TYPE)(rb_buf[0] & 0x0F);
    // uint8_t tx_add = ((rb_buf[0] & 0x40) != 0);
    // uint8_t rx_add = ((rb_buf[0] & 0x80) != 0);
    uint8_t payload_len = (rb_buf[1] & 0x3F);

    // Not valid Advertise Payload.
    if ((payload_len < 6) || (payload_len > 37)) {
        return;
    }

    //--------------Start Payload Parsing--------------------------//

    num_demod_byte = (payload_len + 3);
    symbols_eaten += 8 * num_demod_byte * SAMPLE_PER_SYMBOL;

    if (symbols_eaten > (int)dst_buffer.count) {
        return;
    }

    // sample_idx = symbols_eaten - (8 * num_demod_byte * SAMPLE_PER_SYMBOL);

    for (i = 0; i < num_demod_byte; i++) {
        rb_buf[packet_index] = 0;

        for (j = 0; j < 8; j++) {
            I0 = dst_buffer.p[sample_idx].real();
            Q0 = dst_buffer.p[sample_idx].imag();
            I1 = dst_buffer.p[sample_idx + 1].real();
            Q1 = dst_buffer.p[sample_idx + 1].imag();

            bit_decision = (I0 * Q1 - I1 * Q0) > 0 ? 1 : 0;
            rb_buf[packet_index] = rb_buf[packet_index] | (bit_decision << j);

            sample_idx += SAMPLE_PER_SYMBOL;
            ;
        }

        packet_index++;
    }

    // demod_byte(num_demod_byte, rb_buf + 2);

    scramble_byte(rb_buf + 2, num_demod_byte, scramble_table[channel_number] + 2, rb_buf + 2);

    //--------------Start CRC Checking-----------------------------//

    // Check CRC
    bool crc_flag = crc_check(rb_buf, payload_len + 2, crc_init_internal);
    // pkt_count++;

    // This should be the flag that determines if the data should be sent to the application layer.
    bool sendPacket = false;

    // Checking CRC and excluding Reserved PDU types.
    if (pdu_type < RESERVED0 && !crc_flag) {
        if (parse_adv_pdu_payload_byte(rb_buf + 2, payload_len, (ADV_PDU_TYPE)pdu_type, (void*)(&adv_pdu_payload)) == 0) {
            sendPacket = true;
        }

        // TODO: Make this a packet builder function?
        if (sendPacket) {
            blePacketData.max_dB = max_dB;

            blePacketData.type = pdu_type;
            blePacketData.size = payload_len;

            blePacketData.macAddress[0] = macAddress[0];
            blePacketData.macAddress[1] = macAddress[1];
            blePacketData.macAddress[2] = macAddress[2];
            blePacketData.macAddress[3] = macAddress[3];
            blePacketData.macAddress[4] = macAddress[4];
            blePacketData.macAddress[5] = macAddress[5];

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
}

void BTLERxProcessor::on_message(const Message* const message) {
    if (message->id == Message::ID::BTLERxConfigure)
        configure(*reinterpret_cast<const BTLERxConfigureMessage*>(message));
}

void BTLERxProcessor::configure(const BTLERxConfigureMessage& message) {
    channel_number = message.channel_number;
    decim_0.configure(taps_200k_wfm_decim_0.taps);
    demod.configure(48000, 5000);

    configured = true;

    crc_init_internal = crc_init_reorder(crc_initalVale);
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<BTLERxProcessor>()};
    event_dispatcher.run();
    return 0;
}
