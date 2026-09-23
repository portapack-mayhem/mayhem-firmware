
#ifndef __FPROTO_FINEOFFSET_OOK_H__
#define __FPROTO_FINEOFFSET_OOK_H__

#include "weatherbase.hpp"

/*
Fine Offset Electronics / Ecowitt OOK sensors.
Protocol details are from rtl_433 ( https://github.com/merbanan/rtl_433 src/devices/fineoffset.c, fineoffset_wh1080.c, ecowitt.c ).

All of them use OOK PWM: ~500us pulse = 1, ~1500us pulse = 0, fixed ~1000us gap. The packet is told apart by its bit count:
    47 WH5 (7 bit preamble)
    48 WH2 (8 bit preamble)
    49 Telldus / Proove (WH2 with 9 bit preamble)
    54-56 WH2A, TFA 30.3225 (7 bit preamble), Ecowitt WH53, WH0280, WH0281A (6-8 bit preamble)
    71 WH0530, Agimex Rosenborg 35926 (7 bit preamble)
    87-99 WH1080, WH3080 weather (7 or 8 bit preamble)
    63-64 WH3080 UV / light (7 or 8 bit preamble)
The payload after the preamble is passed to the app.
*/

class FProtoWeatherFineOffsetOOK : public FProtoWeatherBase {
   public:
    FProtoWeatherFineOffsetOOK() {
        sensorType = FPW_FO_WH2;
    }

    void feed(bool level, uint32_t duration) override {
        // A weak signal gets chopped into pieces by the slicer. All valid pulses and gaps are >400us,
        // so merge the short pieces into the run they are in, and handle a run only when the level really changes.
        if (duration < min_run || level == run_level) {
            run_duration += duration;
        } else {
            handle_run(run_level, run_duration);
            run_level = level;
            run_duration = duration;
        }
        // a long gap: the packet is over, don't wait for the next pulse
        if (!run_level && run_duration > end_gap) {
            handle_run(false, run_duration);
            run_duration = 0;
        }
    }

    const uint8_t* getPayload(uint8_t& len) override {
        len = payload_len;
        return payload;
    }

   protected:
    static constexpr uint32_t max_bits = 104;
    static constexpr uint32_t min_run = 100;
    static constexpr uint32_t end_gap = 1400;
    uint8_t bits[max_bits / 8]{};
    uint8_t payload[11]{};
    uint8_t payload_len = 0;
    bool run_level = false;
    uint32_t run_duration = 0;

    void handle_run(bool level, uint32_t duration) {
        if (level) {
            uint8_t bit;
            if (duration > 250 && duration < 900) {
                bit = 1;
            } else if (duration > 1100 && duration < 2000) {
                bit = 0;
            } else {
                decode_count_bit = 0;
                return;
            }
            if (decode_count_bit >= max_bits) {
                decode_count_bit = 0;
                return;
            }
            uint8_t& b = bits[decode_count_bit / 8];
            b = (b << 1) | bit;
            decode_count_bit++;
        } else if (decode_count_bit) {
            if (duration > end_gap) {
                // end of the packet. pad the last partial byte
                if (decode_count_bit % 8) bits[decode_count_bit / 8] <<= 8 - (decode_count_bit % 8);
                check_packet();
                decode_count_bit = 0;
            } else if (duration < 600) {
                decode_count_bit = 0;
            }
        }
    }

    // get 'len' bytes to payload, starting at bit position 'pos'
    __attribute__((noinline)) void extract(uint8_t pos, uint8_t len) {
        for (uint8_t i = 0; i < len; i++) {
            uint8_t p = pos + i * 8;
            uint8_t v = bits[p / 8] << (p % 8);
            if ((p % 8) && p / 8 + 1u < sizeof(bits)) v |= bits[p / 8 + 1] >> (8 - p % 8);
            payload[i] = v;
        }
        payload_len = len;
    }

    __attribute__((noinline)) uint8_t crc8(uint8_t len) {
        return FProtoGeneral::subghz_protocol_blocks_crc8(payload, len, 0x31, 0x00);
    }

    // count of leading 1 bits (the preamble)
    __attribute__((noinline)) uint8_t preamble_len() {
        uint8_t n = 0;
        while (n < 16 && (bits[n / 8] & (0x80 >> (n % 8)))) n++;
        return n;
    }

    __attribute__((noinline)) bool try_wh2(uint8_t pos, uint8_t type) {
        extract(pos, 5);
        if (crc8(4) != payload[4] || (payload[0] >> 4) != 4) return false;
        sensorType = type;
        return true;
    }

    void check_packet() {
        uint8_t n = decode_count_bit;
        uint8_t pre = preamble_len();
        bool ok = false;
        if (n == 48 && pre == 8) {
            ok = try_wh2(8, FPW_FO_WH2);
        } else if (n == 49 && pre == 9) {
            ok = try_wh2(9, FPW_FO_WH2);  // Telldus / Proove
        } else if (n == 47 && pre == 7) {
            // collides with Agimex Rosenborg 66796, reject implausible temperatures (-40 .. 60C)
            ok = try_wh2(7, FPW_FO_WH5) && ((((payload[1] & 0x0F) << 8) | payload[2]) <= 1000);
        } else if (n >= 54 && n <= 56) {
            // Ecowitt WH53: the preamble is not stable, and the model code 0x53 starts with a 0 bit
            if (pre >= 6 && pre <= 8) {
                extract(pre, 6);
                if (payload[0] == 0x53 && crc8(6) == 0 && payload[4] == 0xFF) {
                    sensorType = FPW_FO_WH53;
                    ok = true;
                }
            }
            if (!ok && n == 55 && pre == 7) {
                // WH2A, or TFA 30.3225 that has no humidity (0xFF) and WH5 style temperature
                ok = try_wh2(7, FPW_FO_WH2);
                if (ok) {
                    extract(7, 6);
                    if (payload[3] == 0xFF) {
                        ok = FProtoGeneral::subghz_protocol_blocks_add_bytes(payload, 5) == payload[5];
                        sensorType = FPW_FO_WH5;
                    }
                    payload_len = 5;
                }
            }
        } else if (n == 71 && pre == 7) {
            extract(7, 8);
            ok = (payload[0] >> 4) == 3 && crc8(7) == 0 &&
                 FProtoGeneral::subghz_protocol_blocks_add_bytes(payload, 7) == payload[7];
            sensorType = FPW_FO_WH0530;
        } else if ((n >= 87 && n < 100 && pre >= 7) || ((n == 63 || n == 64) && pre >= 7)) {
            // WH1080 / WH3080. 7 or 8 bit preamble, the payload starts with a 0xA (weather) or 0x7 (UV) nibble.
            // the preamble is followed by a 1 bit in the weather message, so use the bit count to get its length
            uint8_t p = (n == 87 || n == 63) ? 7 : 8;
            bool uv = n < 70;
            extract(p, uv ? 7 : 10);
            if (crc8(payload_len) == 0) {
                if (!uv && (payload[0] >> 4) == 0x0A) {
                    sensorType = FPW_FO_WH1080;
                    ok = true;
                } else if (uv && (payload[0] >> 4) == 0x07) {
                    sensorType = FPW_FO_WH3080UV;
                    ok = true;
                }
            }
        }
        if (ok && callback) callback(this);
    }
};

#endif
