
#ifndef __FPROTO_SOMIFYKEYTIS_H__
#define __FPROTO_SOMIFYKEYTIS_H__

#include "subghzdbase.hpp"

typedef enum {
    SomfyKeytisDecoderStepReset = 0,
    SomfyKeytisDecoderStepCheckPreambula,
    SomfyKeytisDecoderStepFoundPreambula,
    SomfyKeytisDecoderStepStartDecode,
    SomfyKeytisDecoderStepDecoderData,
} SomfyKeytisDecoderStep;

class FProtoSubGhzDSomifyKeytis : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDSomifyKeytis() {
        sensorType = FPS_SOMIFY_KEYTIS;
        te_short = 640;
        te_long = 1280;
        te_delta = 250;
        min_count_bit_for_found = 80;
    }

    void feed(bool level, uint32_t duration) {
        ManchesterEvent event = ManchesterEventReset;
        switch (parser_step) {
            case SomfyKeytisDecoderStepReset:
                if ((level) && DURATION_DIFF(duration, te_short * 4) < te_delta * 4) {
                    parser_step = SomfyKeytisDecoderStepFoundPreambula;
                    header_count++;
                }
                break;
            case SomfyKeytisDecoderStepFoundPreambula:
                if ((!level) && (DURATION_DIFF(duration, te_short * 4) < te_delta * 4)) {
                    parser_step = SomfyKeytisDecoderStepCheckPreambula;
                } else {
                    header_count = 0;
                    parser_step = SomfyKeytisDecoderStepReset;
                }
                break;
            case SomfyKeytisDecoderStepCheckPreambula:
                if (level) {
                    if (DURATION_DIFF(duration, te_short * 4) < te_delta * 4) {
                        parser_step = SomfyKeytisDecoderStepFoundPreambula;
                        header_count++;
                    } else if (
                        (header_count > 1) && (DURATION_DIFF(duration, te_short * 7) < te_delta * 4)) {
                        parser_step = SomfyKeytisDecoderStepDecoderData;
                        decode_data = 0;
                        decode_count_bit = 0;
                        // press_duration_counter = 0;
                        FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventReset, &manchester_saved_state, NULL);
                        FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventLongHigh, &manchester_saved_state, NULL);
                    }
                }

                break;

            case SomfyKeytisDecoderStepDecoderData:
                if (!level) {
                    if (DURATION_DIFF(duration, te_short) < te_delta) {
                        event = ManchesterEventShortLow;
                    } else if (
                        DURATION_DIFF(duration, te_long) < te_delta) {
                        event = ManchesterEventLongLow;
                    } else if (
                        duration >= (te_long + te_delta)) {
                        if (decode_count_bit == min_count_bit_for_found) {
                            // check crc
                            uint64_t data_tmp = data ^ (data >> 8);
                            if (((data_tmp >> 40) & 0xF) == subghz_protocol_somfy_keytis_crc(data_tmp)) {
                                data = decode_data;
                                data_count_bit = decode_count_bit;

                                if (callback) callback(this);
                            }
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventReset, &manchester_saved_state, NULL);
                        FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventLongHigh, &manchester_saved_state, NULL);
                        parser_step = SomfyKeytisDecoderStepReset;
                    } else {
                        parser_step = SomfyKeytisDecoderStepReset;
                    }
                } else {
                    if (DURATION_DIFF(duration, te_short) <
                        te_delta) {
                        event = ManchesterEventShortHigh;
                    } else if (
                        DURATION_DIFF(duration, te_long) <
                        te_delta) {
                        event = ManchesterEventLongHigh;
                    } else {
                        parser_step = SomfyKeytisDecoderStepReset;
                    }
                }
                if (event != ManchesterEventReset) {
                    bool data;
                    bool data_ok = FProtoGeneral::manchester_advance(manchester_saved_state, event, &manchester_saved_state, &data);

                    if (data_ok) {
                        if (decode_count_bit < 56) {
                            decode_data = (decode_data << 1) | data;
                        } else {
                            // press_duration_counter = (press_duration_counter << 1) | data;
                        }

                        decode_count_bit++;
                    }
                }
                break;
        }
    }

   protected:
    ManchesterState manchester_saved_state = ManchesterStateMid1;
    uint8_t subghz_protocol_somfy_keytis_crc(uint64_t data) {
        uint8_t crc = 0;
        data &= 0xFFF0FFFFFFFFFF;
        for (uint8_t i = 0; i < 56; i += 8) {
            crc = crc ^ data >> i ^ (data >> (i + 4));
        }
        return crc & 0xf;
    }
};

#endif
