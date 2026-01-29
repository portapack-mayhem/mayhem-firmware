#include "proc_morsetx.hpp"
#include "portapack_shared_memory.hpp"
#include "sine_table_int8.hpp"
#include "event_m4.hpp"

#include <cstdint>

void MorseTXProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    buffer_s16_t audio_buffer{audio_data, AUDIO_OUTPUT_BUFFER_SIZE, AUDIO_SAMPLING_RATE};

    for (size_t i = 0; i < buffer.count; i++) {
        int8_t sample_sin;
        int8_t sample_cos;

        sample_sin = (sine_table_i8[(tone_phase & 0xFF000000) >> 24]);

        tone_phase += tone_delta;

        audio_decimation_counter++;
        if (audio_decimation_counter >= decimation_factor) {
            audio_decimation_counter = 0;

            int16_t audio_sample = 0;

            if (key_down) {
                int32_t amplified = (int32_t)sample_sin * 258;
                audio_sample = (int16_t)amplified;
            }

            audio_data[i % AUDIO_OUTPUT_BUFFER_SIZE] = audio_sample;

            if ((i % AUDIO_OUTPUT_BUFFER_SIZE) == AUDIO_OUTPUT_BUFFER_SIZE - 1) {
                audio_output.write(audio_buffer);
            }
        }

        // modulation logic
        if (modulation == 0) {  // AM modulation
            im = 0;
            if (key_down) {
                re = 64 + (sample_sin >> 2);
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
            im = 0;
            if (key_down) {
                re = sample_sin;
            } else
                re = 0;

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
                im = (sine_table_i8[((tone_phase + 0x80000000) & 0xFF000000) >> 24]);
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
            tone_delta = message.tone * 1398;  // scale
            modulation = message.modulation;
            audio_output.configure(audio_12k_hpf_300hz_config);
            if (message.fm_delta == 0 && modulation == 1) {
                fm_delta = 90000;
            } else {
                uint64_t scale = 0xFFFFFFFFULL;  // 32-bit max
                fm_delta = (uint32_t)(((uint64_t)message.fm_delta * scale) / 1536000);
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
    audio::dma::init_audio_out();
    EventDispatcher event_dispatcher{std::make_unique<MorseTXProcessor>()};
    event_dispatcher.run();
    return 0;
}