/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2020 Shao
 * Copyright (C) 2023 Netro
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

#ifndef __PROC_BTLERX_H__
#define __PROC_BTLERX_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"

#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"

#include "audio_output.hpp"

#include "fifo.hpp"
#include "message.hpp"

class BTLERxProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    static constexpr int SAMPLE_PER_SYMBOL {1};
    static constexpr int LEN_DEMOD_BUF_ACCESS {32};
    static constexpr uint32_t DEFAULT_ACCESS_ADDR {0x8E89BED6};
    static constexpr int NUM_ACCESS_ADDR_BYTE {4};

    enum ADV_PDU_TYPE {
        ADV_IND = 0,
        ADV_DIRECT_IND = 1,
        ADV_NONCONN_IND = 2,
        SCAN_REQ = 3,
        SCAN_RSP = 4,
        CONNECT_REQ = 5,
        ADV_SCAN_IND = 6,
        RESERVED0 = 7,
        RESERVED1 = 8,
        RESERVED2 = 9,
        RESERVED3 = 10,
        RESERVED4 = 11,
        RESERVED5 = 12,
        RESERVED6 = 13,
        RESERVED7 = 14,
        RESERVED8 = 15
    };

    uint8_t macAddress[6];
    int checksumReceived = 0;

    struct ADV_PDU_PAYLOAD_TYPE_0_2_4_6
    {
        uint8_t Data[31];
    };

    struct ADV_PDU_PAYLOAD_TYPE_1_3
    {
        uint8_t A1[6];
    };

    struct ADV_PDU_PAYLOAD_TYPE_5
    {
        uint8_t AdvA[6];
        uint8_t AA[4];
        uint32_t CRCInit;
        uint8_t WinSize;
        uint16_t WinOffset;
        uint16_t Interval;
        uint16_t Latency;
        uint16_t Timeout;
        uint8_t ChM[5];
        uint8_t Hop;
        uint8_t SCA;
    };

    struct ADV_PDU_PAYLOAD_TYPE_R
    {
        uint8_t payload_byte[40];
    };

    static constexpr size_t baseband_fs = 4000000;
    static constexpr size_t audio_fs = baseband_fs / 8 / 8 / 2;

    uint_fast32_t crc_update(uint_fast32_t crc, const void* data, size_t data_len);
    uint_fast32_t crc24_byte(uint8_t* byte_in, int num_byte, uint32_t init_hex);
    bool crc_check(uint8_t* tmp_byte, int body_len, uint32_t crc_init);
    uint32_t crc_init_reorder(uint32_t crc_init);

    uint32_t crc_initalVale = 0x555555;
    uint32_t crc_init_internal = 0x00;

    void scramble_byte(uint8_t* byte_in, int num_byte, const uint8_t* scramble_table_byte, uint8_t* byte_out);
    // void demod_byte(int num_byte, uint8_t *out_byte);
    int parse_adv_pdu_payload_byte(uint8_t* payload_byte, int num_payload_byte, ADV_PDU_TYPE pdu_type, void* adv_pdu_payload);

    std::array<complex16_t, 1024> dst{};
    const buffer_c16_t dst_buffer{
        dst.data(),
        dst.size()};

    static constexpr int RB_SIZE = 2048;
    uint8_t rb_buf[2048];

    dsp::decimate::FIRC8xR16x24FS4Decim4 decim_0{};
    dsp::decimate::FIRC16xR16x16Decim2 decim_1{};

    dsp::demodulate::FM demod{};
    int rb_head{-1};
    int32_t g_threshold{0};
    uint8_t channel_number{37};

    uint16_t process = 0;

    bool configured{false};
    BlePacketData blePacketData{};

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};

    void configure(const BTLERxConfigureMessage& message);

    ADV_PDU_PAYLOAD_TYPE_0_2_4_6* payload_type_0_2_4_6 = nullptr;
    ADV_PDU_PAYLOAD_TYPE_1_3* payload_type_1_3 = nullptr;
    ADV_PDU_PAYLOAD_TYPE_5* payload_type_5 = nullptr;
    ADV_PDU_PAYLOAD_TYPE_R* payload_type_R = nullptr;
    ADV_PDU_PAYLOAD_TYPE_R adv_pdu_payload = {0};

    // Scramble table definition
    const uint8_t scramble_table[40][42] = {
        {
            64,
            178,
            188,
            195,
            31,
            55,
            74,
            95,
            133,
            246,
            156,
            154,
            193,
            214,
            197,
            68,
            32,
            89,
            222,
            225,
            143,
            27,
            165,
            175,
            66,
            123,
            78,
            205,
            96,
            235,
            98,
            34,
            144,
            44,
            239,
            240,
            199,
            141,
            210,
            87,
            161,
            61,
        },
        {
            137,
            64,
            178,
            188,
            195,
            31,
            55,
            74,
            95,
            133,
            246,
            156,
            154,
            193,
            214,
            197,
            68,
            32,
            89,
            222,
            225,
            143,
            27,
            165,
            175,
            66,
            123,
            78,
            205,
            96,
            235,
            98,
            34,
            144,
            44,
            239,
            240,
            199,
            141,
            210,
            87,
            161,
        },
        {
            210,
            87,
            161,
            61,
            167,
            102,
            176,
            117,
            49,
            17,
            72,
            150,
            119,
            248,
            227,
            70,
            233,
            171,
            208,
            158,
            83,
            51,
            216,
            186,
            152,
            8,
            36,
            203,
            59,
            252,
            113,
            163,
            244,
            85,
            104,
            207,
            169,
            25,
            108,
            93,
            76,
            4,
        },
        {
            27,
            165,
            175,
            66,
            123,
            78,
            205,
            96,
            235,
            98,
            34,
            144,
            44,
            239,
            240,
            199,
            141,
            210,
            87,
            161,
            61,
            167,
            102,
            176,
            117,
            49,
            17,
            72,
            150,
            119,
            248,
            227,
            70,
            233,
            171,
            208,
            158,
            83,
            51,
            216,
            186,
            152,
        },
        {
            100,
            121,
            135,
            63,
            110,
            148,
            190,
            10,
            237,
            57,
            53,
            131,
            173,
            139,
            137,
            64,
            178,
            188,
            195,
            31,
            55,
            74,
            95,
            133,
            246,
            156,
            154,
            193,
            214,
            197,
            68,
            32,
            89,
            222,
            225,
            143,
            27,
            165,
            175,
            66,
            123,
            78,
        },
        {
            173,
            139,
            137,
            64,
            178,
            188,
            195,
            31,
            55,
            74,
            95,
            133,
            246,
            156,
            154,
            193,
            214,
            197,
            68,
            32,
            89,
            222,
            225,
            143,
            27,
            165,
            175,
            66,
            123,
            78,
            205,
            96,
            235,
            98,
            34,
            144,
            44,
            239,
            240,
            199,
            141,
            210,
        },
        {
            246,
            156,
            154,
            193,
            214,
            197,
            68,
            32,
            89,
            222,
            225,
            143,
            27,
            165,
            175,
            66,
            123,
            78,
            205,
            96,
            235,
            98,
            34,
            144,
            44,
            239,
            240,
            199,
            141,
            210,
            87,
            161,
            61,
            167,
            102,
            176,
            117,
            49,
            17,
            72,
            150,
            119,
        },
        {
            63,
            110,
            148,
            190,
            10,
            237,
            57,
            53,
            131,
            173,
            139,
            137,
            64,
            178,
            188,
            195,
            31,
            55,
            74,
            95,
            133,
            246,
            156,
            154,
            193,
            214,
            197,
            68,
            32,
            89,
            222,
            225,
            143,
            27,
            165,
            175,
            66,
            123,
            78,
            205,
            96,
            235,
        },
        {
            8,
            36,
            203,
            59,
            252,
            113,
            163,
            244,
            85,
            104,
            207,
            169,
            25,
            108,
            93,
            76,
            4,
            146,
            229,
            29,
            254,
            184,
            81,
            250,
            42,
            180,
            231,
            212,
            12,
            182,
            46,
            38,
            2,
            201,
            242,
            14,
            127,
            220,
            40,
            125,
            21,
            218,
        },
        {
            193,
            214,
            197,
            68,
            32,
            89,
            222,
            225,
            143,
            27,
            165,
            175,
            66,
            123,
            78,
            205,
            96,
            235,
            98,
            34,
            144,
            44,
            239,
            240,
            199,
            141,
            210,
            87,
            161,
            61,
            167,
            102,
            176,
            117,
            49,
            17,
            72,
            150,
            119,
            248,
            227,
            70,
        },
        {
            154,
            193,
            214,
            197,
            68,
            32,
            89,
            222,
            225,
            143,
            27,
            165,
            175,
            66,
            123,
            78,
            205,
            96,
            235,
            98,
            34,
            144,
            44,
            239,
            240,
            199,
            141,
            210,
            87,
            161,
            61,
            167,
            102,
            176,
            117,
            49,
            17,
            72,
            150,
            119,
            248,
            227,
        },
        {
            83,
            51,
            216,
            186,
            152,
            8,
            36,
            203,
            59,
            252,
            113,
            163,
            244,
            85,
            104,
            207,
            169,
            25,
            108,
            93,
            76,
            4,
            146,
            229,
            29,
            254,
            184,
            81,
            250,
            42,
            180,
            231,
            212,
            12,
            182,
            46,
            38,
            2,
            201,
            242,
            14,
            127,
        },
        {
            44,
            239,
            240,
            199,
            141,
            210,
            87,
            161,
            61,
            167,
            102,
            176,
            117,
            49,
            17,
            72,
            150,
            119,
            248,
            227,
            70,
            233,
            171,
            208,
            158,
            83,
            51,
            216,
            186,
            152,
            8,
            36,
            203,
            59,
            252,
            113,
            163,
            244,
            85,
            104,
            207,
            169,
        },
        {
            229,
            29,
            254,
            184,
            81,
            250,
            42,
            180,
            231,
            212,
            12,
            182,
            46,
            38,
            2,
            201,
            242,
            14,
            127,
            220,
            40,
            125,
            21,
            218,
            115,
            106,
            6,
            91,
            23,
            19,
            129,
            100,
            121,
            135,
            63,
            110,
            148,
            190,
            10,
            237,
            57,
            53,
        },
        {
            190,
            10,
            237,
            57,
            53,
            131,
            173,
            139,
            137,
            64,
            178,
            188,
            195,
            31,
            55,
            74,
            95,
            133,
            246,
            156,
            154,
            193,
            214,
            197,
            68,
            32,
            89,
            222,
            225,
            143,
            27,
            165,
            175,
            66,
            123,
            78,
            205,
            96,
            235,
            98,
            34,
            144,
        },
        {
            119,
            248,
            227,
            70,
            233,
            171,
            208,
            158,
            83,
            51,
            216,
            186,
            152,
            8,
            36,
            203,
            59,
            252,
            113,
            163,
            244,
            85,
            104,
            207,
            169,
            25,
            108,
            93,
            76,
            4,
            146,
            229,
            29,
            254,
            184,
            81,
            250,
            42,
            180,
            231,
            212,
            12,
        },
        {
            208,
            158,
            83,
            51,
            216,
            186,
            152,
            8,
            36,
            203,
            59,
            252,
            113,
            163,
            244,
            85,
            104,
            207,
            169,
            25,
            108,
            93,
            76,
            4,
            146,
            229,
            29,
            254,
            184,
            81,
            250,
            42,
            180,
            231,
            212,
            12,
            182,
            46,
            38,
            2,
            201,
            242,
        },
        {
            25,
            108,
            93,
            76,
            4,
            146,
            229,
            29,
            254,
            184,
            81,
            250,
            42,
            180,
            231,
            212,
            12,
            182,
            46,
            38,
            2,
            201,
            242,
            14,
            127,
            220,
            40,
            125,
            21,
            218,
            115,
            106,
            6,
            91,
            23,
            19,
            129,
            100,
            121,
            135,
            63,
            110,
        },
        {
            66,
            123,
            78,
            205,
            96,
            235,
            98,
            34,
            144,
            44,
            239,
            240,
            199,
            141,
            210,
            87,
            161,
            61,
            167,
            102,
            176,
            117,
            49,
            17,
            72,
            150,
            119,
            248,
            227,
            70,
            233,
            171,
            208,
            158,
            83,
            51,
            216,
            186,
            152,
            8,
            36,
            203,
        },
        {
            139,
            137,
            64,
            178,
            188,
            195,
            31,
            55,
            74,
            95,
            133,
            246,
            156,
            154,
            193,
            214,
            197,
            68,
            32,
            89,
            222,
            225,
            143,
            27,
            165,
            175,
            66,
            123,
            78,
            205,
            96,
            235,
            98,
            34,
            144,
            44,
            239,
            240,
            199,
            141,
            210,
            87,
        },
        {
            244,
            85,
            104,
            207,
            169,
            25,
            108,
            93,
            76,
            4,
            146,
            229,
            29,
            254,
            184,
            81,
            250,
            42,
            180,
            231,
            212,
            12,
            182,
            46,
            38,
            2,
            201,
            242,
            14,
            127,
            220,
            40,
            125,
            21,
            218,
            115,
            106,
            6,
            91,
            23,
            19,
            129,
        },
        {
            61,
            167,
            102,
            176,
            117,
            49,
            17,
            72,
            150,
            119,
            248,
            227,
            70,
            233,
            171,
            208,
            158,
            83,
            51,
            216,
            186,
            152,
            8,
            36,
            203,
            59,
            252,
            113,
            163,
            244,
            85,
            104,
            207,
            169,
            25,
            108,
            93,
            76,
            4,
            146,
            229,
            29,
        },
        {
            102,
            176,
            117,
            49,
            17,
            72,
            150,
            119,
            248,
            227,
            70,
            233,
            171,
            208,
            158,
            83,
            51,
            216,
            186,
            152,
            8,
            36,
            203,
            59,
            252,
            113,
            163,
            244,
            85,
            104,
            207,
            169,
            25,
            108,
            93,
            76,
            4,
            146,
            229,
            29,
            254,
            184,
        },
        {
            175,
            66,
            123,
            78,
            205,
            96,
            235,
            98,
            34,
            144,
            44,
            239,
            240,
            199,
            141,
            210,
            87,
            161,
            61,
            167,
            102,
            176,
            117,
            49,
            17,
            72,
            150,
            119,
            248,
            227,
            70,
            233,
            171,
            208,
            158,
            83,
            51,
            216,
            186,
            152,
            8,
            36,
        },
        {
            152,
            8,
            36,
            203,
            59,
            252,
            113,
            163,
            244,
            85,
            104,
            207,
            169,
            25,
            108,
            93,
            76,
            4,
            146,
            229,
            29,
            254,
            184,
            81,
            250,
            42,
            180,
            231,
            212,
            12,
            182,
            46,
            38,
            2,
            201,
            242,
            14,
            127,
            220,
            40,
            125,
            21,
        },
        {
            81,
            250,
            42,
            180,
            231,
            212,
            12,
            182,
            46,
            38,
            2,
            201,
            242,
            14,
            127,
            220,
            40,
            125,
            21,
            218,
            115,
            106,
            6,
            91,
            23,
            19,
            129,
            100,
            121,
            135,
            63,
            110,
            148,
            190,
            10,
            237,
            57,
            53,
            131,
            173,
            139,
            137,
        },
        {
            10,
            237,
            57,
            53,
            131,
            173,
            139,
            137,
            64,
            178,
            188,
            195,
            31,
            55,
            74,
            95,
            133,
            246,
            156,
            154,
            193,
            214,
            197,
            68,
            32,
            89,
            222,
            225,
            143,
            27,
            165,
            175,
            66,
            123,
            78,
            205,
            96,
            235,
            98,
            34,
            144,
            44,
        },
        {
            195,
            31,
            55,
            74,
            95,
            133,
            246,
            156,
            154,
            193,
            214,
            197,
            68,
            32,
            89,
            222,
            225,
            143,
            27,
            165,
            175,
            66,
            123,
            78,
            205,
            96,
            235,
            98,
            34,
            144,
            44,
            239,
            240,
            199,
            141,
            210,
            87,
            161,
            61,
            167,
            102,
            176,
        },
        {
            188,
            195,
            31,
            55,
            74,
            95,
            133,
            246,
            156,
            154,
            193,
            214,
            197,
            68,
            32,
            89,
            222,
            225,
            143,
            27,
            165,
            175,
            66,
            123,
            78,
            205,
            96,
            235,
            98,
            34,
            144,
            44,
            239,
            240,
            199,
            141,
            210,
            87,
            161,
            61,
            167,
            102,
        },
        {
            117,
            49,
            17,
            72,
            150,
            119,
            248,
            227,
            70,
            233,
            171,
            208,
            158,
            83,
            51,
            216,
            186,
            152,
            8,
            36,
            203,
            59,
            252,
            113,
            163,
            244,
            85,
            104,
            207,
            169,
            25,
            108,
            93,
            76,
            4,
            146,
            229,
            29,
            254,
            184,
            81,
            250,
        },
        {
            46,
            38,
            2,
            201,
            242,
            14,
            127,
            220,
            40,
            125,
            21,
            218,
            115,
            106,
            6,
            91,
            23,
            19,
            129,
            100,
            121,
            135,
            63,
            110,
            148,
            190,
            10,
            237,
            57,
            53,
            131,
            173,
            139,
            137,
            64,
            178,
            188,
            195,
            31,
            55,
            74,
            95,
        },
        {
            231,
            212,
            12,
            182,
            46,
            38,
            2,
            201,
            242,
            14,
            127,
            220,
            40,
            125,
            21,
            218,
            115,
            106,
            6,
            91,
            23,
            19,
            129,
            100,
            121,
            135,
            63,
            110,
            148,
            190,
            10,
            237,
            57,
            53,
            131,
            173,
            139,
            137,
            64,
            178,
            188,
            195,
        },
        {
            96,
            235,
            98,
            34,
            144,
            44,
            239,
            240,
            199,
            141,
            210,
            87,
            161,
            61,
            167,
            102,
            176,
            117,
            49,
            17,
            72,
            150,
            119,
            248,
            227,
            70,
            233,
            171,
            208,
            158,
            83,
            51,
            216,
            186,
            152,
            8,
            36,
            203,
            59,
            252,
            113,
            163,
        },
        {
            169,
            25,
            108,
            93,
            76,
            4,
            146,
            229,
            29,
            254,
            184,
            81,
            250,
            42,
            180,
            231,
            212,
            12,
            182,
            46,
            38,
            2,
            201,
            242,
            14,
            127,
            220,
            40,
            125,
            21,
            218,
            115,
            106,
            6,
            91,
            23,
            19,
            129,
            100,
            121,
            135,
            63,
        },
        {
            242,
            14,
            127,
            220,
            40,
            125,
            21,
            218,
            115,
            106,
            6,
            91,
            23,
            19,
            129,
            100,
            121,
            135,
            63,
            110,
            148,
            190,
            10,
            237,
            57,
            53,
            131,
            173,
            139,
            137,
            64,
            178,
            188,
            195,
            31,
            55,
            74,
            95,
            133,
            246,
            156,
            154,
        },
        {
            59,
            252,
            113,
            163,
            244,
            85,
            104,
            207,
            169,
            25,
            108,
            93,
            76,
            4,
            146,
            229,
            29,
            254,
            184,
            81,
            250,
            42,
            180,
            231,
            212,
            12,
            182,
            46,
            38,
            2,
            201,
            242,
            14,
            127,
            220,
            40,
            125,
            21,
            218,
            115,
            106,
            6,
        },
        {
            68,
            32,
            89,
            222,
            225,
            143,
            27,
            165,
            175,
            66,
            123,
            78,
            205,
            96,
            235,
            98,
            34,
            144,
            44,
            239,
            240,
            199,
            141,
            210,
            87,
            161,
            61,
            167,
            102,
            176,
            117,
            49,
            17,
            72,
            150,
            119,
            248,
            227,
            70,
            233,
            171,
            208,
        },
        {
            141,
            210,
            87,
            161,
            61,
            167,
            102,
            176,
            117,
            49,
            17,
            72,
            150,
            119,
            248,
            227,
            70,
            233,
            171,
            208,
            158,
            83,
            51,
            216,
            186,
            152,
            8,
            36,
            203,
            59,
            252,
            113,
            163,
            244,
            85,
            104,
            207,
            169,
            25,
            108,
            93,
            76,
        },
        {
            214,
            197,
            68,
            32,
            89,
            222,
            225,
            143,
            27,
            165,
            175,
            66,
            123,
            78,
            205,
            96,
            235,
            98,
            34,
            144,
            44,
            239,
            240,
            199,
            141,
            210,
            87,
            161,
            61,
            167,
            102,
            176,
            117,
            49,
            17,
            72,
            150,
            119,
            248,
            227,
            70,
            233,
        },
        {
            31,
            55,
            74,
            95,
            133,
            246,
            156,
            154,
            193,
            214,
            197,
            68,
            32,
            89,
            222,
            225,
            143,
            27,
            165,
            175,
            66,
            123,
            78,
            205,
            96,
            235,
            98,
            34,
            144,
            44,
            239,
            240,
            199,
            141,
            210,
            87,
            161,
            61,
            167,
            102,
            176,
            117,
        },
    };

    /**
     * Static table used for the table_driven implementation.
     *****************************************************************************/
    const uint_fast32_t crc_table[256] =
        {
            0x000000, 0x01b4c0, 0x036980, 0x02dd40, 0x06d300, 0x0767c0, 0x05ba80, 0x040e40,
            0x0da600, 0x0c12c0, 0x0ecf80, 0x0f7b40, 0x0b7500, 0x0ac1c0, 0x081c80, 0x09a840,
            0x1b4c00, 0x1af8c0, 0x182580, 0x199140, 0x1d9f00, 0x1c2bc0, 0x1ef680, 0x1f4240,
            0x16ea00, 0x175ec0, 0x158380, 0x143740, 0x103900, 0x118dc0, 0x135080, 0x12e440,
            0x369800, 0x372cc0, 0x35f180, 0x344540, 0x304b00, 0x31ffc0, 0x332280, 0x329640,
            0x3b3e00, 0x3a8ac0, 0x385780, 0x39e340, 0x3ded00, 0x3c59c0, 0x3e8480, 0x3f3040,
            0x2dd400, 0x2c60c0, 0x2ebd80, 0x2f0940, 0x2b0700, 0x2ab3c0, 0x286e80, 0x29da40,
            0x207200, 0x21c6c0, 0x231b80, 0x22af40, 0x26a100, 0x2715c0, 0x25c880, 0x247c40,
            0x6d3000, 0x6c84c0, 0x6e5980, 0x6fed40, 0x6be300, 0x6a57c0, 0x688a80, 0x693e40,
            0x609600, 0x6122c0, 0x63ff80, 0x624b40, 0x664500, 0x67f1c0, 0x652c80, 0x649840,
            0x767c00, 0x77c8c0, 0x751580, 0x74a140, 0x70af00, 0x711bc0, 0x73c680, 0x727240,
            0x7bda00, 0x7a6ec0, 0x78b380, 0x790740, 0x7d0900, 0x7cbdc0, 0x7e6080, 0x7fd440,
            0x5ba800, 0x5a1cc0, 0x58c180, 0x597540, 0x5d7b00, 0x5ccfc0, 0x5e1280, 0x5fa640,
            0x560e00, 0x57bac0, 0x556780, 0x54d340, 0x50dd00, 0x5169c0, 0x53b480, 0x520040,
            0x40e400, 0x4150c0, 0x438d80, 0x423940, 0x463700, 0x4783c0, 0x455e80, 0x44ea40,
            0x4d4200, 0x4cf6c0, 0x4e2b80, 0x4f9f40, 0x4b9100, 0x4a25c0, 0x48f880, 0x494c40,
            0xda6000, 0xdbd4c0, 0xd90980, 0xd8bd40, 0xdcb300, 0xdd07c0, 0xdfda80, 0xde6e40,
            0xd7c600, 0xd672c0, 0xd4af80, 0xd51b40, 0xd11500, 0xd0a1c0, 0xd27c80, 0xd3c840,
            0xc12c00, 0xc098c0, 0xc24580, 0xc3f140, 0xc7ff00, 0xc64bc0, 0xc49680, 0xc52240,
            0xcc8a00, 0xcd3ec0, 0xcfe380, 0xce5740, 0xca5900, 0xcbedc0, 0xc93080, 0xc88440,
            0xecf800, 0xed4cc0, 0xef9180, 0xee2540, 0xea2b00, 0xeb9fc0, 0xe94280, 0xe8f640,
            0xe15e00, 0xe0eac0, 0xe23780, 0xe38340, 0xe78d00, 0xe639c0, 0xe4e480, 0xe55040,
            0xf7b400, 0xf600c0, 0xf4dd80, 0xf56940, 0xf16700, 0xf0d3c0, 0xf20e80, 0xf3ba40,
            0xfa1200, 0xfba6c0, 0xf97b80, 0xf8cf40, 0xfcc100, 0xfd75c0, 0xffa880, 0xfe1c40,
            0xb75000, 0xb6e4c0, 0xb43980, 0xb58d40, 0xb18300, 0xb037c0, 0xb2ea80, 0xb35e40,
            0xbaf600, 0xbb42c0, 0xb99f80, 0xb82b40, 0xbc2500, 0xbd91c0, 0xbf4c80, 0xbef840,
            0xac1c00, 0xada8c0, 0xaf7580, 0xaec140, 0xaacf00, 0xab7bc0, 0xa9a680, 0xa81240,
            0xa1ba00, 0xa00ec0, 0xa2d380, 0xa36740, 0xa76900, 0xa6ddc0, 0xa40080, 0xa5b440,
            0x81c800, 0x807cc0, 0x82a180, 0x831540, 0x871b00, 0x86afc0, 0x847280, 0x85c640,
            0x8c6e00, 0x8ddac0, 0x8f0780, 0x8eb340, 0x8abd00, 0x8b09c0, 0x89d480, 0x886040,
            0x9a8400, 0x9b30c0, 0x99ed80, 0x985940, 0x9c5700, 0x9de3c0, 0x9f3e80, 0x9e8a40,
            0x972200, 0x9696c0, 0x944b80, 0x95ff40, 0x91f100, 0x9045c0, 0x929880, 0x932c40};
};

#endif /*__PROC_BTLERX_H__*/
