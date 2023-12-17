
#ifndef __FPROTO_SECPLUSV1_H__
#define __FPROTO_SECPLUSV1_H__

#include "subghzdbase.hpp"
#include <string.h>

#define SECPLUS_V1_BIT_ERR -1  // 0b0000
#define SECPLUS_V1_BIT_0 0     // 0b0001
#define SECPLUS_V1_BIT_1 1     // 0b0011
#define SECPLUS_V1_BIT_2 2     // 0b0111

#define SECPLUS_V1_PACKET_1_HEADER 0x00
#define SECPLUS_V1_PACKET_2_HEADER 0x02
#define SECPLUS_V1_PACKET_1_INDEX_BASE 0
#define SECPLUS_V1_PACKET_2_INDEX_BASE 21
#define SECPLUS_V1_PACKET_1_ACCEPTED (1 << 0)
#define SECPLUS_V1_PACKET_2_ACCEPTED (1 << 1)

typedef enum : uint8_t {
    SecPlus_v1DecoderStepReset = 0,
    SecPlus_v1DecoderStepSearchStartBit,
    SecPlus_v1DecoderStepSaveDuration,
    SecPlus_v1DecoderStepDecoderData,
} SecPlus_v1DecoderStep;

class FProtoSubGhzDSecPlusV1 : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDSecPlusV1() {
        sensorType = FPS_SECPLUSV1;
        te_short = 500;
        te_long = 1500;
        te_delta = 100;
        min_count_bit_for_found = 21;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case SecPlus_v1DecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 120) < te_delta * 120)) {
                    // Found header Security+ 1.0
                    parser_step = SecPlus_v1DecoderStepSearchStartBit;
                    decode_data = 0;
                    decode_count_bit = 0;
                    packet_accepted = 0;
                    memset(data_array, 0, sizeof(data_array));
                }
                break;
            case SecPlus_v1DecoderStepSearchStartBit:
                if (level) {
                    if (DURATION_DIFF(duration, te_short) < te_delta) {
                        base_packet_index = SECPLUS_V1_PACKET_1_INDEX_BASE;
                        data_array[decode_count_bit + base_packet_index] = SECPLUS_V1_BIT_0;
                        decode_count_bit++;
                        parser_step = SecPlus_v1DecoderStepSaveDuration;
                    } else if (
                        DURATION_DIFF(duration, te_long) < te_delta) {
                        base_packet_index = SECPLUS_V1_PACKET_2_INDEX_BASE;
                        data_array[decode_count_bit + base_packet_index] = SECPLUS_V1_BIT_2;
                        decode_count_bit++;
                        parser_step = SecPlus_v1DecoderStepSaveDuration;
                    } else {
                        parser_step = SecPlus_v1DecoderStepReset;
                    }
                } else {
                    parser_step = SecPlus_v1DecoderStepReset;
                }
                break;
            case SecPlus_v1DecoderStepSaveDuration:
                if (!level) {  // save interval
                    if (DURATION_DIFF(duration, te_short * 120) < te_delta * 120) {
                        if (decode_count_bit == min_count_bit_for_found) {
                            if (base_packet_index == SECPLUS_V1_PACKET_1_INDEX_BASE)
                                packet_accepted |= SECPLUS_V1_PACKET_1_ACCEPTED;
                            if (base_packet_index == SECPLUS_V1_PACKET_2_INDEX_BASE)
                                packet_accepted |= SECPLUS_V1_PACKET_2_ACCEPTED;

                            if (packet_accepted == (SECPLUS_V1_PACKET_1_ACCEPTED | SECPLUS_V1_PACKET_2_ACCEPTED)) {
                                // subghz_protocol_secplus_v1_decode();  // disabled doe to lack of flash
                                //  controller
                                // uint32_t fixed = (data >> 32) & 0xFFFFFFFF;
                                // cnt = data & 0xFFFFFFFF;
                                // btn = fixed % 3;
                                if (callback) callback(this);
                                parser_step = SecPlus_v1DecoderStepReset;
                            }
                        }
                        parser_step = SecPlus_v1DecoderStepSearchStartBit;
                        decode_data = 0;
                        decode_count_bit = 0;
                    } else {
                        te_last = duration;
                        parser_step = SecPlus_v1DecoderStepDecoderData;
                    }
                } else {
                    parser_step = SecPlus_v1DecoderStepReset;
                }
                break;
            case SecPlus_v1DecoderStepDecoderData:
                if (level && (decode_count_bit <= min_count_bit_for_found)) {
                    if ((DURATION_DIFF(te_last, te_short * 3) < te_delta * 3) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        data_array[decode_count_bit + base_packet_index] = SECPLUS_V1_BIT_0;
                        decode_count_bit++;
                        parser_step = SecPlus_v1DecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_short * 2) < te_delta * 2) &&
                        (DURATION_DIFF(duration, te_short * 2) < te_delta * 2)) {
                        data_array[decode_count_bit + base_packet_index] = SECPLUS_V1_BIT_1;
                        decode_count_bit++;
                        parser_step = SecPlus_v1DecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_short * 3) < te_delta * 3)) {
                        data_array[decode_count_bit + base_packet_index] = SECPLUS_V1_BIT_2;
                        decode_count_bit++;
                        parser_step = SecPlus_v1DecoderStepSaveDuration;
                    } else {
                        parser_step = SecPlus_v1DecoderStepReset;
                    }
                } else {
                    parser_step = SecPlus_v1DecoderStepReset;
                }
                break;
        }
    }

   protected:
    uint8_t packet_accepted = 0;
    uint8_t base_packet_index = 0;
    uint8_t data_array[44];
};

#endif
