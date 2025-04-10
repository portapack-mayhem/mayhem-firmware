/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#ifndef __PROC_FLEX_H__
#define __PROC_FLEX_H__

#include "audio_output.hpp"
#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"
#include "dsp_iir_config.hpp"
#include "dsp_fir_taps.hpp"
#include "message.hpp"
#include "portapack_shared_memory.hpp"
#include "rssi_thread.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

using flex_batch_t = std::array<uint32_t, flex_batch_size>;

class FlexAudioNormalizer {
   public:
    void execute_in_place(const buffer_f32_t& audio);

   private:
    void calculate_thresholds();
    uint32_t counter_ = 0;
    float min_ = 99.0f;
    float max_ = -99.0f;
    float t_high_ = 1.0f;
    float t_mid_high_ = 0.33f;
    float t_mid_low_ = -0.33f;
    float t_low_ = -1.0f;
};

class FlexBitQueue {
   public:
    void push(uint8_t symbol);
    bool pop(uint8_t& symbol);
    void reset();
    uint8_t size() const;
    uint32_t data() const;

   private:
    uint32_t data_ = 0;
    uint8_t count_ = 0;
    static constexpr uint8_t max_size_ = sizeof(data_) * 8 / 2;
};

class FlexBitExtractor {
   public:
    FlexBitExtractor(FlexBitQueue& bits) : bits_{bits} {}
    void extract_bits(const buffer_f32_t& audio);
    void configure(uint32_t sample_rate);
    void reset();
    uint16_t baud_rate() const;

   private:
    static constexpr uint32_t sync1_pattern = 0xA6C6AAAA;
    
    struct RateInfo {
        enum class State : uint8_t {
            WaitForSample,
            ReadyToSend
        };
        const uint16_t baud_rate = 0;
        float sample_interval = 0.0;
        State state = State::WaitForSample;
        float samples_until_next = 0.0;
        float prev_value = 0.0f;
        bool is_stable = false;
        FlexBitQueue bits{};
        bool handle_sample(float sample);
        void reset();
    };
    std::array<RateInfo, 3> known_rates_{RateInfo{1600}, RateInfo{3200}, RateInfo{6400}};
    FlexBitQueue& bits_;
    uint32_t sample_rate_ = 0;
    RateInfo* current_rate_ = nullptr;
};

class FlexCodewordExtractor {
   public:
    using batch_t = flex_batch_t;
    using batch_handler_t = std::function<void(FlexCodewordExtractor&)>;
    FlexCodewordExtractor(FlexBitQueue& bits, batch_handler_t on_batch) : bits_{bits}, on_batch_{on_batch} {}
    void process_bits();
    void flush();
    void reset();
    const batch_t& batch() const { return batch_; }
    uint32_t current() const { return data_; }
    uint8_t count() const { return word_count_; }
    bool has_sync() const { return has_sync_; }

   private:
    static constexpr uint32_t sync1_codeword = 0xA6C6AAAA;
    static constexpr uint8_t data_bit_count = sizeof(uint32_t) * 8;
    void clear_data_bits();
    void take_one_symbol();
    void handle_sync();
    void save_current_codeword();
    void handle_batch_complete();
    void pad_idle();
    FlexBitQueue& bits_;
    batch_handler_t on_batch_{};
    bool has_sync_ = false;
    uint32_t data_ = 0;
    uint8_t bit_count_ = 0;
    uint8_t word_count_ = 0;
    batch_t batch_{};
};

class FlexProcessor : public BasebandProcessor {
   public:
    FlexProcessor();
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    enum class State {
        Idle,
        FrameSync,
        FrameData
    };

    static constexpr size_t baseband_fs = 3072000;
    static constexpr uint8_t stat_update_interval = 10;
    static constexpr uint32_t stat_update_threshold = baseband_fs / stat_update_interval;
    void configure();
    void flush();
    void reset();
    void send_stats() const;
    void send_packet();
    void process_frame();
    void on_beep_message(const AudioBeepMessage& message);
    bool configured = false;
    std::array<complex16_t, 256> dst{};
    const buffer_c16_t dst_buffer{dst.data(), dst.size()};
    std::array<float, 16> audio{};
    const buffer_f32_t audio_buffer{audio.data(), audio.size()};
    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0{};
    dsp::decimate::FIRC16xR16x32Decim8 decim_1{};
    dsp::decimate::FIRAndDecimateComplex channel_filter{};
    dsp::demodulate::FM demod{};
    FMSquelch squelch{};
    uint64_t squelch_history = 0;
    IIRBiquadFilter lpf{{{0.04125354f, 0.082507070f, 0.04125354f}, {1.00000000f, -1.34896775f, 0.51398189f}}};
    FlexAudioNormalizer normalizer{};
    AudioOutput audio_output{};
    FlexPacket packet{};
    uint32_t samples_processed = 0;
    FlexBitQueue bits{};
    FlexBitExtractor bit_extractor{bits};
    FlexCodewordExtractor word_extractor{bits, [this](FlexCodewordExtractor&) { send_packet(); }};
    
    State state = State::Idle;
    uint32_t sync_buffer = 0;
    std::vector<uint32_t> frame_buffer;
    uint32_t current_word = 0;
    uint8_t bit_count = 0;
    uint16_t current_baud_rate = 1600;
    
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};
};

#endif /*__PROC_FLEX_H__*/