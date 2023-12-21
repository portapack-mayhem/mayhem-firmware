#ifndef __FPROTO_GENERAL_H__
#define __FPROTO_GENERAL_H__

// useful methods for both weather and subghzd

#include <stdint.h>
#include <stddef.h>

#define bit_read(value, bit) (((value) >> (bit)) & 0x01)
#define bit_set(value, bit)           \
    ({                                \
        __typeof__(value) _one = (1); \
        (value) |= (_one << (bit));   \
    })
#define bit_clear(value, bit)         \
    ({                                \
        __typeof__(value) _one = (1); \
        (value) &= ~(_one << (bit));  \
    })
#define bit_write(value, bit, bitvalue) (bitvalue ? bit_set(value, bit) : bit_clear(value, bit))

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

class FProtoGeneral {
   public:
    static bool manchester_advance(
        ManchesterState state,
        ManchesterEvent event,
        ManchesterState* next_state,
        bool* data) {
        bool result = false;
        ManchesterState new_state;

        if (event == ManchesterEventReset) {
            new_state = ManchesterStateMid1;
        } else {
            new_state = (ManchesterState)(transitions[state] >> event & 0x3);
            if (new_state == state) {
                new_state = ManchesterStateMid1;
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
    static uint8_t subghz_protocol_blocks_get_parity(uint64_t key, uint8_t bit_count) {
        uint8_t parity = 0;
        for (uint8_t i = 0; i < bit_count; i++) {
            parity += bit_read(key, i);
        }
        return parity & 0x01;
    }

    static uint8_t subghz_protocol_blocks_add_bytes(uint8_t const message[], size_t size) {
        uint32_t result = 0;
        for (size_t i = 0; i < size; ++i) {
            result += message[i];
        }
        return (uint8_t)result;
    }

    static uint8_t subghz_protocol_blocks_parity8(uint8_t byte) {
        byte ^= byte >> 4;
        byte &= 0xf;
        return (0x6996 >> byte) & 1;
    }

    static uint8_t subghz_protocol_blocks_parity_bytes(uint8_t const message[], size_t size) {
        uint8_t result = 0;
        for (size_t i = 0; i < size; ++i) {
            result ^= subghz_protocol_blocks_parity8(message[i]);
        }
        return result;
    }

    static uint8_t subghz_protocol_blocks_lfsr_digest8(
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
    static float locale_fahrenheit_to_celsius(float temp_f) {
        return (temp_f - 32.f) / 1.8f;
    }

    static uint8_t subghz_protocol_blocks_crc4(
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
    static uint8_t subghz_protocol_blocks_lfsr_digest8_reflect(
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
    static uint64_t subghz_protocol_blocks_reverse_key(uint64_t key, uint8_t bit_count) {
        uint64_t reverse_key = 0;
        for (uint8_t i = 0; i < bit_count; i++) {
            reverse_key = reverse_key << 1 | bit_read(key, i);
        }
        return reverse_key;
    }
    static uint8_t subghz_protocol_blocks_crc8(
        uint8_t const message[],
        size_t size,
        uint8_t polynomial,
        uint8_t init) {
        uint8_t remainder = init;

        for (size_t byte = 0; byte < size; ++byte) {
            remainder ^= message[byte];
            for (uint8_t bit = 0; bit < 8; ++bit) {
                if (remainder & 0x80) {
                    remainder = (remainder << 1) ^ polynomial;
                } else {
                    remainder = (remainder << 1);
                }
            }
        }
        return remainder;
    }

   private:
    static inline const uint8_t transitions[] = {0b00000001, 0b10010001, 0b10011011, 0b11111011};
};

#endif