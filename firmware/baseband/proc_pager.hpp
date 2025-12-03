/*
 * Unified Pager (POCSAG + FLEX) baseband processor.
 *
 * This file combines a simplified version of the POCSAG2 processor with the
 * standalone FLEX processor so that a single baseband image can decode both.
 */

#ifndef __PROC_PAGER_H__
#define __PROC_PAGER_H__

#include "baseband_processor.hpp"
#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"
#include "dsp_iir.hpp"
#include "dsp_squelch.hpp"
#include "audio_output.hpp"
#include "message.hpp"
#include "pocsag.hpp"
#include "pocsag_packet.hpp"
#include "flex_defs.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"

#include <array>
#include <cstdint>
#include <functional>

namespace pager {

enum class Mode {
    POCSAG,
    FLEX,
    AUTO
};

}  // namespace pager

// --- FLEX internal types ----------------------------------------------------

namespace flex {

enum class PageType {
    SECURE,
    SHORT_INSTRUCTION,
    TONE,
    STANDARD_NUMERIC,
    SPECIAL_NUMERIC,
    ALPHANUMERIC,
    BINARY,
    NUMBERED_NUMERIC
};

enum class State {
    SYNC1,
    FIW,
    SYNC2,
    DATA
};

struct FlexDemodParams {
    unsigned int sample_freq = 24000;
    double sample_last = 0.0;
    int locked = 0;
    int phase = 0;
    unsigned int sample_count = 0;
    unsigned int symbol_count = 0;
    double envelope_sum = 0.0;
    int envelope_count = 0;
    uint64_t lock_buf = 0;
    int symcount[4] = {0};
    int timeout = 0;
    int nonconsec = 0;
    unsigned int baud = 1600;
};

struct FlexGroupHandler {
    int64_t GroupCodes[17][100];
    int GroupCycle[17];
    int GroupFrame[17];
};

struct FlexModulation {
    double symbol_rate = 0.0;
    double envelope = 0.0;
    double zero = 0.0;
};

struct FlexStateInfo {
    unsigned int sync2_count = 0;
    unsigned int data_count = 0;
    unsigned int fiwcount = 0;
    State Current = State::SYNC1;
    State Previous = State::SYNC1;
};

struct FlexSync {
    unsigned int sync = 0;
    unsigned int baud = 0;
    unsigned int levels = 0;
    unsigned int polarity = 0;
    uint64_t syncbuf = 0;
};

struct FlexFIW {
    uint32_t rawdata = 0;
    unsigned int checksum = 0;
    unsigned int cycleno = 0;
    unsigned int frameno = 0;
    unsigned int fix3 = 0;
};

struct FlexPhase {
    uint32_t buf[88] = {0};
    int idle_count = 0;
};

struct FlexData {
    int phase_toggle = 0;
    unsigned int data_bit_counter = 0;
    FlexPhase PhaseA;
    FlexPhase PhaseB;
    FlexPhase PhaseC;
    FlexPhase PhaseD;
};

struct FlexDecode {
    PageType type = PageType::ALPHANUMERIC;
    int long_address = 0;
    int64_t capcode = 0;
};

}  // namespace flex

// --- POCSAG helpers ---------------------------------------------------------

class AudioNormalizer {
   public:
    void execute_in_place(const buffer_f32_t& audio);

   private:
    float min_ = 0.0f;
    float max_ = 0.0f;
    float t_hi_ = 0.5f;
    float t_lo_ = -0.5f;
    uint32_t counter_ = 0;

    void calculate_thresholds();
};

class BitQueue {
   public:
    void push(bool bit);
    bool pop();
    void reset();
    uint8_t size() const;
    uint32_t data() const;

   private:
    static constexpr uint8_t max_size_ = 32;
    uint32_t data_ = 0;
    uint8_t count_ = 0;
};

class BitExtractor {
   public:
    void extract_bits(const buffer_f32_t& audio);
    void configure(uint32_t sample_rate);
    void reset();
    void set_baud_config(int8_t baud_config);
    uint16_t baud_rate() const;

   private:
    BitQueue bits_;

    struct RateInfo {
        uint16_t baud_rate;
        double sample_interval;
        double samples_until_next;
        bool prev_value;
        bool is_stable;
        BitQueue bits;
        enum class State {
            WaitForSample,
            ReadyToSend
        } state;

        bool handle_sample(float sample);
        void reset();
    };

    static constexpr uint32_t clock_magic_number = 0xAAAAAAAA;
    std::array<RateInfo, 4> known_rates_{{
        {512, 0.0, 0.0, false, false, {}, RateInfo::State::WaitForSample},
        {1200, 0.0, 0.0, false, false, {}, RateInfo::State::WaitForSample},
        {2400, 0.0, 0.0, false, false, {}, RateInfo::State::WaitForSample},
        {3200, 0.0, 0.0, false, false, {}, RateInfo::State::WaitForSample},
    }};
    RateInfo* current_rate_ = nullptr;
    uint32_t sample_rate_ = 0;
    int8_t baud_config_ = -1;
};

class CodewordExtractor {
   public:
    void process_bits();
    void flush();
    void reset();

    uint32_t current() const { return batch_[word_count_]; }
    uint8_t count() const { return word_count_; }
    bool has_sync() const { return has_sync_; }
    const pocsag::batch_t& batch() const { return batch_; }

    std::function<void(CodewordExtractor&)> on_batch_;

    BitQueue& bits_;

    explicit CodewordExtractor(BitQueue& bits)
        : bits_{bits} {}

   private:
    static constexpr uint32_t sync_codeword = 0x7CD215D8;
    static constexpr uint32_t idle_codeword = 0x7A89C197;
    static constexpr uint8_t data_bit_count = 32;

    uint32_t data_ = 0;
    uint8_t bit_count_ = 0;
    bool has_sync_ = false;
    bool inverted_ = false;

    uint8_t word_count_ = 0;
    pocsag::batch_t batch_{};

    void clear_data_bits();
    void take_one_bit();
    void handle_sync(bool inverted);
    void save_current_codeword();
    void handle_batch_complete();
    void pad_idle();
};

// --- PagerProcessor ---------------------------------------------------------

class PagerProcessor : public BasebandProcessor {
   public:
    PagerProcessor();
    ~PagerProcessor();
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    bool configured{false};
    pager::Mode mode = pager::Mode::POCSAG;

    // DSP chain: 3.072 MHz IQ -> 24 kHz audio
    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0{};
    dsp::decimate::FIRC16xR16x32Decim8 decim_1{};
    dsp::decimate::FIRAndDecimateComplex channel_filter{};
    dsp::demodulate::FM demod{};

    // Buffers
    std::array<complex16_t, 256> dst{};
    const buffer_c16_t dst_buffer{dst.data(), dst.size()};

    std::array<complex16_t, 256> intermediate{};
    const buffer_c16_t intermediate_buffer{intermediate.data(), intermediate.size()};

    std::array<float, 16> audio{};
    const buffer_f32_t audio_buffer{audio.data(), audio.size()};

    // --- POCSAG state ---
    static constexpr size_t baseband_fs = 3072000;
    static constexpr uint8_t stat_update_interval = 10;
    static constexpr uint32_t stat_update_threshold = baseband_fs / stat_update_interval;

    FMSquelch squelch{};
    uint64_t squelch_history = 0;

    IIRBiquadFilter lpf{{{0.04125354f, 0.082507070f, 0.04125354f},
                         {1.00000000f, -1.34896775f, 0.51398189f}}};

    AudioNormalizer normalizer{};
    AudioOutput audio_output{};

    BitQueue bits{};
    BitExtractor bit_extractor{};
    CodewordExtractor word_extractor{bits};
    pocsag::POCSAGPacket packet{};

    uint32_t samples_processed = 0;

    void configure_pocsag(int8_t baud_config);
    void execute_pocsag(const buffer_f32_t& audio);
    void flush_pocsag();
    void reset_pocsag();
    void send_stats_pocsag() const;
    void send_packet_pocsag();
    void on_beep_message(const AudioBeepMessage& message);

    // --- FLEX state ---
    flex::FlexDemodParams flex_demodulator{};
    flex::FlexModulation flex_modulation{};
    flex::FlexStateInfo flex_state{};
    flex::FlexSync flex_sync{};
    flex::FlexFIW flex_fiw{};
    flex::FlexData flex_data{};
    flex::FlexDecode flex_decode{};
    flex::FlexGroupHandler flex_group_handler{};

    // pocsag::EccContainer ecc{};
    // Use Forward Declared class
    class FlexEccContainer* ecc = nullptr;

    void configure_flex();
    void execute_flex(const buffer_f32_t& audio);

    // Internal FLEX logic
    int flex_build_symbol(double sample);
    void flex_demodulate_sample(double sample);
    void flex_sym(unsigned char sym);
    unsigned int flex_sync_check(uint64_t buf);
    unsigned int flex_sync_process(unsigned char sym);
    void flex_decode_mode(unsigned int sync_code);
    void flex_read_2fsk(unsigned int sym, uint32_t* dat);
    int flex_decode_fiw();
    int flex_read_data(unsigned char sym);
    void flex_decode_data();
    void flex_decode_phase(char PhaseNo);
    int flex_bch_fix_errors(uint32_t* data_to_fix);

    // Parsing
    void flex_parse_capcode(uint32_t aw1);
    void flex_parse_alphanumeric(uint32_t* phaseptr, char PhaseNo, int mw1, int mw2, int flex_groupmessage);
    void flex_parse_numeric(uint32_t* phaseptr, char PhaseNo, int j);
    void flex_parse_tone_only(uint32_t* phaseptr, char PhaseNo, int j);

    void flex_send_packet(const flex::FlexPacket& packet);
    void flex_send_debug(const char* text, uint32_t v1, uint32_t v2);

    // Threads must be last members so everything else is initialized first.
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};
};

#endif /*__PROC_PAGER_H__*/


