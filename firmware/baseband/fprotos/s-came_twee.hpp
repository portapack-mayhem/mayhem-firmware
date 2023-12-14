
#ifndef __FPROTO_CAMETWEE_H__
#define __FPROTO_CAMETWEE_H__

#include "subghzdbase.hpp"

typedef enum : uint8_t {
    CameTweeDecoderStepReset = 0,
    CameTweeDecoderStepDecoderData,
} CameTweeDecoderStep;

class FProtoSubGhzDCameTwee : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDCameTwee() {
        sensorType = FPS_CAMETWEE;
        te_short = 500;
        te_long = 1000;
        te_delta = 250;
        min_count_bit_for_found = 54;
    }

    void feed(bool level, uint32_t duration) {
        ManchesterEvent event = ManchesterEventReset;
        switch (parser_step) {
            case CameTweeDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_long * 51) < te_delta * 20)) {
                    // Found header CAME
                    parser_step = CameTweeDecoderStepDecoderData;
                    decode_data = 0;
                    decode_count_bit = 0;
                    FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventLongLow, &manchester_saved_state, NULL);
                    FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventLongHigh, &manchester_saved_state, NULL);
                    FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventShortLow, &manchester_saved_state, NULL);
                }
                break;
            case CameTweeDecoderStepDecoderData:
                if (!level) {
                    if (DURATION_DIFF(duration, te_short) < te_delta) {
                        event = ManchesterEventShortLow;
                    } else if (
                        DURATION_DIFF(duration, te_long) < te_delta) {
                        event = ManchesterEventLongLow;
                    } else if (
                        duration >= ((uint32_t)te_long * 2 + te_delta)) {
                        if (decode_count_bit == min_count_bit_for_found) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            subghz_protocol_came_twee_remote_controller();
                            if (callback) callback(this);
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventLongLow, &manchester_saved_state, NULL);
                        FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventLongHigh, &manchester_saved_state, NULL);
                        FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventShortLow, &manchester_saved_state, NULL);
                    } else {
                        parser_step = CameTweeDecoderStepReset;
                    }
                } else {
                    if (DURATION_DIFF(duration, te_short) < te_delta) {
                        event = ManchesterEventShortHigh;
                    } else if (
                        DURATION_DIFF(duration, te_long) < te_delta) {
                        event = ManchesterEventLongHigh;
                    } else {
                        parser_step = CameTweeDecoderStepReset;
                    }
                }
                if (event != ManchesterEventReset) {
                    bool bit;
                    if (FProtoGeneral::manchester_advance(manchester_saved_state, event, &manchester_saved_state, &bit)) {
                        decode_data = (decode_data << 1) | !bit;
                        decode_count_bit++;
                    }
                }
                break;
        }
    }

   protected:
    ManchesterState manchester_saved_state = ManchesterStateMid1;

    void subghz_protocol_came_twee_remote_controller() {
        /*      Came Twee 54 bit, rolling code 15 parcels with
         *       a decreasing counter from 0xE to 0x0
         *       with originally coded dip switches on the console 10 bit code
         *
         *  0x003FFF72E04A6FEE
         *  0x003FFF72D17B5EDD
         *  0x003FFF72C2684DCC
         *  0x003FFF72B3193CBB
         *  0x003FFF72A40E2BAA
         *  0x003FFF72953F1A99
         *  0x003FFF72862C0988
         *  0x003FFF7277DDF877
         *  0x003FFF7268C2E766
         *  0x003FFF7259F3D655
         *  0x003FFF724AE0C544
         *  0x003FFF723B91B433
         *  0x003FFF722C86A322
         *  0x003FFF721DB79211
         *  0x003FFF720EA48100
         *
         *   decryption
         * the last 32 bits, do XOR by the desired number, divide the result by 4,
         * convert the first 16 bits of the resulting 32-bit number to bin and do
         * bit-by-bit mirroring, adding up to 10 bits
         *
         * Example
         * Step 1. 0x003FFF721DB79211        => 0x1DB79211
         * Step 4. 0x1DB79211 xor 0x1D1D1D11 => 0x00AA8F00
         * Step 4. 0x00AA8F00 / 4            => 0x002AA3C0
         * Step 5. 0x002AA3C0                => 0x002A
         * Step 6. 0x002A    bin             => b101010
         * Step 7. b101010                   => b0101010000
         * Step 8. b0101010000               => (Dip) Off ON Off ON Off ON Off Off Off Off
         */

        uint8_t cnt_parcel = (uint8_t)(data & 0xF);
        serial = (uint32_t)(data & 0x0FFFFFFFF);
        data = (data ^ came_twee_magic_numbers_xor[cnt_parcel]);
        data /= 4;
        btn = (data >> 4) & 0x0F;
        data >>= 16;
        data = (uint16_t)FProtoGeneral::subghz_protocol_blocks_reverse_key(data, 16);
        cnt = data >> 6;
    }
    inline static const uint32_t came_twee_magic_numbers_xor[15] = {
        0x0E0E0E00,
        0x1D1D1D11,
        0x2C2C2C22,
        0x3B3B3B33,
        0x4A4A4A44,
        0x59595955,
        0x68686866,
        0x77777777,
        0x86868688,
        0x95959599,
        0xA4A4A4AA,
        0xB3B3B3BB,
        0xC2C2C2CC,
        0xD1D1D1DD,
        0xE0E0E0EE,
    };
};

#endif
