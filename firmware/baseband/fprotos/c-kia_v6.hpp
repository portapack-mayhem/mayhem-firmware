#pragma once
#include "subcarbase.hpp"
#include <cstring>

typedef enum {
    KiaV6DecoderStepReset = 0,
    KiaV6DecoderStepWaitFirstHigh,
    KiaV6DecoderStepCountPreamble,
    KiaV6DecoderStepWaitLongHigh,
    KiaV6DecoderStepData,
} KiaV6DecoderStep;

#define KIA_V6_XOR_MASK_LOW 0x84AF25FB
#define KIA_V6_XOR_MASK_HIGH 0x638766AB

class FProtoSubCarKiaV6 : public FProtoSubCarBase {
   public:
    FProtoSubCarKiaV6() {
        sensorType = FPC_KIAV6;
        te_short = 200;
        te_long = 400;
        te_delta = 100;
        min_count_bit_for_found = 144;
    }

    void feed(bool level, uint32_t duration) {
        uint32_t uVar4, uVar5;
        ManchesterEvent event;
        bool data_bit;
        uint8_t bit_count_inc;
        uint32_t step_value;

        switch (parser_step) {
            case KiaV6DecoderStepReset:  // case 0
                if (level == 0) {
                    return;
                }
                if (DURATION_DIFF(duration, te_short) <
                    te_delta) {
                    parser_step = KiaV6DecoderStepWaitFirstHigh;
                    te_last = duration;
                    header_count = 0;
                    FProtoGeneral::manchester_advance(
                        manchester_state,
                        ManchesterEventReset,
                        &manchester_state,
                        NULL);
                }
                return;

            case KiaV6DecoderStepWaitFirstHigh: {  // case 1
                if (level != 0) {
                    return;
                }
                uint32_t diff_short = DURATION_DIFF(duration, te_short);
                uint32_t diff_long = DURATION_DIFF(duration, te_long);

                uint32_t diff = (diff_long < diff_short) ? diff_long : diff_short;

                if (diff_long < te_delta && diff_long < diff_short) {
                    if (header_count >= 0x259) {  // 601 decimal
                        header_count = 0;
                        te_last = duration;
                        parser_step = KiaV6DecoderStepWaitLongHigh;
                        return;
                    }
                }

                if (diff >= te_delta) {
                    step_value = KiaV6DecoderStepReset;
                    goto LAB_reset;
                }

                if (DURATION_DIFF(te_last, te_short) <
                    te_delta) {
                    te_last = duration;
                    header_count++;
                    return;
                } else {
                    step_value = KiaV6DecoderStepReset;
                    goto LAB_reset;
                }
            }
            case KiaV6DecoderStepWaitLongHigh: {  // case 2
                if (level == 0) {
                    step_value = KiaV6DecoderStepReset;
                    goto LAB_reset;
                }
                uint32_t diff_long_check = DURATION_DIFF(duration, te_long);
                uint32_t diff_short_check = DURATION_DIFF(duration, te_short);

                if (diff_long_check >= te_delta) {
                    if (diff_short_check >= te_delta) {
                        step_value = KiaV6DecoderStepReset;
                        goto LAB_reset;
                    }
                }

                if (DURATION_DIFF(te_last, te_long) >=
                    te_delta) {
                    step_value = KiaV6DecoderStepReset;
                    goto LAB_reset;
                }
                decode_data = 0;
                decode_count_bit = 0;

                subghz_protocol_blocks_add_bit(1);
                subghz_protocol_blocks_add_bit(1);
                subghz_protocol_blocks_add_bit(0);
                subghz_protocol_blocks_add_bit(1);

                data_part1_low = (uint32_t)(decode_data & 0xFFFFFFFF);
                data_part1_high = (uint32_t)((decode_data >> 32) & 0xFFFFFFFF);
                bit_count = decode_count_bit;

                parser_step = KiaV6DecoderStepData;
                return;
            }
            case KiaV6DecoderStepData:  // case 3
                if (DURATION_DIFF(duration, te_short) <
                    te_delta) {
                    event = (ManchesterEvent)((level & 0x7F) << 1);
                    goto manchester_process;
                } else if (
                    DURATION_DIFF(duration, te_long) <
                    te_delta) {
                    event = (ManchesterEvent)(level ? 6 : 4);
                    goto manchester_process;
                }
                step_value = KiaV6DecoderStepReset;
                goto LAB_reset;

            manchester_process:
                if (FProtoGeneral::manchester_advance(
                        manchester_state, event, &manchester_state, &data_bit)) {
                    uVar4 = data_part1_low;
                    uVar5 = (uVar4 << 1) | (data_bit ? 1 : 0);

                    uint32_t carry = (uVar4 >> 31) & 1;
                    uVar4 = (data_part1_high << 1) | carry;

                    data_part1_low = uVar5;
                    data_part1_high = uVar4;

                    decode_data = ((uint64_t)uVar4 << 32) | uVar5;

                    bit_count_inc = bit_count + 1;
                    bit_count = bit_count_inc;

                    if (bit_count_inc == 0x40) {
                        // stored_part1_low = ~uVar5;
                        // stored_part1_high = ~uVar4;
                        data_part1_low = 0;
                        data_part1_high = 0;
                    } else if (bit_count_inc == 0x80) {
                        // stored_part2_low = ~uVar5;
                        // stored_part2_high = ~uVar4;
                        data_part1_low = 0;
                        data_part1_high = 0;
                    }
                }

                te_last = duration;

                if (bit_count != min_count_bit_for_found) {
                    return;
                }
                data_count_bit = min_count_bit_for_found;
                // data_part3 = ~((uint16_t)data_part1_low);

                // kia_v6_decrypt(); --won't
                decode_data = data_part1_low | ((uint64_t)data_part1_high << 32);
                if (callback) {
                    callback(this);
                }

                data_part1_low = 0;
                data_part1_high = 0;
                bit_count = 0;
                step_value = KiaV6DecoderStepReset;
                goto LAB_reset;

            default:
                return;
        }

    LAB_reset:
        parser_step = step_value;
        return;
    }

    uint8_t bit_count = 0;

    uint16_t header_count = 0;
    ManchesterState manchester_state = ManchesterStateMid1;
    uint32_t data_part1_low = 0;
    uint32_t data_part1_high = 0;

    // uint32_t stored_part1_low = 0;
    // uint32_t stored_part1_high = 0;
    // uint32_t stored_part2_low = 0;
    // uint32_t stored_part2_high = 0;
    // uint16_t data_part3 = 0;
};
