/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#ifndef __PROC_BLE_TX_H__
#define __PROC_BLE_TX_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"

class BTLETxProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;
    void configure(const BTLETxConfigureMessage& message);

   private:
    static constexpr int max_char {256};
    static constexpr int SAMPLE_PER_SYMBOL {4};
    static constexpr float AMPLITUDE {127.0};
    static constexpr float MOD_IDX {0.5};
    static constexpr int LEN_GAUSS_FILTER {4};
    static constexpr int MAX_NUM_INFO_BYTE {43};
    static constexpr int MAX_NUM_PHY_BYTE {47};
    static constexpr int MAX_NUM_PHY_SAMPLE {(MAX_NUM_PHY_BYTE*8*SAMPLE_PER_SYMBOL)+(LEN_GAUSS_FILTER*SAMPLE_PER_SYMBOL)};

    enum PKT_TYPE
    {
        INVALID_TYPE,
        RAW,
        DISCOVERY,
        IBEACON,
        ADV_IND,
        ADV_DIRECT_IND,
        ADV_NONCONN_IND,
        ADV_SCAN_IND,
        SCAN_REQ,
        SCAN_RSP,
        CONNECT_REQ,
        LL_DATA,
        LL_CONNECTION_UPDATE_REQ,
        LL_CHANNEL_MAP_REQ,
        LL_TERMINATE_IND,
        LL_ENC_REQ,
        LL_ENC_RSP,
        LL_START_ENC_REQ,
        LL_START_ENC_RSP,
        LL_UNKNOWN_RSP,
        LL_FEATURE_REQ,
        LL_FEATURE_RSP,
        LL_PAUSE_ENC_REQ,
        LL_PAUSE_ENC_RSP,
        LL_VERSION_IND,
        LL_REJECT_IND,
        NUM_PKT_TYPE
    };

    struct PKT_INFO
    {
        int channel_number;
        PKT_TYPE pkt_type;

        int num_info_bit;
        char info_bit[MAX_NUM_PHY_BYTE*8]; // without CRC and whitening

        int num_info_byte;
        uint8_t info_byte[MAX_NUM_PHY_BYTE];

        int num_phy_bit;
        char phy_bit[MAX_NUM_PHY_BYTE*8]; // all bits which will be fed to GFSK modulator

        int num_phy_byte;
        uint8_t phy_byte[MAX_NUM_PHY_BYTE];

        int num_phy_sample;
        char phy_sample[2*MAX_NUM_PHY_SAMPLE]; // GFSK output to D/A (hackrf board)
        int8_t phy_sample1[2*MAX_NUM_PHY_SAMPLE]; // GFSK output to D/A (hackrf board)

        int space; // how many millisecond null signal shouwl be padded after this packet
    };

    PKT_INFO packets {};

    int calculate_pkt_info( PKT_INFO *pkt );
    int calculate_sample_from_pkt_type(PKT_INFO *pkt);
    int calculate_sample_for_ADV_IND(PKT_INFO *pkt);
    void fill_adv_pdu_header(PKT_INFO *pkt, int txadd, int rxadd, int payload_len, char *bit_out);
    void crc24_and_scramble_to_gen_phy_bit(char *crc_init_hex, PKT_INFO *pkt);
    void disp_bit_in_hex(char *bit, int num_bit);
    void scramble(char *bit_in, int num_bit, int channel_number, char *bit_out);
    void crc24(char *bit_in, int num_bit, char *init_hex, char *crc_result);
    int convert_hex_to_bit(char *hex, char *bit, int stream_flip, int octet_limit);
    void octet_hex_to_bit(char *hex, char *bit);
    int gen_sample_from_phy_bit(char *bit, char *sample, int num_bit);
    bool configured = false;

    float tmp_phy_bit_over_sampling[MAX_NUM_PHY_SAMPLE + 2*LEN_GAUSS_FILTER*SAMPLE_PER_SYMBOL];
    float gauss_coef[LEN_GAUSS_FILTER*SAMPLE_PER_SYMBOL] = {7.561773e-09, 1.197935e-06, 8.050684e-05, 2.326833e-03, 2.959908e-02, 1.727474e-01, 4.999195e-01, 8.249246e-01, 9.408018e-01, 8.249246e-01, 4.999195e-01, 1.727474e-01, 2.959908e-02, 2.326833e-03, 8.050684e-05, 1.197935e-06};

    uint32_t samples_per_bit{4};
    uint32_t channel_number{37};

    uint32_t length{0};
    uint32_t shift_zero{}, shift_one{};
    uint32_t progress_notice{}, progress_count{0};
    uint32_t sample_count{0};
    uint32_t phase{0}, sphase{0};

    uint8_t cur_bit{0};
    uint16_t bit_pos{0};

    // clang-format off
    const int8_t gauss_coef_int8[16] = {
    0, 0, 0, 0, 2, 11, 32, 53, 60, 53, 32, 11, 2, 0, 0, 0, };

    const int8_t cos_table_int8[1024] = {
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 126, 126, 126, 126, 126, 126, 126, 126, 126, 
    126, 126, 125, 125, 125, 125, 125, 125, 125, 124, 124, 124, 124, 124, 124, 123, 123, 123, 123, 123, 122, 122, 122, 122, 
    122, 121, 121, 121, 121, 120, 120, 120, 120, 119, 119, 119, 118, 118, 118, 118, 117, 117, 117, 116, 116, 116, 115, 115, 
    115, 114, 114, 114, 113, 113, 113, 112, 112, 112, 111, 111, 111, 110, 110, 109, 109, 109, 108, 108, 107, 107, 106, 106, 
    106, 105, 105, 104, 104, 103, 103, 102, 102, 102, 101, 101, 100, 100, 99, 99, 98, 98, 97, 97, 96, 96, 95, 95, 
    94, 94, 93, 93, 92, 91, 91, 90, 90, 89, 89, 88, 88, 87, 86, 86, 85, 85, 84, 84, 83, 82, 82, 81, 
    81, 80, 79, 79, 78, 78, 77, 76, 76, 75, 74, 74, 73, 72, 72, 71, 71, 70, 69, 69, 68, 67, 67, 66, 
    65, 65, 64, 63, 63, 62, 61, 61, 60, 59, 58, 58, 57, 56, 56, 55, 54, 54, 53, 52, 51, 51, 50, 49, 
    49, 48, 47, 46, 46, 45, 44, 44, 43, 42, 41, 41, 40, 39, 38, 38, 37, 36, 35, 35, 34, 33, 32, 32, 
    31, 30, 29, 29, 28, 27, 26, 26, 25, 24, 23, 22, 22, 21, 20, 19, 19, 18, 17, 16, 16, 15, 14, 13, 
    12, 12, 11, 10, 9, 9, 8, 7, 6, 5, 5, 4, 3, 2, 2, 1, 0, -1, -2, -2, -3, -4, -5, -5, 
    -6, -7, -8, -9, -9, -10, -11, -12, -12, -13, -14, -15, -16, -16, -17, -18, -19, -19, -20, -21, -22, -22, -23, -24, 
    -25, -26, -26, -27, -28, -29, -29, -30, -31, -32, -32, -33, -34, -35, -35, -36, -37, -38, -38, -39, -40, -41, -41, -42, 
    -43, -44, -44, -45, -46, -46, -47, -48, -49, -49, -50, -51, -51, -52, -53, -54, -54, -55, -56, -56, -57, -58, -58, -59, 
    -60, -61, -61, -62, -63, -63, -64, -65, -65, -66, -67, -67, -68, -69, -69, -70, -71, -71, -72, -72, -73, -74, -74, -75, 
    -76, -76, -77, -78, -78, -79, -79, -80, -81, -81, -82, -82, -83, -84, -84, -85, -85, -86, -86, -87, -88, -88, -89, -89, 
    -90, -90, -91, -91, -92, -93, -93, -94, -94, -95, -95, -96, -96, -97, -97, -98, -98, -99, -99, -100, -100, -101, -101, -102, 
    -102, -102, -103, -103, -104, -104, -105, -105, -106, -106, -106, -107, -107, -108, -108, -109, -109, -109, -110, -110, -111, -111, -111, -112, 
    -112, -112, -113, -113, -113, -114, -114, -114, -115, -115, -115, -116, -116, -116, -117, -117, -117, -118, -118, -118, -118, -119, -119, -119, 
    -120, -120, -120, -120, -121, -121, -121, -121, -122, -122, -122, -122, -122, -123, -123, -123, -123, -123, -124, -124, -124, -124, -124, -124, 
    -125, -125, -125, -125, -125, -125, -125, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -127, -127, -127, -127, -127, -127, 
    -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -126, 
    -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -125, -125, -125, -125, -125, -125, -125, -124, -124, -124, -124, -124, -124, -123, 
    -123, -123, -123, -123, -122, -122, -122, -122, -122, -121, -121, -121, -121, -120, -120, -120, -120, -119, -119, -119, -118, -118, -118, -118, 
    -117, -117, -117, -116, -116, -116, -115, -115, -115, -114, -114, -114, -113, -113, -113, -112, -112, -112, -111, -111, -111, -110, -110, -109, 
    -109, -109, -108, -108, -107, -107, -106, -106, -106, -105, -105, -104, -104, -103, -103, -102, -102, -102, -101, -101, -100, -100, -99, -99, 
    -98, -98, -97, -97, -96, -96, -95, -95, -94, -94, -93, -93, -92, -91, -91, -90, -90, -89, -89, -88, -88, -87, -86, -86, 
    -85, -85, -84, -84, -83, -82, -82, -81, -81, -80, -79, -79, -78, -78, -77, -76, -76, -75, -74, -74, -73, -72, -72, -71, 
    -71, -70, -69, -69, -68, -67, -67, -66, -65, -65, -64, -63, -63, -62, -61, -61, -60, -59, -58, -58, -57, -56, -56, -55, 
    -54, -54, -53, -52, -51, -51, -50, -49, -49, -48, -47, -46, -46, -45, -44, -44, -43, -42, -41, -41, -40, -39, -38, -38, 
    -37, -36, -35, -35, -34, -33, -32, -32, -31, -30, -29, -29, -28, -27, -26, -26, -25, -24, -23, -22, -22, -21, -20, -19, 
    -19, -18, -17, -16, -16, -15, -14, -13, -12, -12, -11, -10, -9, -9, -8, -7, -6, -5, -5, -4, -3, -2, -2, -1, 
    0, 1, 2, 2, 3, 4, 5, 5, 6, 7, 8, 9, 9, 10, 11, 12, 12, 13, 14, 15, 16, 16, 17, 18, 
    19, 19, 20, 21, 22, 22, 23, 24, 25, 26, 26, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36, 
    37, 38, 38, 39, 40, 41, 41, 42, 43, 44, 44, 45, 46, 46, 47, 48, 49, 49, 50, 51, 51, 52, 53, 54, 
    54, 55, 56, 56, 57, 58, 58, 59, 60, 61, 61, 62, 63, 63, 64, 65, 65, 66, 67, 67, 68, 69, 69, 70, 
    71, 71, 72, 72, 73, 74, 74, 75, 76, 76, 77, 78, 78, 79, 79, 80, 81, 81, 82, 82, 83, 84, 84, 85, 
    85, 86, 86, 87, 88, 88, 89, 89, 90, 90, 91, 91, 92, 93, 93, 94, 94, 95, 95, 96, 96, 97, 97, 98, 
    98, 99, 99, 100, 100, 101, 101, 102, 102, 102, 103, 103, 104, 104, 105, 105, 106, 106, 106, 107, 107, 108, 108, 109, 
    109, 109, 110, 110, 111, 111, 111, 112, 112, 112, 113, 113, 113, 114, 114, 114, 115, 115, 115, 116, 116, 116, 117, 117, 
    117, 118, 118, 118, 118, 119, 119, 119, 120, 120, 120, 120, 121, 121, 121, 121, 122, 122, 122, 122, 122, 123, 123, 123, 
    123, 123, 124, 124, 124, 124, 124, 124, 125, 125, 125, 125, 125, 125, 125, 126, 126, 126, 126, 126, 126, 126, 126, 126, 
    126, 126, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, };

    const int8_t sin_table_int8[1024] = {
    0, 1, 2, 2, 3, 4, 5, 5, 6, 7, 8, 9, 9, 10, 11, 12, 12, 13, 14, 15, 16, 16, 17, 18, 
    19, 19, 20, 21, 22, 22, 23, 24, 25, 26, 26, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36, 
    37, 38, 38, 39, 40, 41, 41, 42, 43, 44, 44, 45, 46, 46, 47, 48, 49, 49, 50, 51, 51, 52, 53, 54, 
    54, 55, 56, 56, 57, 58, 58, 59, 60, 61, 61, 62, 63, 63, 64, 65, 65, 66, 67, 67, 68, 69, 69, 70, 
    71, 71, 72, 72, 73, 74, 74, 75, 76, 76, 77, 78, 78, 79, 79, 80, 81, 81, 82, 82, 83, 84, 84, 85, 
    85, 86, 86, 87, 88, 88, 89, 89, 90, 90, 91, 91, 92, 93, 93, 94, 94, 95, 95, 96, 96, 97, 97, 98, 
    98, 99, 99, 100, 100, 101, 101, 102, 102, 102, 103, 103, 104, 104, 105, 105, 106, 106, 106, 107, 107, 108, 108, 109, 
    109, 109, 110, 110, 111, 111, 111, 112, 112, 112, 113, 113, 113, 114, 114, 114, 115, 115, 115, 116, 116, 116, 117, 117, 
    117, 118, 118, 118, 118, 119, 119, 119, 120, 120, 120, 120, 121, 121, 121, 121, 122, 122, 122, 122, 122, 123, 123, 123, 
    123, 123, 124, 124, 124, 124, 124, 124, 125, 125, 125, 125, 125, 125, 125, 126, 126, 126, 126, 126, 126, 126, 126, 126, 
    126, 126, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 
    127, 127, 127, 127, 127, 127, 127, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 125, 125, 125, 125, 125, 125, 
    125, 124, 124, 124, 124, 124, 124, 123, 123, 123, 123, 123, 122, 122, 122, 122, 122, 121, 121, 121, 121, 120, 120, 120, 
    120, 119, 119, 119, 118, 118, 118, 118, 117, 117, 117, 116, 116, 116, 115, 115, 115, 114, 114, 114, 113, 113, 113, 112, 
    112, 112, 111, 111, 111, 110, 110, 109, 109, 109, 108, 108, 107, 107, 106, 106, 106, 105, 105, 104, 104, 103, 103, 102, 
    102, 102, 101, 101, 100, 100, 99, 99, 98, 98, 97, 97, 96, 96, 95, 95, 94, 94, 93, 93, 92, 91, 91, 90, 
    90, 89, 89, 88, 88, 87, 86, 86, 85, 85, 84, 84, 83, 82, 82, 81, 81, 80, 79, 79, 78, 78, 77, 76, 
    76, 75, 74, 74, 73, 72, 72, 71, 71, 70, 69, 69, 68, 67, 67, 66, 65, 65, 64, 63, 63, 62, 61, 61, 
    60, 59, 58, 58, 57, 56, 56, 55, 54, 54, 53, 52, 51, 51, 50, 49, 49, 48, 47, 46, 46, 45, 44, 44, 
    43, 42, 41, 41, 40, 39, 38, 38, 37, 36, 35, 35, 34, 33, 32, 32, 31, 30, 29, 29, 28, 27, 26, 26, 
    25, 24, 23, 22, 22, 21, 20, 19, 19, 18, 17, 16, 16, 15, 14, 13, 12, 12, 11, 10, 9, 9, 8, 7, 
    6, 5, 5, 4, 3, 2, 2, 1, 0, -1, -2, -2, -3, -4, -5, -5, -6, -7, -8, -9, -9, -10, -11, -12, 
    -12, -13, -14, -15, -16, -16, -17, -18, -19, -19, -20, -21, -22, -22, -23, -24, -25, -26, -26, -27, -28, -29, -29, -30, 
    -31, -32, -32, -33, -34, -35, -35, -36, -37, -38, -38, -39, -40, -41, -41, -42, -43, -44, -44, -45, -46, -46, -47, -48, 
    -49, -49, -50, -51, -51, -52, -53, -54, -54, -55, -56, -56, -57, -58, -58, -59, -60, -61, -61, -62, -63, -63, -64, -65, 
    -65, -66, -67, -67, -68, -69, -69, -70, -71, -71, -72, -72, -73, -74, -74, -75, -76, -76, -77, -78, -78, -79, -79, -80, 
    -81, -81, -82, -82, -83, -84, -84, -85, -85, -86, -86, -87, -88, -88, -89, -89, -90, -90, -91, -91, -92, -93, -93, -94, 
    -94, -95, -95, -96, -96, -97, -97, -98, -98, -99, -99, -100, -100, -101, -101, -102, -102, -102, -103, -103, -104, -104, -105, -105, 
    -106, -106, -106, -107, -107, -108, -108, -109, -109, -109, -110, -110, -111, -111, -111, -112, -112, -112, -113, -113, -113, -114, -114, -114, 
    -115, -115, -115, -116, -116, -116, -117, -117, -117, -118, -118, -118, -118, -119, -119, -119, -120, -120, -120, -120, -121, -121, -121, -121, 
    -122, -122, -122, -122, -122, -123, -123, -123, -123, -123, -124, -124, -124, -124, -124, -124, -125, -125, -125, -125, -125, -125, -125, -126, 
    -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, 
    -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -126, -126, -126, -126, -126, -126, -126, -126, -126, 
    -126, -126, -125, -125, -125, -125, -125, -125, -125, -124, -124, -124, -124, -124, -124, -123, -123, -123, -123, -123, -122, -122, -122, -122, 
    -122, -121, -121, -121, -121, -120, -120, -120, -120, -119, -119, -119, -118, -118, -118, -118, -117, -117, -117, -116, -116, -116, -115, -115, 
    -115, -114, -114, -114, -113, -113, -113, -112, -112, -112, -111, -111, -111, -110, -110, -109, -109, -109, -108, -108, -107, -107, -106, -106, 
    -106, -105, -105, -104, -104, -103, -103, -102, -102, -102, -101, -101, -100, -100, -99, -99, -98, -98, -97, -97, -96, -96, -95, -95, 
    -94, -94, -93, -93, -92, -91, -91, -90, -90, -89, -89, -88, -88, -87, -86, -86, -85, -85, -84, -84, -83, -82, -82, -81, 
    -81, -80, -79, -79, -78, -78, -77, -76, -76, -75, -74, -74, -73, -72, -72, -71, -71, -70, -69, -69, -68, -67, -67, -66, 
    -65, -65, -64, -63, -63, -62, -61, -61, -60, -59, -58, -58, -57, -56, -56, -55, -54, -54, -53, -52, -51, -51, -50, -49, 
    -49, -48, -47, -46, -46, -45, -44, -44, -43, -42, -41, -41, -40, -39, -38, -38, -37, -36, -35, -35, -34, -33, -32, -32, 
    -31, -30, -29, -29, -28, -27, -26, -26, -25, -24, -23, -22, -22, -21, -20, -19, -19, -18, -17, -16, -16, -15, -14, -13, 
    -12, -12, -11, -10, -9, -9, -8, -7, -6, -5, -5, -4, -3, -2, -2, -1, };
    // clang-format on
    
    PKT_TYPE packetType = ADV_IND;

    TXProgressMessage txprogress_message{};

    /* NB: Threads should be the last members in the class definition. */
    
    //BasebandThread baseband_thread{4000000, this, baseband::Direction::Transmit};
    //Rx for now because trying to test formulation of packet.
    AFSKDataMessage data_message{false, 0};
    BasebandThread baseband_thread{4000000, this, baseband::Direction::Transmit};
    
};

#endif
