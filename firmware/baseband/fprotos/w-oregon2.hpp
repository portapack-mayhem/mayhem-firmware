
#ifndef __FPROTO_OREGON2_H__
#define __FPROTO_OREGON2_H__

#include "weatherbase.hpp"

#define OREGON2_PREAMBLE_BITS 19
#define OREGON2_PREAMBLE_MASK 0b1111111111111111111
#define OREGON2_SENSOR_ID(d) (((d) >> 16) & 0xFFFF)
#define OREGON2_CHECKSUM_BITS 8

// 15 ones + 0101 (inverted A)
#define OREGON2_PREAMBLE 0b1111111111111110101

// bit indicating the low battery
#define OREGON2_FLAG_BAT_LOW 0x4

/// Documentation for Oregon Scientific protocols can be found here:
/// http://wmrx00.sourceforge.net/Arduino/OregonScientific-RF-Protocols.pdf
// Sensors ID
#define ID_THGR122N 0x1d20
#define ID_THGR968 0x1d30
#define ID_BTHR918 0x5d50
#define ID_BHTR968 0x5d60
#define ID_RGR968 0x2d10
#define ID_THR228N 0xec40
#define ID_THN132N 0xec40   // same as THR228N but different packet size
#define ID_RTGN318 0x0cc3   // warning: id is from 0x0cc3 and 0xfcc3
#define ID_RTGN129 0x0cc3   // same as RTGN318 but different packet size
#define ID_THGR810 0xf824   // This might be ID_THGR81, but what's true is lost in (git) history
#define ID_THGR810a 0xf8b4  // unconfirmed version
#define ID_THN802 0xc844
#define ID_PCR800 0x2914
#define ID_PCR800a 0x2d14  // Different PCR800 ID - AU version I think
#define ID_WGR800 0x1984
#define ID_WGR800a 0x1994  // unconfirmed version
#define ID_WGR968 0x3d00
#define ID_UV800 0xd874
#define ID_THN129 0xcc43   // THN129 Temp only
#define ID_RTHN129 0x0cd3  // RTHN129 Temp, clock sensors
#define ID_RTHN129_1 0x9cd3
#define ID_RTHN129_2 0xacd3
#define ID_RTHN129_3 0xbcd3
#define ID_RTHN129_4 0xccd3
#define ID_RTHN129_5 0xdcd3
#define ID_BTHGN129 0x5d53  // Baro, Temp, Hygro sensor
#define ID_UVR128 0xec70
#define ID_THGR328N 0xcc23    // Temp & Hygro sensor similar to THR228N with 5 channel instead of 3
#define ID_RTGR328N_1 0xdcc3  // RTGR328N_[1-5] RFclock(date &time)&Temp&Hygro sensor
#define ID_RTGR328N_2 0xccc3
#define ID_RTGR328N_3 0xbcc3
#define ID_RTGR328N_4 0xacc3
#define ID_RTGR328N_5 0x9cc3
#define ID_RTGR328N_6 0x8ce3  // RTGR328N_6&7 RFclock(date &time)&Temp&Hygro sensor like THGR328N
#define ID_RTGR328N_7 0x8ae3
typedef enum {
    Oregon2DecoderStepReset = 0,
    Oregon2DecoderStepFoundPreamble,
    Oregon2DecoderStepVarData,
} Oregon2DecoderStep;

class FProtoWeatherOregon2 : public FProtoWeatherBase {
   public:
    FProtoWeatherOregon2() {
        sensorType = FPW_OREGON2;
    }

    void feed(bool level, uint32_t duration) override {
        // oregon v2.1 signal is inverted
        ManchesterEvent event = level_and_duration_to_event(!level, duration);
        bool bit_value = false;

        // low-level bit sequence decoding
        if (event == ManchesterEventReset) {
            parser_step = Oregon2DecoderStepReset;
            have_bit = false;
            decode_data = 0UL;
            decode_count_bit = 0;
        }
        if (FProtoGeneral::manchester_advance(manchester_saved_state, event, &manchester_saved_state, &bit_value)) {
            if (have_bit) {
                if (!prev_bit && bit_value) {
                    subghz_protocol_blocks_add_bit(1);
                } else if (prev_bit && !bit_value) {
                    subghz_protocol_blocks_add_bit(0);
                } else {
                    ws_protocol_decoder_oregon2_reset();
                }
                have_bit = false;
            } else {
                prev_bit = bit_value;
                have_bit = true;
            }
        }

        switch (parser_step) {
            case Oregon2DecoderStepReset:
                // waiting for fixed oregon2 preamble
                if (decode_count_bit >= OREGON2_PREAMBLE_BITS &&
                    ((decode_data & OREGON2_PREAMBLE_MASK) == OREGON2_PREAMBLE)) {
                    parser_step = Oregon2DecoderStepFoundPreamble;
                    decode_count_bit = 0;
                    decode_data = 0UL;
                }
                break;
            case Oregon2DecoderStepFoundPreamble:
                // waiting for fixed oregon2 data
                if (decode_count_bit == 32) {
                    data = decode_data;
                    data_count_bit = decode_count_bit;
                    decode_data = 0UL;
                    decode_count_bit = 0;

                    // reverse nibbles in decoded data
                    data = (data & 0x55555555) << 1 |
                           (data & 0xAAAAAAAA) >> 1;
                    data = (data & 0x33333333) << 2 |
                           (data & 0xCCCCCCCC) >> 2;

                    ws_oregon2_decode_const_data();
                    var_bits = oregon2_sensor_id_var_bits(OREGON2_SENSOR_ID(data));

                    if (!var_bits) {
                        // sensor is not supported, stop decoding, but showing the decoded fixed part
                        parser_step = Oregon2DecoderStepReset;
                        if (callback) callback(this);
                    } else {
                        parser_step = Oregon2DecoderStepVarData;
                    }
                }
                break;
            case Oregon2DecoderStepVarData:
                // waiting for variable (sensor-specific data)
                if (decode_count_bit == (uint32_t)var_bits + OREGON2_CHECKSUM_BITS) {
                    var_data = decode_data & 0xFFFFFFFF;

                    // reverse nibbles in var data
                    var_data = (var_data & 0x55555555) << 1 |
                               (var_data & 0xAAAAAAAA) >> 1;
                    var_data = (var_data & 0x33333333) << 2 |
                               (var_data & 0xCCCCCCCC) >> 2;

                    ws_oregon2_decode_var_data(OREGON2_SENSOR_ID(data), var_data >> OREGON2_CHECKSUM_BITS);

                    parser_step = Oregon2DecoderStepReset;
                    if (callback) callback(this);
                }
                break;
        }
    }

   protected:
    // timing values
    uint32_t te_short = 500;
    uint32_t te_long = 1000;
    uint32_t te_delta = 200;
    uint32_t min_count_bit_for_found = 32;

    bool have_bit = false;
    bool prev_bit = 0;
    uint8_t var_bits{0};
    uint32_t var_data{0};
    ManchesterState manchester_saved_state = ManchesterStateMid1;
    void ws_protocol_decoder_oregon2_reset() {
        parser_step = Oregon2DecoderStepReset;
        decode_data = 0UL;
        decode_count_bit = 0;
        FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventReset, &manchester_saved_state, NULL);
        have_bit = false;
        var_data = 0;
        var_bits = 0;
    }

    ManchesterEvent level_and_duration_to_event(bool level, uint32_t duration) {
        bool is_long = false;
        if (DURATION_DIFF(duration, te_long) < te_delta) {
            is_long = true;
        } else if (DURATION_DIFF(duration, te_short) < te_delta) {
            is_long = false;
        } else {
            return ManchesterEventReset;
        }

        if (level)
            return is_long ? ManchesterEventLongHigh : ManchesterEventShortHigh;
        else
            return is_long ? ManchesterEventLongLow : ManchesterEventShortLow;
    }

    uint8_t oregon2_sensor_id_var_bits(uint16_t sensor_id) {
        switch (sensor_id) {
            case ID_THR228N:
            case ID_RTHN129_1:
            case ID_RTHN129_2:
            case ID_RTHN129_3:
            case ID_RTHN129_4:
            case ID_RTHN129_5:
                return 16;
            case ID_THGR122N:
                return 24;
            default:
                return 0;
        }
    }
    void ws_oregon2_decode_const_data() {
        id = OREGON2_SENSOR_ID(data);
        uint8_t ch_bits = (data >> 12) & 0xF;
        channel = 1;
        while (ch_bits > 1) {
            channel++;
            ch_bits >>= 1;
        }
        battery_low = (data & OREGON2_FLAG_BAT_LOW) ? 1 : 0;
    }
    uint16_t bcd_decode_short(uint32_t data) {
        return (data & 0xF) * 10 + ((data >> 4) & 0xF);
    }
    float ws_oregon2_decode_temp(uint32_t data) {
        int32_t temp_val;
        temp_val = bcd_decode_short(data >> 4);
        temp_val *= 10;
        temp_val += (data >> 12) & 0xF;
        if (data & 0xF) temp_val = -temp_val;
        return (float)temp_val / 10.0;
    }
    void ws_oregon2_decode_var_data(uint16_t sensor_id, uint32_t data) {
        switch (sensor_id) {
            case ID_THR228N:
            case ID_RTHN129_1:
            case ID_RTHN129_2:
            case ID_RTHN129_3:
            case ID_RTHN129_4:
            case ID_RTHN129_5:
                temp = ws_oregon2_decode_temp(data);
                humidity = WS_NO_HUMIDITY;
                return;
            case ID_THGR122N:
                humidity = bcd_decode_short(data);
                temp = ws_oregon2_decode_temp(data >> 8);
                return;
            default:
                break;
        }
    }
};

#endif
