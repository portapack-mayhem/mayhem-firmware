#ifndef __PROC_SIGNAL_HUNTER_H__
#define __PROC_SIGNAL_HUNTER_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"
#include "message.hpp"
#include "dsp_decimate.hpp"
#include "stream_input.hpp"

#include <cstdint>
#include <cstddef>
#include <array>
#include <memory>

class SignalHunterProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    // TODO: Verify baseband_fs = 2 MHz is correct for the use case.
    static constexpr size_t baseband_fs = 2000000;

    // TODO: taps_4k25_decim_0 is placeholder copied from RTTY processor.
    // Verify correct tap coefficients for 2M->250k decimation ratio with your target bandwidth.
    // Reference: See how proc_capture.cpp selects taps (e.g., taps_200k_decim_0) based on
    // baseband sample rate and oversample configuration.
    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0{};

    std::array<complex16_t, 512> dst_buffer_data{};
    const buffer_c16_t dst_buffer{dst_buffer_data.data(), dst_buffer_data.size()};

    // --- Energy sliding window for threshold detection ---
    static constexpr size_t WINDOW_SIZE = 64;
    uint32_t window_buf[WINDOW_SIZE]{};
    size_t   window_idx{0};
    uint32_t window_sum{0};

    bool flush_pending{false};
    size_t flush_start_idx{0};

    // Pre-trigger IQ ring buffer
    // IQ_RING_SAMPLES = 2048 provides ~8ms pre-roll buffer (2048 / 250kHz).
    // This captures OOK preambles efficiently while maintaining strict 8KB flush size to prevent
    // Out-Of-Memory (OOM) / HardFault crashes on M0 CaptureThread, which has only 16KB buffer constraint.
    static constexpr size_t IQ_RING_SAMPLES = 2048;
    std::array<complex16_t, IQ_RING_SAMPLES> iq_ring{};
    size_t iq_ring_idx{0};

    uint32_t energy_threshold{5000};
    bool     hunting{false};
    bool     configured{false};

    // State machine tracking signal detection lifecycle
    enum class HuntState {
        IDLE,            // Awaiting energy threshold crossing
        AWAITING_STREAM, // Trigger sent; waiting for CaptureConfigMessage from M0 to create stream
        RECORDING,       // Stream active; writing live decimated samples
        HANGTIME,        // Energy dropped below threshold; checking if signal returns
        AWAITING_CLOSE   // Stop sent; waiting for CaptureConfigMessage(nullptr) from M0 to destroy stream
    };
    HuntState hunt_state{HuntState::IDLE};

    uint32_t hangtime_counter{0};
    // Dynamic hangtime configuration: holds sample count in post-decimation domain (250 kHz rate).
    // Converted from milliseconds: hangtime_ms * 250 samples/ms.
    // Set dynamically via HunterConfigMessage to allow configurable silence tolerance.
    uint32_t hangtime_samples_limit{0};

    std::unique_ptr<StreamInput> stream{};

    void configure();
    void reset_hunt_state();

    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
    RSSIThread     rssi_thread{};
};

#endif /*__PROC_SIGNAL_HUNTER_H__*/
