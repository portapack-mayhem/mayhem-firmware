
#ifndef __FPROTO_OREGON3_H__
#define __FPROTO_OREGON3_H__

#include "weatherbase.hpp"

#define OREGON3_PREAMBLE_BITS 28
#define OREGON3_PREAMBLE_MASK 0b1111111111111111111111111111
// 24 ones + 0101 (inverted A)
#define OREGON3_PREAMBLE 0b1111111111111111111111110101

// Fixed part contains:
// - Sensor type: 16 bits
// - Channel: 4 bits
// - ID (changes when batteries are changed): 8 bits
// - Battery status: 4 bits
#define OREGON3_FIXED_PART_BITS (16 + 4 + 8 + 4)
#define OREGON3_SENSOR_ID(d) (((d) >> 16) & 0xFFFF)
#define OREGON3_CHECKSUM_BITS 8

// bit indicating the low battery
#define OREGON3_FLAG_BAT_LOW 0x4

/// Documentation for Oregon Scientific protocols can be found here:
/// https://www.osengr.org/Articles/OS-RF-Protocols-IV.pdf
// Sensors ID
#define ID_THGR221 0xf824
typedef enum {
    Oregon3DecoderStepReset = 0,
    Oregon3DecoderStepFoundPreamble,
    Oregon3DecoderStepVarData,
} Oregon3DecoderStep;

class FProtoWeatherOregon3 : public FProtoWeatherBase {
   public:
    FProtoWeatherOregon3() {
        sensorType = FPW_OREGON3;
    }

    void feed(bool level, uint32_t duration) override {
        ManchesterEvent event = level_and_duration_to_event(!level, duration);

        // low-level bit sequence decoding
        if (event == ManchesterEventReset) {
            parser_step = Oregon3DecoderStepReset;
            prev_bit = false;
            decode_data = 0UL;
            decode_count_bit = 0;
        }
        if (FProtoGeneral::manchester_advance(
                manchester_saved_state, event, &manchester_saved_state, &prev_bit)) {
            subghz_protocol_blocks_add_bit(prev_bit);
        }

        switch (parser_step) {
            case Oregon3DecoderStepReset:
                // waiting for fixed oregon3 preamble
                if (decode_count_bit >= OREGON3_PREAMBLE_BITS &&
                    ((decode_data & OREGON3_PREAMBLE_MASK) == OREGON3_PREAMBLE)) {
                    parser_step = Oregon3DecoderStepFoundPreamble;
                    decode_count_bit = 0;
                    decode_data = 0UL;
                }
                break;
            case Oregon3DecoderStepFoundPreamble:
                // waiting for fixed oregon3 data
                if (decode_count_bit == OREGON3_FIXED_PART_BITS) {
                    data = decode_data;
                    data_count_bit = decode_count_bit;
                    decode_data = 0UL;
                    decode_count_bit = 0;

                    // reverse nibbles in decoded data as oregon v3.0 is LSB first
                    data = (data & 0x55555555) << 1 |
                           (data & 0xAAAAAAAA) >> 1;
                    data = (data & 0x33333333) << 2 |
                           (data & 0xCCCCCCCC) >> 2;

                    ws_oregon3_decode_const_data();
                    var_bits =
                        oregon3_sensor_id_var_bits(OREGON3_SENSOR_ID(data));

                    if (!var_bits) {
                        // sensor is not supported, stop decoding
                        parser_step = Oregon3DecoderStepReset;
                    } else {
                        parser_step = Oregon3DecoderStepVarData;
                    }
                }
                break;
            case Oregon3DecoderStepVarData:
                // waiting for variable (sensor-specific data)
                if (decode_count_bit == (uint32_t)var_bits + OREGON3_CHECKSUM_BITS) {
                    var_data = decode_data & 0xFFFFFFFFFFFFFFFF;

                    // reverse nibbles in var data
                    var_data = (var_data & 0x5555555555555555) << 1 |
                               (var_data & 0xAAAAAAAAAAAAAAAA) >> 1;
                    var_data = (var_data & 0x3333333333333333) << 2 |
                               (var_data & 0xCCCCCCCCCCCCCCCC) >> 2;

                    ws_oregon3_decode_var_data(OREGON3_SENSOR_ID(data), var_data >> OREGON3_CHECKSUM_BITS);
                    parser_step = Oregon3DecoderStepReset;
                    if (callback) callback(this);
                }
                break;
        }
    }

   protected:
    // timing values
    uint32_t te_short = 500;
    uint32_t te_long = 1100;
    uint32_t te_delta = 300;
    uint32_t min_count_bit_for_found = 32;

    bool prev_bit = false;
    uint8_t var_bits{0};
    uint64_t var_data{0};
    ManchesterState manchester_saved_state = ManchesterStateMid1;
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
    uint8_t oregon3_sensor_id_var_bits(uint16_t sensor_id) {
        switch (sensor_id) {
            case ID_THGR221:
                // nibbles: temp + hum + '0'
                return (4 + 2 + 1) * 4;
            default:
                return 0;
        }
    }
    void ws_oregon3_decode_const_data() {
        id = OREGON3_SENSOR_ID(data);
        channel = (data >> 12) & 0xF;
        battery_low = (data & OREGON3_FLAG_BAT_LOW) ? 1 : 0;
    }
    uint16_t ws_oregon3_bcd_decode_short(uint32_t data) {
        return (data & 0xF) * 10 + ((data >> 4) & 0xF);
    }
    float ws_oregon3_decode_temp(uint32_t data) {
        int32_t temp_val;
        temp_val = ws_oregon3_bcd_decode_short(data >> 4);
        temp_val *= 10;
        temp_val += (data >> 12) & 0xF;
        if (data & 0xF) temp_val = -temp_val;
        return (float)temp_val / 10.0;
    }
    void ws_oregon3_decode_var_data(uint16_t sensor_id, uint32_t data) {
        switch (sensor_id) {
            case ID_THGR221:
            default:
                humidity = ws_oregon3_bcd_decode_short(data >> 4);
                temp = ws_oregon3_decode_temp(data >> 12);
                break;
        }
    }
};

#endif
