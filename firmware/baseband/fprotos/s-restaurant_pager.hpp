
#ifndef __FPROTO_RESTAURANT_PAGER_H__
#define __FPROTO_RESTAURANT_PAGER_H__

#include "subghzdbase.hpp"

typedef enum : uint8_t {
    RestaurantPagerDecoderStepReset = 0,
    RestaurantPagerDecoderStepSaveDuration,
    RestaurantPagerDecoderStepCheckDuration,
} RestaurantPagerDecoderStep;

class FProtoSubGhzDRestaurantPager : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDRestaurantPager() {
        sensorType = FPS_RESTAURANT_PAGER;
        te_short = 204;
        te_long = 636;
        te_delta = 100;
        min_count_bit_for_found = 25;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case RestaurantPagerDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 36) < te_delta * 36)) {
                    // Found preamble
                    parser_step = RestaurantPagerDecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                }
                break;
            case RestaurantPagerDecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = RestaurantPagerDecoderStepCheckDuration;
                }
                break;
            case RestaurantPagerDecoderStepCheckDuration:
                if (!level) {
                    if (duration >= ((uint32_t)te_long * 2)) {
                        parser_step = RestaurantPagerDecoderStepSaveDuration;
                        if (decode_count_bit == min_count_bit_for_found) {
                            // Skip all-ones preamble frames
                            if ((decode_data >> 1) != 0xFFFFFF) {
                                data_count_bit = decode_count_bit;
                                if (callback) callback(this);
                            }
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        break;
                    }

                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta * 3)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = RestaurantPagerDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta * 3) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = RestaurantPagerDecoderStepSaveDuration;
                    } else {
                        parser_step = RestaurantPagerDecoderStepReset;
                    }
                } else {
                    parser_step = RestaurantPagerDecoderStepReset;
                }
                break;
        }
    }
};

#endif
