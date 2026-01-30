#ifndef __PROC_MORSETX_H__
#define __PROC_MORSETX_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "portapack_shared_memory.hpp"
#include "audio_output.hpp"
#include "audio_dma.hpp"

#define AUDIO_OUTPUT_BUFFER_SIZE 32
#define AUDIO_SAMPLING_RATE 24000

class MorseTXProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const msg) override;

   private:
    bool configured{false};
    bool key_down{false};  // currently the virtual key is pressed or not.

    int8_t sample{0}, re{0}, im{0};
    uint8_t modulation{0};   // 0=AM, 1=FM, 2=DSB, 3=USB, 4=LSB
    uint32_t tone_delta{0};  // shifting value by tone
    uint32_t fm_delta{};
    uint32_t tone_phase{0};
    uint32_t tone{0};                       // audio tone frequeny
    int32_t phase{0}, sphase{0}, delta{0};  // sample generation.

    BasebandThread baseband_thread{1536000, this, baseband::Direction::Transmit};
};

#endif