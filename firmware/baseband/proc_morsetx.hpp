#ifndef __PROC_MORSETX_H__
#define __PROC_MORSETX_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "portapack_shared_memory.hpp"
#include "audio_output.hpp"

class MorseTXProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const msg) override;

   private:
    AudioOutput audio_output{};

    std::array<float, 32> audio_buffer{};
    size_t audio_buffer_index{0};

    uint32_t audio_decimation_counter{0};
    const uint32_t decimation_factor{64};

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