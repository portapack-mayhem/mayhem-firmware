/*
Base class for all weather protocols.
This and most of the weather protocols uses code from Flipper XTreme codebase ( https://github.com/Flipper-XFW/Xtreme-Firmware/tree/dev/lib/subghz ). Thanks for their work!
For comments in a protocol implementation check w-nexus-th.hpp
*/

#ifndef __FPROTO_BASE_H__
#define __FPROTO_BASE_H__

#define bit_read(value, bit) (((value) >> (bit)) & 0x01)

#include "weathertypes.hpp"

#include <string>
// default walues to indicate 'no value'
#define WS_NO_ID 0xFFFFFFFF
#define WS_NO_BATT 0xFF
#define WS_NO_HUMIDITY 0xFF
#define WS_NO_CHANNEL 0xFF
#define WS_NO_BTN 0xFF
#define WS_NO_TEMPERATURE -273.0f

#define DURATION_DIFF(x, y) (((x) < (y)) ? ((y) - (x)) : ((x) - (y)))
typedef enum {
    ManchesterStateStart1 = 0,
    ManchesterStateMid1 = 1,
    ManchesterStateMid0 = 2,
    ManchesterStateStart0 = 3
} ManchesterState;
typedef enum {
    ManchesterEventShortLow = 0,
    ManchesterEventShortHigh = 2,
    ManchesterEventLongLow = 4,
    ManchesterEventLongHigh = 6,
    ManchesterEventReset = 8
} ManchesterEvent;
class FProtoWeatherBase;
typedef void (*SubGhzProtocolDecoderBaseRxCallback)(FProtoWeatherBase* instance);

class FProtoWeatherBase {
   public:
    FProtoWeatherBase() {}
    virtual ~FProtoWeatherBase() {}
    virtual void feed(bool level, uint32_t duration) = 0;                        // need to be implemented on each protocol handler.
    void setCallback(SubGhzProtocolDecoderBaseRxCallback cb) { callback = cb; }  // this is called when there is a hit.

    uint8_t getSensorType() { return sensorType; }
    uint32_t getSensorId() { return id; }
    float getTemp() { return temp; }
    uint8_t getHumidity() { return humidity; }
    uint8_t getBattLow() { return battery_low; }
    uint32_t getTimestamp() { return timestamp; }
    uint8_t getChannel() { return channel; }
    uint8_t getButton() { return btn; }

   protected:
    // Helper functions to keep it as compatible with flipper as we can, so adding new protos will be easy.
    void subghz_protocol_blocks_add_bit(uint8_t bit) {
        decode_data = decode_data << 1 | bit;
        decode_count_bit++;
    }
    uint8_t subghz_protocol_blocks_add_bytes(uint8_t const message[], size_t size) {
        uint32_t result = 0;
        for (size_t i = 0; i < size; ++i) {
            result += message[i];
        }
        return (uint8_t)result;
    }
    uint8_t subghz_protocol_blocks_parity8(uint8_t byte) {
        byte ^= byte >> 4;
        byte &= 0xf;
        return (0x6996 >> byte) & 1;
    }
    uint8_t subghz_protocol_blocks_parity_bytes(uint8_t const message[], size_t size) {
        uint8_t result = 0;
        for (size_t i = 0; i < size; ++i) {
            result ^= subghz_protocol_blocks_parity8(message[i]);
        }
        return result;
    }
    uint8_t subghz_protocol_blocks_lfsr_digest8(
        uint8_t const message[],
        size_t size,
        uint8_t gen,
        uint8_t key) {
        uint8_t sum = 0;
        for (size_t byte = 0; byte < size; ++byte) {
            uint8_t data = message[byte];
            for (int i = 7; i >= 0; --i) {
                // XOR key into sum if data bit is set
                if ((data >> i) & 1) sum ^= key;
                // roll the key right (actually the LSB is dropped here)
                // and apply the gen (needs to include the dropped LSB as MSB)
                if (key & 1)
                    key = (key >> 1) ^ gen;
                else
                    key = (key >> 1);
            }
        }
        return sum;
    }
    float locale_fahrenheit_to_celsius(float temp_f) {
        return (temp_f - 32.f) / 1.8f;
    }
    bool manchester_advance(
        ManchesterState state,
        ManchesterEvent event,
        ManchesterState* next_state,
        bool* data) {
        bool result = false;
        ManchesterState new_state;

        if (event == ManchesterEventReset) {
            new_state = manchester_reset_state;
        } else {
            new_state = (ManchesterState)(transitions[state] >> event & 0x3);
            if (new_state == state) {
                new_state = manchester_reset_state;
            } else {
                if (new_state == ManchesterStateMid0) {
                    if (data) *data = false;
                    result = true;
                } else if (new_state == ManchesterStateMid1) {
                    if (data) *data = true;
                    result = true;
                }
            }
        }

        *next_state = new_state;
        return result;
    }
    uint8_t subghz_protocol_blocks_crc4(
        uint8_t const message[],
        size_t size,
        uint8_t polynomial,
        uint8_t init) {
        uint8_t remainder = init << 4;  // LSBs are unused
        uint8_t poly = polynomial << 4;
        uint8_t bit;

        while (size--) {
            remainder ^= *message++;
            for (bit = 0; bit < 8; bit++) {
                if (remainder & 0x80) {
                    remainder = (remainder << 1) ^ poly;
                } else {
                    remainder = (remainder << 1);
                }
            }
        }
        return remainder >> 4 & 0x0f;  // discard the LSBs
    }
    uint8_t subghz_protocol_blocks_lfsr_digest8_reflect(
        uint8_t const message[],
        size_t size,
        uint8_t gen,
        uint8_t key) {
        uint8_t sum = 0;
        // Process message from last byte to first byte (reflected)
        for (int byte = size - 1; byte >= 0; --byte) {
            uint8_t data = message[byte];
            // Process individual bits of each byte (reflected)
            for (uint8_t i = 0; i < 8; ++i) {
                // XOR key into sum if data bit is set
                if ((data >> i) & 1) {
                    sum ^= key;
                }
                // roll the key left (actually the LSB is dropped here)
                // and apply the gen (needs to include the dropped lsb as MSB)
                if (key & 0x80)
                    key = (key << 1) ^ gen;
                else
                    key = (key << 1);
            }
        }
        return sum;
    }
    uint64_t subghz_protocol_blocks_reverse_key(uint64_t key, uint8_t bit_count) {
        uint64_t reverse_key = 0;
        for (uint8_t i = 0; i < bit_count; i++) {
            reverse_key = reverse_key << 1 | bit_read(key, i);
        }
        return reverse_key;
    }
    // General weather data holder
    uint8_t sensorType = FPW_Invalid;
    uint32_t id = WS_NO_ID;
    float temp = WS_NO_TEMPERATURE;
    uint8_t humidity = WS_NO_HUMIDITY;
    uint8_t battery_low = WS_NO_BATT;
    uint32_t timestamp = 0;
    uint8_t channel = WS_NO_CHANNEL;
    uint8_t btn = WS_NO_BTN;

    // inner logic stuff, also for flipper compatibility. //todo revork a bit, so won't have dupes (decode_data + data, ..), but check if any of the protos uses it in the same time or not. (shouldn't)
    SubGhzProtocolDecoderBaseRxCallback callback = NULL;
    uint16_t header_count = 0;
    uint8_t parser_step = 0;
    uint32_t te_last = 0;
    uint64_t data = 0;
    uint32_t data_count_bit = 0;
    uint64_t decode_data = 0;
    uint32_t decode_count_bit = 0;
    ManchesterState manchester_saved_state = ManchesterStateMid1;
    static const ManchesterState manchester_reset_state = ManchesterStateMid1;
    static inline const uint8_t transitions[] = {0b00000001, 0b10010001, 0b10011011, 0b11111011};
};

#endif