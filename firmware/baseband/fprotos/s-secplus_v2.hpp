
#ifndef __FPROTO_SECPLUSV2_H__
#define __FPROTO_SECPLUSV2_H__

#include "subghzdbase.hpp"

#define SECPLUS_V2_HEADER 0x3C0000000000
#define SECPLUS_V2_HEADER_MASK 0xFFFF3C0000000000
#define SECPLUS_V2_PACKET_1 0x000000000000
#define SECPLUS_V2_PACKET_2 0x010000000000
#define SECPLUS_V2_PACKET_MASK 0x30000000000

typedef enum : uint8_t {
    SecPlus_v2DecoderStepReset = 0,
    SecPlus_v2DecoderStepDecoderData,
} SecPlus_v2DecoderStep;

class FProtoSubGhzDSecPlusV2 : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDSecPlusV2() {
        sensorType = FPS_SECPLUSV2;
        te_short = 250;
        te_long = 500;
        te_delta = 110;
        min_count_bit_for_found = 62;
    }

    void feed(bool level, uint32_t duration) {
        ManchesterEvent event = ManchesterEventReset;
        switch (parser_step) {
            case SecPlus_v2DecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_long * 130) < te_delta * 100)) {
                    // Found header Security+ 2.0
                    parser_step = SecPlus_v2DecoderStepDecoderData;
                    decode_data = 0;
                    decode_count_bit = 0;
                    secplus_packet_1 = 0;
                    FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventReset, &manchester_saved_state, NULL);
                    FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventLongHigh, &manchester_saved_state, NULL);
                    FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventShortLow, &manchester_saved_state, NULL);
                }
                break;
            case SecPlus_v2DecoderStepDecoderData:
                if (!level) {
                    if (DURATION_DIFF(duration, te_short) < te_delta) {
                        event = ManchesterEventShortLow;
                    } else if (
                        DURATION_DIFF(duration, te_long) < te_delta) {
                        event = ManchesterEventLongLow;
                    } else if (
                        duration >= (te_long * 2UL + te_delta)) {
                        if (decode_count_bit == min_count_bit_for_found) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            if (subghz_protocol_secplus_v2_check_packet()) {
                                // controller too big
                                if (callback) callback(this);
                                parser_step = SecPlus_v2DecoderStepReset;
                            }
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventReset, &manchester_saved_state, NULL);
                        FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventLongHigh, &manchester_saved_state, NULL);
                        FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventShortLow, &manchester_saved_state, NULL);
                    } else {
                        parser_step = SecPlus_v2DecoderStepReset;
                    }
                } else {
                    if (DURATION_DIFF(duration, te_short) < te_delta) {
                        event = ManchesterEventShortHigh;
                    } else if (
                        DURATION_DIFF(duration, te_long) < te_delta) {
                        event = ManchesterEventLongHigh;
                    } else {
                        parser_step = SecPlus_v2DecoderStepReset;
                    }
                }
                if (event != ManchesterEventReset) {
                    bool bit;
                    bool data_ok = FProtoGeneral::manchester_advance(manchester_saved_state, event, &manchester_saved_state, &bit);

                    if (data_ok) {
                        decode_data = (decode_data << 1) | bit;
                        decode_count_bit++;
                    }
                }
                break;
        }
    }

   protected:
    uint64_t secplus_packet_1 = 0;
    ManchesterState manchester_saved_state = ManchesterStateMid0;

    bool subghz_protocol_secplus_v2_check_packet() {
        if ((decode_data & SECPLUS_V2_HEADER_MASK) == SECPLUS_V2_HEADER) {
            if ((decode_data & SECPLUS_V2_PACKET_MASK) == SECPLUS_V2_PACKET_1) {
                secplus_packet_1 = decode_data;
            } else if (
                ((decode_data & SECPLUS_V2_PACKET_MASK) == SECPLUS_V2_PACKET_2) &&
                (secplus_packet_1)) {
                return true;
            }
        }
        return false;
    }
};

#endif
