#include "proc_morsetx.hpp"
#include "portapack_shared_memory.hpp"
#include "sine_table_int8.hpp"
#include "event_m4.hpp"

#include <cstdint>

void MorseTXProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    for (size_t i = 0; i < buffer.count; i++) {
        int8_t sample_sin;
        int8_t sample_cos;

        if (key_down) {
            sample_sin = (sine_table_i8[(tone_phase & 0xFF000000) >> 24]);
            tone_phase += tone_delta;
        } else {
            sample_sin = 0;
            sample_cos = 0;
        }

        // modulation logic

        if (modulation == 0) {  // AM modulation
            im = 0;
            if (key_down) {
                re = 127;  // max. signed 8 bits value .   (-128 ...+127), max. amplitude ,  static phasor at 0º
            } else {
                re = 0;
            }

        } else if (modulation == 1) {  // FM modulation
            // In FM, 'silence' means that the frequency is not oscillating.
            // continuous carrier wave
            if (key_down) {
                delta = sample_sin * fm_delta;
            } else {
                delta = 0;
            }

            phase += delta;
            sphase = phase + (64 << 24);

            re = (sine_table_i8[(sphase & 0xFF000000) >> 24]);
            im = (sine_table_i8[(phase & 0xFF000000) >> 24]);

        } else if (modulation == 2) {  // DSB
            re = sample_sin;
            im = 0;

        } else if (modulation == 3) {  // USB
            if (key_down) {
                sample_cos = (sine_table_i8[((tone_phase + 0x40000000) & 0xFF000000) >> 24]);
                re = sample_cos;
                im = sample_sin;
            } else {
                re = 0;
                im = 0;
            }

        } else if (modulation == 4) {  // LSB
            if (key_down) {
                sample_cos = (sine_table_i8[((tone_phase + 0x40000000) & 0xFF000000) >> 24]);
                re = sample_cos;
                im = -sample_sin;
            } else {
                re = 0;
                im = 0;
            }
        }

        buffer.p[i] = {re, im};
    }
}

void MorseTXProcessor::on_message(const Message* const p) {
    switch (p->id) {
        case Message::ID::MorseTXConfigure: {
            auto message = *reinterpret_cast<const MorseTXConfigureMessage*>(p);
            tone_delta = message.tone;
            modulation = message.modulation;
            key_down = message.key_down;
            if (message.fm_delta == 0 && modulation == 1) {
                fm_delta = 90000;
            } else {
                fm_delta = message.fm_delta;
            }
            break;
        }

        case Message::ID::MorseTXkey: {
            auto key = *reinterpret_cast<const MorseTXkeyMessage*>(p);
            key_down = key.key_down;
            configured = true;
            break;
        }

        default:
            break;
    }
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<MorseTXProcessor>()};
    event_dispatcher.run();
    return 0;
}