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

        sample_sin = (sine_table_i8[(tone_phase & 0xFF000000) >> 24]);
        tone_phase += tone_delta;

        im = 0;
        re = 0;

        // modulation logic
        if (modulation == 0) {  // AM modulation
            if (key_down) {
                re = 64 + (sample_sin >> 2);
            }
        } else if (modulation == 1) {  // FM modulation
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
            if (key_down) {
                re = sample_sin;
            }
        } else if (modulation == 3) {  // USB
            if (key_down) {
                sample_cos = (sine_table_i8[((tone_phase + 0x40000000) & 0xFF000000) >> 24]);
                re = sample_cos;
                im = sample_sin;
            }
        } else if (modulation == 4) {  // LSB
            if (key_down) {
                sample_cos = (sine_table_i8[((tone_phase + 0x40000000) & 0xFF000000) >> 24]);
                re = sample_cos;
                im = (sine_table_i8[((tone_phase + 0x80000000) & 0xFF000000) >> 24]);
            }
        }
        buffer.p[i] = {re, im};
    }
}

void MorseTXProcessor::on_message(const Message* const p) {
    switch (p->id) {
        case Message::ID::MorseTXConfigure: {
            auto message = *reinterpret_cast<const MorseTXConfigureMessage*>(p);
            tone_delta = message.tone * 1398;  // scale
            tone = message.tone;               // audio tone
            modulation = message.modulation;
            if (message.fm_delta == 0 && modulation == 1) {
                fm_delta = 90000;
            } else {
                uint64_t scale = 0xFFFFFFFFULL;  // 32-bit max
                fm_delta = (uint32_t)((uint64_t)message.fm_delta * (scale / 1536000));
            }
            break;
        }

        case Message::ID::MorseTXkey: {
            auto key = *reinterpret_cast<const MorseTXkeyMessage*>(p);
            key_down = key.key_down;
            configured = true;
            if (key_down)
                audio::dma::beep_start(tone, 24000, 0);
            else
                audio::dma::beep_stop();
            break;
        }

        default:
            break;
    }
}

int main() {
    audio::dma::init_audio_out();
    EventDispatcher event_dispatcher{std::make_unique<MorseTXProcessor>()};
    event_dispatcher.run();
    return 0;
}