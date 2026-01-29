#ifndef __PROC_MORSETX_H__
#define __PROC_MORSETX_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "portapack_shared_memory.hpp"
#include "audio_output.hpp"
#include "audio_dma.hpp"

#define AUDIO_OUTPUT_BUFFER_SIZE 32
#define AUDIO_SAMPLING_RATE 12000

class MorseTXProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const msg) override;

   private:
    AudioOutput audio_output{};

    int16_t audio_data[AUDIO_OUTPUT_BUFFER_SIZE];

    uint32_t audio_decimation_counter{0};
    const uint32_t decimation_factor{32};

    bool configured{false};
    bool key_down{false};

    int8_t sample{0}, re{0}, im{0};  // they have sign + and -.
    uint8_t modulation{};
    uint32_t tone_delta{0}, fm_delta{}, tone_phase{0};
    int32_t phase{0}, sphase{0}, delta{0};  // they may have sign in the pseudo random sample generation.

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{1536000, this, baseband::Direction::Transmit};
};

#endif