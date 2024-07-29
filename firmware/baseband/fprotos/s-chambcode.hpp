
#ifndef __FPROTO_CHAMBCODE_H__
#define __FPROTO_CHAMBCODE_H__

#include "subghzdbase.hpp"

#define CHAMBERLAIN_CODE_BIT_STOP 0b0001
#define CHAMBERLAIN_CODE_BIT_1 0b0011
#define CHAMBERLAIN_CODE_BIT_0 0b0111

#define CHAMBERLAIN_7_CODE_MASK 0xF000000FF0F
#define CHAMBERLAIN_8_CODE_MASK 0xF00000F00F
#define CHAMBERLAIN_9_CODE_MASK 0xF000000000F

#define CHAMBERLAIN_7_CODE_MASK_CHECK 0x10000001101
#define CHAMBERLAIN_8_CODE_MASK_CHECK 0x1000001001
#define CHAMBERLAIN_9_CODE_MASK_CHECK 0x10000000001

typedef enum : uint8_t {
    Chamb_CodeDecoderStepReset = 0,
    Chamb_CodeDecoderStepFoundStartBit,
    Chamb_CodeDecoderStepSaveDuration,
    Chamb_CodeDecoderStepCheckDuration,
} Chamb_CodeDecoderStep;

class FProtoSubGhzDChambCode : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDChambCode() {
        sensorType = FPS_CHAMBCODE;
        te_short = 1000;
        te_long = 3000;
        te_delta = 200;
        min_count_bit_for_found = 10;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case Chamb_CodeDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 39) < te_delta * 20)) {
                    // Found header Chamb_Code
                    parser_step = Chamb_CodeDecoderStepFoundStartBit;
                }
                break;
            case Chamb_CodeDecoderStepFoundStartBit:
                if ((level) && (DURATION_DIFF(duration, te_short) < te_delta)) {
                    // Found start bit Chamb_Code
                    decode_data = 0;
                    decode_count_bit = 0;
                    decode_data = decode_data << 4 | CHAMBERLAIN_CODE_BIT_STOP;
                    decode_count_bit++;
                    parser_step = Chamb_CodeDecoderStepSaveDuration;
                } else {
                    parser_step = Chamb_CodeDecoderStepReset;
                }
                break;
            case Chamb_CodeDecoderStepSaveDuration:
                if (!level) {  // save interval
                    if (duration > te_short * 5) {
                        if (decode_count_bit >= min_count_bit_for_found) {
                            if (subghz_protocol_decoder_chamb_code_check_mask_and_parse()) {
                                data = decode_data;
                                data_count_bit = decode_count_bit;
                                if (callback) callback(this);
                            }
                        }
                        parser_step = Chamb_CodeDecoderStepReset;
                    } else {
                        te_last = duration;
                        parser_step = Chamb_CodeDecoderStepCheckDuration;
                    }
                } else {
                    parser_step = Chamb_CodeDecoderStepReset;
                }
                break;
            case Chamb_CodeDecoderStepCheckDuration:
                if (level) {  // Found stop bit Chamb_Code
                    if ((DURATION_DIFF(te_last, te_short * 3) <
                         te_delta) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        decode_data = decode_data << 4 | CHAMBERLAIN_CODE_BIT_STOP;
                        decode_count_bit++;
                        parser_step = Chamb_CodeDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_short * 2) < te_delta) &&
                        (DURATION_DIFF(duration, te_short * 2) < te_delta)) {
                        decode_data = decode_data << 4 | CHAMBERLAIN_CODE_BIT_1;
                        decode_count_bit++;
                        parser_step = Chamb_CodeDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_short * 3) < te_delta)) {
                        decode_data = decode_data << 4 | CHAMBERLAIN_CODE_BIT_0;
                        decode_count_bit++;
                        parser_step = Chamb_CodeDecoderStepSaveDuration;
                    } else {
                        parser_step = Chamb_CodeDecoderStepReset;
                    }

                } else {
                    parser_step = Chamb_CodeDecoderStepReset;
                }
                break;
        }
    }

   protected:
    bool subghz_protocol_decoder_chamb_code_check_mask_and_parse() {
        if (decode_count_bit > min_count_bit_for_found + 1)
            return false;

        if ((decode_data & CHAMBERLAIN_7_CODE_MASK) == CHAMBERLAIN_7_CODE_MASK_CHECK) {
            decode_count_bit = 7;
            decode_data &= ~CHAMBERLAIN_7_CODE_MASK;
            decode_data = (decode_data >> 12) | ((decode_data >> 4) & 0xF);
        } else if (
            (decode_data & CHAMBERLAIN_8_CODE_MASK) == CHAMBERLAIN_8_CODE_MASK_CHECK) {
            decode_count_bit = 8;
            decode_data &= ~CHAMBERLAIN_8_CODE_MASK;
            decode_data = decode_data >> 4 | CHAMBERLAIN_CODE_BIT_0 << 8;  // DIP 6 no use
        } else if (
            (decode_data & CHAMBERLAIN_9_CODE_MASK) == CHAMBERLAIN_9_CODE_MASK_CHECK) {
            decode_count_bit = 9;
            decode_data &= ~CHAMBERLAIN_9_CODE_MASK;
            decode_data >>= 4;
        } else {
            return false;
        }
        return subghz_protocol_chamb_code_to_bit(&decode_data, decode_count_bit);
    }

    bool subghz_protocol_chamb_code_to_bit(uint64_t* data, uint8_t size) {
        uint64_t data_tmp = data[0];
        uint64_t data_res = 0;
        for (uint8_t i = 0; i < size; i++) {
            if ((data_tmp & 0xFll) == CHAMBERLAIN_CODE_BIT_0) {
                bit_write(data_res, i, 0);
            } else if ((data_tmp & 0xFll) == CHAMBERLAIN_CODE_BIT_1) {
                bit_write(data_res, i, 1);
            } else {
                return false;
            }
            data_tmp >>= 4;
        }
        data[0] = data_res;
        return true;
    }
};

#endif
