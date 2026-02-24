#ifndef __PROC_FLEX_H__
#define __PROC_FLEX_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"
#include "message.hpp"
#include "flex_defs.hpp"
#include "pocsag.hpp"  // For EccContainer

#include <cstdint>
#include <array>

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
    int64_t GroupCodes[17][100];  // Reduced size from 1000 to save RAM
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

class FlexProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    bool configured{false};

    // DSP components
    // 3.072MHz -> 24kHz (Decim 128)
    // decim_0: 8, decim_1: 8, channel: 2. Total 128.
    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0_iq{};
    dsp::decimate::FIRC16xR16x32Decim8 decim_1_iq{};
    dsp::decimate::FIRAndDecimateComplex channel_filter{};
    dsp::demodulate::FM demod{};

    // Buffers
    std::array<complex16_t, 256> dst{};
    const buffer_c16_t dst_buffer{dst.data(), dst.size()};

    std::array<float, 16> audio{};
    const buffer_f32_t audio_buffer{audio.data(), audio.size()};

    // Flex State
    flex::FlexDemodParams demodulator{};
    flex::FlexModulation modulation{};
    flex::FlexStateInfo state{};
    flex::FlexSync sync{};
    flex::FlexFIW fiw{};
    flex::FlexData data{};
    flex::FlexDecode decode{};
    flex::FlexGroupHandler group_handler{};

    pocsag::EccContainer ecc{};

    // Methods
    void configure();
    void process_audio(const buffer_f32_t& audio);

    // Internal Flex logic
    int build_symbol(double sample);
    void flex_demodulate(double sample);
    void flex_sym(unsigned char sym);
    unsigned int flex_sync_check(uint64_t buf);
    unsigned int flex_sync(unsigned char sym);
    void decode_mode(unsigned int sync_code);
    void read_2fsk(unsigned int sym, uint32_t* dat);  // Changed to uint32_t*
    int decode_fiw();
    int read_data(unsigned char sym);
    void decode_data();
    void decode_phase(char PhaseNo);
    int bch_fix_errors(uint32_t* data_to_fix);

    // Parsing
    void parse_capcode(uint32_t aw1);
    void parse_alphanumeric(uint32_t* phaseptr, char PhaseNo, int mw1, int mw2, int flex_groupmessage);
    void parse_numeric(uint32_t* phaseptr, char PhaseNo, int j);
    void parse_tone_only(uint32_t* phaseptr, char PhaseNo, int j);
    void parse_unknown(uint32_t* phaseptr, char PhaseNo, int mw1, int mw2);

    void send_packet(const flex::FlexPacket& packet);
    void send_stats();
    void send_debug(const char* text, uint32_t v1, uint32_t v2);

    // Threads
    BasebandThread baseband_thread{3072000, this, baseband::Direction::Receive};
};

#endif /*__PROC_FLEX_H__*/
