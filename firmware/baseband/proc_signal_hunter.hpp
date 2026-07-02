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
    // TODO: over si že 2 000 000 je správny baseband_fs pre tvoj use-case
    static constexpr size_t baseband_fs = 2000000;

    // TODO: taps_4k25_decim_0 je placeholder skopírovaný z RTTY procesoru.
    // Over si v dsp_fir_taps.hpp aký tap je správny pre 2M -> 250k pri tvojom
    // cieľovom bandwidth-e (pozri ako proc_capture.cpp volí taps_200k_decim_0
    // podľa sample rate / oversample).
    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0{};

    std::array<complex16_t, 512> dst_buffer_data{};
    const buffer_c16_t dst_buffer{dst_buffer_data.data(), dst_buffer_data.size()};

    // --- Energy sliding window (pre detekciu) ---
    static constexpr size_t WINDOW_SIZE = 64;
    uint32_t window_buf[WINDOW_SIZE]{};
    size_t   window_idx{0};
    uint32_t window_sum{0};

    bool flush_pending{false};
    size_t flush_start_idx{0};

    // --- Pre-trigger IQ ring buffer (pre záznam pred detekciou) ---
    // TODO: 4096 vzoriek * 4 bajty (complex16_t) = 16KB. Over si dostupnú
    // M4 SRAM - porovnaj s proc_capture.cpp ktorý má dst_buffer len 512
    // vzoriek (2KB) ako orientačný odhad rozpočtu.
    static constexpr size_t IQ_RING_SAMPLES = 2048;
    std::array<complex16_t, IQ_RING_SAMPLES> iq_ring{};
    size_t iq_ring_idx{0};

    uint32_t energy_threshold{5000};
    bool     hunting{false};
    bool     configured{false};

    // --- State machine ---
    enum class HuntState {
        IDLE,            // čaká na prekročenie threshold
        AWAITING_STREAM, // trigger poslaný, čaká sa na CaptureConfigMessage od M0
        RECORDING,       // stream existuje, zapisujeme live vzorky
        HANGTIME,        // energia klesla pod threshold, čaká sa či sa vráti
        AWAITING_CLOSE   // stop poslaný, čaká sa na CaptureConfigMessage(nullptr)
    };
    HuntState hunt_state{HuntState::IDLE};

    uint32_t hangtime_counter{0};
    // TODO: tento počet je v post-decimation vzorkách (250kHz rate),
    // nie v baseband_fs vzorkách. Nastav podľa toho koľko ticha chceš tolerovať.
    //static constexpr uint32_t HANGTIME_SAMPLES = 12000; // ~48ms @ 250kHz
    uint32_t hangtime_samples_limit{0};

    std::unique_ptr<StreamInput> stream{};

    void configure();
    void reset_hunt_state();

    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
    RSSIThread     rssi_thread{};
};

#endif /*__PROC_SIGNAL_HUNTER_H__*/
