#include "proc_pager.hpp"
#include "event_m4.hpp"
#include "audio_dma.hpp"
#include "dsp_fir_taps.hpp"
#include "portapack_shared_memory.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>

// Constants for FLEX
#define FREQ_SAMP_FLEX       24000
#define DC_OFFSET_FILTER     0.010 
#define PHASE_LOCKED_RATE    0.045
#define PHASE_UNLOCKED_RATE  0.050
#define LOCK_LEN             24
#define IDLE_THRESHOLD       0
#define DEMOD_TIMEOUT        100
#define FLEX_SYNC_MARKER     0xA6C6AAAAul
#define SLICE_THRESHOLD      0.667

using namespace std;
using namespace pocsag;

// --- FlexEccContainer Implementation for FLEX ---
// Copied from proc_flex.cpp to avoid linking issues

class FlexEccContainer {
   public:
    FlexEccContainer();
    int error_correct(uint32_t& val);

   private:
    uint32_t ecs[32];
    uint32_t bch[1024];
    void setup_ecc();
};

FlexEccContainer::FlexEccContainer() {
    setup_ecc();
}

void FlexEccContainer::setup_ecc() {
    unsigned int srr = 0x3b4;
    unsigned int i, n, j, k;

    for (i = 0; i <= 20; i++) {
        ecs[i] = srr;
        if ((srr & 0x01) != 0)
            srr = (srr >> 1) ^ 0x3B4;
        else
            srr = srr >> 1;
    }

    for (i = 0; i < 1024; i++) bch[i] = 0;

    for (n = 0; n <= 20; n++) {
        for (i = 0; i <= 20; i++) {
            j = (i << 5) + n;
            k = ecs[n] ^ ecs[i];
            bch[k] = j + 0x2000;
        }
    }

    for (n = 0; n <= 20; n++) {
        k = ecs[n];
        j = n + (0x1f << 5);
        bch[k] = j + 0x1000;
    }

    for (n = 0; n <= 20; n++) {
        for (i = 0; i < 10; i++) {
            k = ecs[n] ^ (1 << i);
            j = n + (0x1f << 5);
            bch[k] = j + 0x2000;
        }
    }

    for (n = 0; n < 10; n++) {
        k = 1 << n;
        bch[k] = 0x3ff + 0x1000;
    }

    for (n = 0; n < 10; n++) {
        for (i = 0; i < 10; i++) {
            if (i != n) {
                k = (1 << n) ^ (1 << i);
                bch[k] = 0x3ff + 0x2000;
            }
        }
    }
}

int FlexEccContainer::error_correct(uint32_t& val) {
    int i, synd, errl, acc, pari, ecc, b1, b2;

    errl = 0;
    pari = 0;

    ecc = 0;
    for (i = 31; i >= 11; --i) {
        if (val & (1 << i)) {
            ecc = ecc ^ ecs[31 - i];
            pari = pari ^ 0x01;
        }
    }

    acc = 0;
    for (i = 10; i >= 1; --i) {
        acc = acc << 1;
        if (val & (1 << i)) {
            acc = acc ^ 0x01;
        }
    }

    synd = ecc ^ acc;
    errl = 0;

    if (synd != 0) {
        if (bch[synd] != 0) {
            b1 = bch[synd] & 0x1f;
            b2 = bch[synd] >> 5;
            b2 = b2 & 0x1f;

            if (b2 != 0x1f) {
                val ^= 0x01 << (31 - b2);
                ecc = ecc ^ ecs[b2];
            }

            if (b1 != 0x1f) {
                val ^= 0x01 << (31 - b1);
                ecc = ecc ^ ecs[b1];
            }

            errl = bch[synd] >> 12;
        } else {
            errl = 3;
        }

        if (errl == 1) pari = pari ^ 0x01;
    }

    if (errl == 4) errl = 3;

    return errl;
}

// --- POCSAG Helpers ---

namespace {
/* Count of bits that differ between the two values. */
uint8_t diff_bit_count(uint32_t left, uint32_t right) {
    uint32_t diff = left ^ right;
    uint8_t count = 0;
    for (size_t i = 0; i < sizeof(diff) * 8; ++i) {
        if (((diff >> i) & 0x1) == 1)
            ++count;
    }
    return count;
}

// FLEX Helpers
unsigned int popcount(unsigned int n) {
    n = n - ((n >> 1) & 0x55555555);
    n = (n & 0x33333333) + ((n >> 2) & 0x33333333);
    return (((n + (n >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

uint32_t bit_reverse_32(uint32_t x) {
    x = ((x >> 1) & 0x55555555) | ((x & 0x55555555) << 1);
    x = ((x >> 2) & 0x33333333) | ((x & 0x33333333) << 2);
    x = ((x >> 4) & 0x0F0F0F0F) | ((x & 0x0F0F0F0F) << 4);
    x = ((x >> 8) & 0x00FF00FF) | ((x & 0x00FF00FF) << 8);
    x = (x >> 16) | (x << 16);
    return x;
}

}  // namespace

// --- AudioNormalizer ---

void AudioNormalizer::execute_in_place(const buffer_f32_t& audio) {
    if (counter_ >= 24'000) {
        max_ *= 0.9f;
        min_ *= 0.9f;
        counter_ = 0;
        calculate_thresholds();
    }

    counter_ += audio.count;

    for (size_t i = 0; i < audio.count; ++i) {
        auto& val = audio.p[i];

        if (val > max_) {
            max_ = val;
            calculate_thresholds();
        }
        if (val < min_) {
            min_ = val;
            calculate_thresholds();
        }

        if (val >= t_hi_)
            val = 1.0f;
        else if (val <= t_lo_)
            val = -1.0f;
        else
            val = 0.0;
    }
}

void AudioNormalizer::calculate_thresholds() {
    auto center = (max_ + min_) / 2.0f;
    auto range = (max_ - min_) / 2.0f;
    auto threshold = range * 0.1;
    t_hi_ = center + threshold;
    t_lo_ = center - threshold;
}

// --- BitQueue ---

void BitQueue::push(bool bit) {
    data_ = (data_ << 1) | (bit ? 1 : 0);
    if (count_ < max_size_) ++count_;
}

bool BitQueue::pop() {
    if (count_ == 0) return false;
    --count_;
    return (data_ & (1 << count_)) != 0;
}

void BitQueue::reset() {
    data_ = 0;
    count_ = 0;
}

uint8_t BitQueue::size() const {
    return count_;
}

uint32_t BitQueue::data() const {
    return data_;
}

// --- BitExtractor ---

void BitExtractor::extract_bits(const buffer_f32_t& audio) {
    for (size_t i = 0; i < audio.count; ++i) {
        auto sample = audio.p[i];

        if (current_rate_) {
            if (current_rate_->handle_sample(sample)) {
                auto value = (current_rate_->bits.data() & 1) == 1;
                bits_.push(value);
            }
        } else {
            for (auto& rate : known_rates_) {
                if (rate.handle_sample(sample) &&
                    diff_bit_count(rate.bits.data(), clock_magic_number) <= 3) {
                    rate.is_stable = true;
                    current_rate_ = &rate;
                }
            }
        }
    }
}

void BitExtractor::configure(uint32_t sample_rate) {
    sample_rate_ = sample_rate;
    for (auto& rate : known_rates_)
        rate.sample_interval = sample_rate / (2.0 * rate.baud_rate);

    if (baud_config_ >= 0 && baud_config_ < static_cast<int8_t>(known_rates_.size())) {
        current_rate_ = &known_rates_[baud_config_];
    } else {
        current_rate_ = nullptr;
    }
}

void BitExtractor::reset() {
    for (auto& rate : known_rates_)
        rate.reset();

    if (baud_config_ >= 0 && baud_config_ < static_cast<int8_t>(known_rates_.size())) {
        current_rate_ = &known_rates_[baud_config_];
    } else {
        current_rate_ = nullptr;
    }
}

void BitExtractor::set_baud_config(int8_t baud_config) {
    baud_config_ = baud_config;
}

uint16_t BitExtractor::baud_rate() const {
    return current_rate_ ? current_rate_->baud_rate : 0;
}

bool BitExtractor::RateInfo::handle_sample(float sample) {
    samples_until_next -= 1;
    if (samples_until_next > 0) return false;

    bool value = signbit(sample);
    bool bit_pushed = false;

    switch (state) {
        case State::WaitForSample:
            state = State::ReadyToSend;
            break;

        case State::ReadyToSend:
            if (!is_stable && prev_value != value) {
                samples_until_next += (sample_interval / 8.0);
            } else {
                state = State::WaitForSample;
                bit_pushed = true;
                bits.push(value);
            }
            break;
    }

    samples_until_next += sample_interval;
    prev_value = value;
    return bit_pushed;
}

void BitExtractor::RateInfo::reset() {
    state = State::WaitForSample;
    samples_until_next = 0.0;
    prev_value = false;
    is_stable = false;
    bits.reset();
}

// --- CodewordExtractor ---

void CodewordExtractor::process_bits() {
    while (bits_.size() > 0) {
        take_one_bit();

        if (bit_count_ < data_bit_count) continue;

        if (!has_sync_) {
            if (diff_bit_count(data_, sync_codeword) <= 2)
                handle_sync(false);
            else if (diff_bit_count(data_, ~sync_codeword) <= 2)
                handle_sync(true);
            continue;
        }

        save_current_codeword();

        if (word_count_ == pocsag::batch_size)
            handle_batch_complete();
    }
}

void CodewordExtractor::flush() {
    if (word_count_ == 0) return;
    pad_idle();
    handle_batch_complete();
}

void CodewordExtractor::reset() {
    clear_data_bits();
    has_sync_ = false;
    inverted_ = false;
    word_count_ = 0;
}

void CodewordExtractor::clear_data_bits() {
    data_ = 0;
    bit_count_ = 0;
}

void CodewordExtractor::take_one_bit() {
    data_ = (data_ << 1) | bits_.pop();
    if (bit_count_ < data_bit_count)
        ++bit_count_;
}

void CodewordExtractor::handle_sync(bool inverted) {
    clear_data_bits();
    has_sync_ = true;
    inverted_ = inverted;
    word_count_ = 0;
}

void CodewordExtractor::save_current_codeword() {
    batch_[word_count_++] = inverted_ ? ~data_ : data_;
    clear_data_bits();
}

void CodewordExtractor::handle_batch_complete() {
    if(on_batch_) on_batch_(*this);
    has_sync_ = false;
    word_count_ = 0;
}

void CodewordExtractor::pad_idle() {
    while (word_count_ < pocsag::batch_size)
        batch_[word_count_++] = idle_codeword;
}

// --- PagerProcessor ---

PagerProcessor::PagerProcessor() {
    ecc = new FlexEccContainer();
}

PagerProcessor::~PagerProcessor() {
    delete ecc;
}

void PagerProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    // Common decimation path
    // 3.072MHz -> 24kHz
    
    // decim_0: 3.072M -> 384k. Input buffer -> dst_buffer.
    auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    
    // decim_1: 384k -> 48k. dst_buffer -> intermediate_buffer.
    auto decim_1_out = decim_1.execute(decim_0_out, intermediate_buffer);
    
    // channel_filter: 48k -> 24k. intermediate_buffer -> dst_buffer (reuse).
    auto channel_out = channel_filter.execute(decim_1_out, dst_buffer);
    
    // demod: 24k -> audio (float). dst_buffer -> audio_buffer.
    auto audio = demod.execute(channel_out, audio_buffer);

    // Heartbeat debug similar to original FLEX-only processor
    // static int flex_debug_counter = 0;
    // if (++flex_debug_counter > 1000) {
    //     flex_send_debug("Running", 0, 0);
    //     flex_debug_counter = 0;
    // }

    if (mode == pager::Mode::AUTO) {
        // Try both - FLEX first since it needs raw audio
        // POCSAG modifies audio in-place, so FLEX must go first
        execute_flex(audio);
        execute_pocsag(audio);
    } else if (mode == pager::Mode::POCSAG) {
        execute_pocsag(audio);
    } else {
        execute_flex(audio);
    }
}

void PagerProcessor::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::POCSAGConfigure: {
            auto config = reinterpret_cast<const POCSAGConfigureMessage*>(message);
            
            // Let's overload POCSAGConfigure baud_config=-2 for AUTO
            if (config->baud_config == -2) {
                mode = pager::Mode::AUTO;
                // Configure generic defaults
                configure_pocsag(-1); // Auto-baud POCSAG
                configure_flex();     // Reset FLEX
                // We need to ensure DSP is configured compatibly.
                // Both use 24kHz out. 
                // POCSAG uses 4500 dev, FLEX 4800. Let's use 4800 for AUTO.
                demod.configure(24000, 4800); 
            } else {
                mode = pager::Mode::POCSAG;
                configure_pocsag(config->baud_config);
            }
            break;
        }
            
        case Message::ID::FlexConfigure:
            mode = pager::Mode::FLEX;
            configure_flex();
            break;

        case Message::ID::NBFMConfigure: {
            auto config = reinterpret_cast<const NBFMConfigureMessage*>(message);
            squelch.set_threshold(config->squelch_level / 99.0);
            break;
        }

        case Message::ID::AudioBeep:
            on_beep_message(*reinterpret_cast<const AudioBeepMessage*>(message));
            break;

        default:
            break;
    }
}

// --- POCSAG Implementation ---

void PagerProcessor::configure_pocsag(int8_t baud_config) {
    // POCSAG specific settings
    decim_0.configure(taps_16k0_decim_0.taps);
    decim_1.configure(taps_16k0_decim_1.taps);
    channel_filter.configure(taps_16k0_channel.taps, 2);
    demod.configure(24000, 4500); // FSK +/- 4.5kHz

    audio_output.configure(false);
    bit_extractor.set_baud_config(baud_config);
    bit_extractor.configure(24000); // demod_input_fs

    word_extractor.on_batch_ = [this](CodewordExtractor& extractor) {
        packet.set_flag(pocsag::PacketFlag::NORMAL);
        packet.set_timestamp(Timestamp::now());
        packet.set_bitrate(bit_extractor.baud_rate());
        packet.set(extractor.batch());
        send_packet_pocsag();
    };

    configured = true;
    samples_processed = 0;
    
    // Send debug or status if needed
}

void PagerProcessor::execute_pocsag(const buffer_f32_t& audio) {
    // If in AUTO mode, check if we should lock
    if (mode == pager::Mode::AUTO) {
        // If FLEX locked, skip POCSAG
        if (flex_demodulator.locked) return;
    }

    bool has_audio = squelch.execute(audio);
    squelch_history = (squelch_history << 1) | (has_audio ? 1 : 0);

    if (squelch_history == 0) {
        if (word_extractor.current() > 0) {
            flush_pocsag();
            reset_pocsag();
            send_stats_pocsag();
        }
        
        // Silence audio output
        for (size_t i = 0; i < audio.count; ++i) audio.p[i] = 0.0;
        audio_output.write(audio);
        return;
    }

    lpf.execute_in_place(audio);
    normalizer.execute_in_place(audio);
    audio_output.write(audio);

    bit_extractor.extract_bits(audio);
    word_extractor.process_bits();

    // If in AUTO mode and we detect POCSAG sync, we could lock or just keep decoding.
    // For now, let's just let it run. If it finds something, it emits packets.
    // Ideally we would set mode = POCSAG to save CPU on FLEX, but for now dual run is safer.
}

// Fixed logic for samples_processed in execute()
/*
    // Update the status.
    samples_processed += buffer.count;
    if (samples_processed >= stat_update_threshold) {
        send_stats();
        samples_processed -= stat_update_threshold;
    }
*/
// I will add this logic to PagerProcessor::execute() at the end.

void PagerProcessor::flush_pocsag() {
    word_extractor.flush();
}

void PagerProcessor::reset_pocsag() {
    bits.reset();
    bit_extractor.reset();
    word_extractor.reset();
    samples_processed = 0;
}

void PagerProcessor::send_stats_pocsag() const {
    POCSAGStatsMessage message(
        word_extractor.current(), word_extractor.count(),
        word_extractor.has_sync(), bit_extractor.baud_rate());
    shared_memory.application_queue.push(message);
}

void PagerProcessor::send_packet_pocsag() {
    POCSAGPacketMessage message(packet);
    shared_memory.application_queue.push(message);
}

void PagerProcessor::on_beep_message(const AudioBeepMessage& message) {
    audio::dma::beep_start(message.freq, message.sample_rate, message.duration_ms);
}


// --- FLEX Implementation ---

void PagerProcessor::configure_flex() {
    // Match the working FLEX-only processor DSP chain:
    // 3.072 MHz -> 384 kHz -> 48 kHz -> 24 kHz
    decim_0.configure(taps_11k0_decim_0.taps);
    decim_1.configure(taps_11k0_decim_1.taps);
    channel_filter.configure(taps_11k0_channel.taps, 2); // 48k -> 24k
    demod.configure(24000, 4800); // FSK +/- 4.8kHz for FLEX
    
    // Reset FLEX state
    flex_demodulator = {};
    flex_demodulator.sample_freq = 24000;
    flex_demodulator.baud = 1600;
    flex_state.Current = flex::State::SYNC1;
    
    // Initialize GroupHandler with -1
    for (int i = 0; i < 17; i++) {
        flex_group_handler.GroupFrame[i] = -1;
        flex_group_handler.GroupCycle[i] = -1;
        for (int j = 0; j < 100; j++) {
            flex_group_handler.GroupCodes[i][j] = 0;
        }
    }

    configured = true;
    flex_send_debug("FLEX Configured", 0, 0);
}

void PagerProcessor::execute_flex(const buffer_f32_t& audio) {
    if (mode == pager::Mode::AUTO) {
        // ...
    }

    float max_val = 0.0f;
    for (size_t i = 0; i < audio.count; ++i) {
        max_val = std::max(max_val, std::abs(audio.p[i]));
        flex_demodulate_sample(audio.p[i]);
    }

    static int signal_log_counter = 0;
    if (max_val > 0.1f) {
        if (++signal_log_counter > 3000) { // Approx 2 seconds
            // flex_send_debug("Signal Detected", (uint32_t)(max_val * 1000), 0);
            signal_log_counter = 0;
        }
    }
}

void PagerProcessor::flex_send_debug(const char* text, uint32_t v1, uint32_t v2) {
    FlexDebugMessage message(v1, v2, text);
    shared_memory.application_queue.push(message);
}

void PagerProcessor::flex_send_packet(const flex::FlexPacket& packet) {
    FlexPacketMessage message(packet);
    shared_memory.application_queue.push(message);
}

// Copied FLEX Logic

void PagerProcessor::flex_demodulate_sample(double sample) {
    if (flex_build_symbol(sample) == 1) {
        flex_demodulator.nonconsec = 0;
        flex_demodulator.symbol_count++;

        int j;
        int decmax = 0;
        int modal_symbol = 0;
        for (j = 0; j<4; j++) {
            if (flex_demodulator.symcount[j] > decmax) {
                modal_symbol = j;
                decmax = flex_demodulator.symcount[j];
            }
        }
        flex_demodulator.symcount[0] = 0;
        flex_demodulator.symcount[1] = 0;
        flex_demodulator.symcount[2] = 0;
        flex_demodulator.symcount[3] = 0;

        if (flex_demodulator.locked) {
            // Normal path: process symbol when already locked.
            flex_sym(modal_symbol);
        } else {
            // Try to acquire lock based on preamble pattern.
            flex_demodulator.lock_buf = (flex_demodulator.lock_buf << 2) | (modal_symbol ^ 0x1);
            uint64_t lock_pattern = flex_demodulator.lock_buf ^ 0x6666666666666666ull;
            uint64_t lock_mask = (1ull << (2 * LOCK_LEN)) - 1;
            
            if ((lock_pattern & lock_mask) == 0 || ((~lock_pattern) & lock_mask) == 0) {
                flex_demodulator.locked = 1;
                flex_demodulator.lock_buf = 0;
                flex_demodulator.symbol_count = 0;
                flex_demodulator.sample_count = 0;
                flex_send_debug("Lock", 0, 0);
            }
        }

        flex_demodulator.timeout++;
        if (flex_demodulator.timeout > DEMOD_TIMEOUT) {
            if (flex_demodulator.locked) {
                flex_send_debug("Timeout", flex_demodulator.timeout, 0);
            }
            flex_demodulator.locked = 0;
        }
    }
}

int PagerProcessor::flex_build_symbol(double sample) {
    const int64_t phase_max = 100 * flex_demodulator.sample_freq;
    const int64_t phase_rate = phase_max * flex_demodulator.baud / flex_demodulator.sample_freq;
    const double phasepercent = 100.0 * flex_demodulator.phase / phase_max;

    flex_demodulator.sample_count++;

    /*Remove DC offset (FIR filter)*/
    if (flex_state.Current == flex::State::SYNC1) {
        flex_modulation.zero = (flex_modulation.zero * (FREQ_SAMP_FLEX * DC_OFFSET_FILTER) + sample) / ((FREQ_SAMP_FLEX * DC_OFFSET_FILTER) + 1);
    }
    sample -= flex_modulation.zero;

    if (flex_demodulator.locked) {
        if (flex_state.Current == flex::State::SYNC1) {
            flex_demodulator.envelope_sum += std::abs(sample);
            flex_demodulator.envelope_count++;
            flex_modulation.envelope = flex_demodulator.envelope_sum / flex_demodulator.envelope_count;
        }
    } else {
        flex_modulation.envelope = 0;
        flex_demodulator.envelope_sum = 0;
        flex_demodulator.envelope_count = 0;
        flex_demodulator.baud = 1600;
        flex_demodulator.timeout = 0;
        flex_demodulator.nonconsec = 0;
        flex_state.Current = flex::State::SYNC1;
    }

    if (phasepercent > 10 && phasepercent < 90) {
        if (sample > 0) {
            if (sample > flex_modulation.envelope * SLICE_THRESHOLD)
                flex_demodulator.symcount[3]++;
            else
                flex_demodulator.symcount[2]++;
        } else {
            if (sample < -flex_modulation.envelope * SLICE_THRESHOLD)
                flex_demodulator.symcount[0]++;
            else
                flex_demodulator.symcount[1]++;
        }
    }

    if ((flex_demodulator.sample_last < 0 && sample >= 0) || (flex_demodulator.sample_last >= 0 && sample < 0)) {
        double phase_error = 0.0;
        if (phasepercent < 50) {
            phase_error = flex_demodulator.phase;
        } else {
            phase_error = flex_demodulator.phase - phase_max;
        }

        if (flex_demodulator.locked) {
            flex_demodulator.phase -= phase_error * PHASE_LOCKED_RATE;
        } else {
            flex_demodulator.phase -= phase_error * PHASE_UNLOCKED_RATE;
        }

        if (phasepercent > 10 && phasepercent < 90) {
            flex_demodulator.nonconsec++;
            if (flex_demodulator.nonconsec > 20 && flex_demodulator.locked) {
                flex_demodulator.locked = 0;
            }
        } else {
            flex_demodulator.nonconsec = 0;
        }

        flex_demodulator.timeout = 0;
    }
    flex_demodulator.sample_last = sample;

    flex_demodulator.phase += phase_rate;

    if (flex_state.Current == flex::State::DATA && flex_demodulator.symbol_count < 10) {
       // Debug symbol processing at start of DATA
       // flex_send_debug("Sym", flex_demodulator.symbol_count, (uint32_t)(sample*1000));
    }

    if (flex_demodulator.phase > phase_max) {
        flex_demodulator.phase -= phase_max;
        return 1;
    } else {
        return 0;
    }
}

unsigned int PagerProcessor::flex_sync_process(unsigned char sym) {
    int retval = 0;
    flex_sync.syncbuf = (flex_sync.syncbuf << 1) | ((sym < 2) ? 1 : 0);

    retval = flex_sync_check(flex_sync.syncbuf);
    if (retval != 0) {
        flex_sync.polarity = 0;
    } else {
        retval = flex_sync_check(~flex_sync.syncbuf);
        if (retval != 0) {
            flex_sync.polarity = 1;
        }
    }
    return retval;
}

unsigned int PagerProcessor::flex_sync_check(uint64_t buf) {
    unsigned int marker = (buf & 0x0000FFFFFFFF0000ULL) >> 16;
    unsigned short codehigh = (buf & 0xFFFF000000000000ULL) >> 48;
    unsigned short codelow = ~(buf & 0x000000000000FFFFULL);

    int retval = 0;
    unsigned int diff_marker = popcount(marker ^ FLEX_SYNC_MARKER);
    unsigned int diff_code = popcount(codelow ^ codehigh);

    // Occasionally report near-sync candidates to debug how close we get.
    // Only log when both distances are reasonably small, and rate-limit.
    if (diff_marker <= 6 && diff_code <= 6) {
        static int sync_debug_counter = 0;
        if (++sync_debug_counter > 5000) {  // roughly throttled
            uint32_t diffs = (diff_marker << 16) | diff_code;
            flex_send_debug("SyncCand", diffs, marker);
            sync_debug_counter = 0;
        }
    }

    if (diff_marker < 4 && diff_code < 4) {
        retval = codehigh;
    } else {
        retval = 0;
    }
    return retval;
}

void PagerProcessor::flex_decode_mode(unsigned int sync_code) {
    struct FlexModeDef {
        int sync;
        unsigned int baud;
        unsigned int levels;
    } flex_modes[] = {
        {0x870C, 1600, 2},
        {0xB068, 1600, 4},
        {0x7B18, 3200, 2},
        {0xDEA0, 3200, 4},
        {0x4C7C, 3200, 4},
        {0, 0, 0}
    };

    for (int i = 0; flex_modes[i].sync != 0; i++) {
        unsigned int diff = popcount((unsigned int)flex_modes[i].sync ^ sync_code);
        if (diff < 4) {
            flex_sync.sync = sync_code;
            flex_sync.baud = flex_modes[i].baud;
            flex_sync.levels = flex_modes[i].levels;
            return;
        }
    }
    flex_sync.baud = 1600;
    flex_sync.levels = 2;
}

void PagerProcessor::flex_read_2fsk(unsigned int sym, uint32_t* dat) {
    *dat = (*dat >> 1) | ((sym > 1) ? 0x80000000 : 0);
}

int PagerProcessor::flex_bch_fix_errors(uint32_t* data_to_fix) {
    if (!ecc) {
        // Should never happen, but safety check
        return 3;
    }
    uint32_t reversed = bit_reverse_32(*data_to_fix);
    int result = ecc->error_correct(reversed);
    if (result == 0 || result == 1 || result == 2) {
        *data_to_fix = bit_reverse_32(reversed);
    }
    return result;
}

int PagerProcessor::flex_decode_fiw() {
    uint32_t fiw_val = flex_fiw.rawdata;
    int decode_error = flex_bch_fix_errors(&fiw_val);

    if (decode_error > 2) { 
        return 1;
    }

    flex_fiw.checksum = fiw_val & 0xF;
    flex_fiw.cycleno = (fiw_val >> 4) & 0xF;
    flex_fiw.frameno = (fiw_val >> 8) & 0x7F;
    flex_fiw.fix3 = (fiw_val >> 15) & 0x3F;

    unsigned int checksum = (fiw_val & 0xF);
    checksum += ((fiw_val >> 4) & 0xF);
    checksum += ((fiw_val >> 8) & 0xF);
    checksum += ((fiw_val >> 12) & 0xF);
    checksum += ((fiw_val >> 16) & 0xF);
    checksum += ((fiw_val >> 20) & 0x01);
    checksum &= 0xF;

    if (checksum == 0xF) {
        return 0;
    } else {
        return 1;
    }
}

int PagerProcessor::flex_read_data(unsigned char sym) {
    int bit_a = (sym > 1);
    int bit_b = 0;
    if (flex_sync.levels == 4) {
        bit_b = (sym == 1) || (sym == 2);
    }

    if (flex_sync.baud == 1600) {
        flex_data.phase_toggle = 0;
    }

    unsigned int idx = ((flex_data.data_bit_counter >> 5) & 0xFFF8) | (flex_data.data_bit_counter & 0x0007);
    if (idx >= 88) return 0;

    if (flex_data.phase_toggle == 0) {
        flex_data.PhaseA.buf[idx] = (flex_data.PhaseA.buf[idx] >> 1) | (bit_a ? 0x80000000 : 0);
        flex_data.PhaseB.buf[idx] = (flex_data.PhaseB.buf[idx] >> 1) | (bit_b ? 0x80000000 : 0);
        flex_data.phase_toggle = 1;

        if ((flex_data.data_bit_counter & 0xFF) == 0xFF) {
            if (flex_data.PhaseA.buf[idx] == 0x00000000 || flex_data.PhaseA.buf[idx] == 0xffffffff) flex_data.PhaseA.idle_count++;
            if (flex_data.PhaseB.buf[idx] == 0x00000000 || flex_data.PhaseB.buf[idx] == 0xffffffff) flex_data.PhaseB.idle_count++;
        }
    } else {
        flex_data.PhaseC.buf[idx] = (flex_data.PhaseC.buf[idx] >> 1) | (bit_a ? 0x80000000 : 0);
        flex_data.PhaseD.buf[idx] = (flex_data.PhaseD.buf[idx] >> 1) | (bit_b ? 0x80000000 : 0);
        flex_data.phase_toggle = 0;

        if ((flex_data.data_bit_counter & 0xFF) == 0xFF) {
            if (flex_data.PhaseC.buf[idx] == 0x00000000 || flex_data.PhaseC.buf[idx] == 0xffffffff) flex_data.PhaseC.idle_count++;
            if (flex_data.PhaseD.buf[idx] == 0x00000000 || flex_data.PhaseD.buf[idx] == 0xffffffff) flex_data.PhaseD.idle_count++;
        }
    }

    // Increment counter: always for 1600 baud, or when phase_toggle becomes 0 (after writing C/D)
    if (flex_sync.baud == 1600 || flex_data.phase_toggle == 0) {
        flex_data.data_bit_counter++;
    }

    int idle = 0;
    if (flex_sync.baud == 1600) {
        if (flex_sync.levels == 2) {
            idle = (flex_data.PhaseA.idle_count > IDLE_THRESHOLD);
        } else {
            idle = ((flex_data.PhaseA.idle_count > IDLE_THRESHOLD) && (flex_data.PhaseB.idle_count > IDLE_THRESHOLD));
        }
    } else {
        if (flex_sync.levels == 2) {
            idle = ((flex_data.PhaseA.idle_count > IDLE_THRESHOLD) && (flex_data.PhaseC.idle_count > IDLE_THRESHOLD));
        } else {
            idle = ((flex_data.PhaseA.idle_count > IDLE_THRESHOLD) && (flex_data.PhaseB.idle_count > IDLE_THRESHOLD) && (flex_data.PhaseC.idle_count > IDLE_THRESHOLD) && (flex_data.PhaseD.idle_count > IDLE_THRESHOLD));
        }
    }
    return idle;
}

void PagerProcessor::flex_sym(unsigned char sym) {
    unsigned char sym_rectified;
    if (flex_sync.polarity) {
        sym_rectified = 3 - sym;
    } else {
        sym_rectified = sym;
    }

    switch (flex_state.Current) {
        case flex::State::SYNC1: {
            unsigned int sync_code = flex_sync_process(sym);
            if (sync_code != 0) {
                flex_decode_mode(sync_code);
                if (flex_sync.baud != 0 && flex_sync.levels != 0) {
                    flex_state.Current = flex::State::FIW;
                    flex_send_debug("SYNC1 Found", flex_sync.baud, sync_code);
                } else {
                    flex_state.Current = flex::State::SYNC1;
                }
            } else {
                flex_state.Current = flex::State::SYNC1;
            }
            flex_state.fiwcount = 0;
            flex_fiw.rawdata = 0;
            break;
        }
        case flex::State::FIW: {
            flex_state.fiwcount++;
            if (flex_state.fiwcount >= 16) {
                flex_read_2fsk(sym_rectified, &flex_fiw.rawdata);
            }
            if (flex_state.fiwcount == 48) {
                if (flex_decode_fiw() == 0) {
                    flex_state.sync2_count = 0;
                    flex_demodulator.baud = flex_sync.baud;
                    flex_state.Current = flex::State::SYNC2;
                    flex_send_debug("FIW OK", flex_fiw.frameno, flex_fiw.cycleno);
                } else {
                    flex_state.Current = flex::State::SYNC1;
                    flex_send_debug("FIW Fail", flex_fiw.rawdata, 0);
                }
            }
            break;
        }
        case flex::State::SYNC2: {
            if (++flex_state.sync2_count == flex_sync.baud * 25 / 1000) {
                flex_state.data_count = 0;
                for (int i=0; i<88; i++) {
                    flex_data.PhaseA.buf[i]=0; flex_data.PhaseB.buf[i]=0;
                    flex_data.PhaseC.buf[i]=0; flex_data.PhaseD.buf[i]=0;
                }
                flex_data.PhaseA.idle_count=0; flex_data.PhaseB.idle_count=0;
                flex_data.PhaseC.idle_count=0; flex_data.PhaseD.idle_count=0;
                flex_data.phase_toggle=0;
                flex_data.data_bit_counter=0;

                flex_state.Current = flex::State::DATA;
            }
            break;
        }
        case flex::State::DATA: {
            int idle = flex_read_data(sym_rectified);
            if (++flex_state.data_count == flex_sync.baud * 1760 / 1000 || idle) {
                flex_decode_data();
                flex_demodulator.baud = 1600;
                flex_state.Current = flex::State::SYNC1;
                flex_state.data_count = 0;
            }
            break;
        }
    }
}

void PagerProcessor::flex_decode_data() {
    if (flex_sync.baud == 1600) {
        if (flex_sync.levels == 2) {
            flex_decode_phase('A');
        } else {
            flex_decode_phase('A');
            flex_decode_phase('B');
        }
    } else {
        if (flex_sync.levels == 2) {
            flex_decode_phase('A');
            flex_decode_phase('C');
        } else {
            flex_decode_phase('A');
            flex_decode_phase('B');
            flex_decode_phase('C');
            flex_decode_phase('D');
        }
    }
}

void PagerProcessor::flex_decode_phase(char PhaseNo) {
    uint32_t *phaseptr = nullptr;
    switch (PhaseNo) {
        case 'A': phaseptr = flex_data.PhaseA.buf; break;
        case 'B': phaseptr = flex_data.PhaseB.buf; break;
        case 'C': phaseptr = flex_data.PhaseC.buf; break;
        case 'D': phaseptr = flex_data.PhaseD.buf; break;
        default: return;
    }

    for (int i = 0; i < 88; i++) {
        int decode_error = flex_bch_fix_errors(&phaseptr[i]);
        if (decode_error > 2) return;
        phaseptr[i] &= 0x001FFFFF; 
    }
    
    uint32_t biw = phaseptr[0];
    if (biw == 0 || biw == 0x001FFFFF) return;

    int voffset = (biw >> 10) & 0x3f;
    int aoffset = ((biw >> 8) & 0x03) + 1;

    for (int i = aoffset; i < voffset; i++) {
        int j = voffset + i - aoffset;
        if (phaseptr[i] == 0x00000000 || phaseptr[i] == 0x001FFFFF) continue;

        flex_parse_capcode(phaseptr[i]);
        if (flex_decode.long_address) continue; 

        if (flex_decode.capcode > 4297068542ll || flex_decode.capcode < 0) continue;

        uint32_t viw = phaseptr[j];
        int type_val = (viw >> 4) & 0x07;
        
        switch(type_val) {
            case 0: flex_decode.type = flex::PageType::SECURE; break;
            case 1: flex_decode.type = flex::PageType::SHORT_INSTRUCTION; break;
            case 2: flex_decode.type = flex::PageType::TONE; break;
            case 3: flex_decode.type = flex::PageType::STANDARD_NUMERIC; break;
            case 4: flex_decode.type = flex::PageType::SPECIAL_NUMERIC; break;
            case 5: flex_decode.type = flex::PageType::ALPHANUMERIC; break;
            case 6: flex_decode.type = flex::PageType::BINARY; break;
            case 7: flex_decode.type = flex::PageType::NUMBERED_NUMERIC; break;
        }

        int mw1 = (viw >> 7) & 0x7F;
        int len = (viw >> 14) & 0x7F;
        int mw2 = mw1 + (len - 1);

        if (mw1 == 0 && mw2 == 0) continue;
        if (flex_decode.type == flex::PageType::TONE) mw1 = mw2 = 0;

        if (flex_decode.type == flex::PageType::ALPHANUMERIC || flex_decode.type == flex::PageType::SECURE) {
            if (mw1 > 87 || mw2 > 87) continue;
            flex_parse_alphanumeric(phaseptr, PhaseNo, mw1, mw2, 0);
        } else if (flex_decode.type == flex::PageType::STANDARD_NUMERIC || flex_decode.type == flex::PageType::SPECIAL_NUMERIC || flex_decode.type == flex::PageType::NUMBERED_NUMERIC) {
            flex_parse_numeric(phaseptr, PhaseNo, j);
        } else if (flex_decode.type == flex::PageType::TONE) {
            flex_parse_tone_only(phaseptr, PhaseNo, j);
        }
    }
}

void PagerProcessor::flex_parse_capcode(uint32_t aw1) {
    flex_decode.long_address = (aw1 < 0x008001L) || (aw1 > 0x1E0000L) || (aw1 > 0x1E7FFEL);
    flex_decode.capcode = aw1 - 0x8000;
}

void PagerProcessor::flex_parse_alphanumeric(uint32_t * phaseptr, char, int mw1, int mw2, int) {
    char message[128] = {0}; 
    int currentChar = 0;
    
    mw1++;
    
    for (int i = mw1; i <= mw2; i++) {
        unsigned int dw = phaseptr[i];
        unsigned char ch;

        if (i > mw1) {
            ch = dw & 0x7F;
            if (ch != 0x03 && currentChar < 127) message[currentChar++] = ch;
        }
        
        ch = (dw >> 7) & 0x7F;
        if (ch != 0x03 && currentChar < 127) message[currentChar++] = ch;
        
        ch = (dw >> 14) & 0x7F;
        if (ch != 0x03 && currentChar < 127) message[currentChar++] = ch;
    }
    message[currentChar] = '\0';

    flex::FlexPacket packet;
    packet.bitrate = flex_sync.baud;
    packet.capcode = flex_decode.capcode;
    packet.function = 0;
    packet.type = 5; 
    packet.status = 0; 
    strncpy(packet.message, message, sizeof(packet.message) - 1);
    
    flex_send_packet(packet);
}

void PagerProcessor::flex_parse_numeric(uint32_t * phaseptr, char, int j) {
    char message[128] = {0};
    const char flex_bcd[] = "0123456789 U -][";
    
    int w1 = phaseptr[j] >> 7;
    int w2 = w1 >> 7;
    w1 = w1 & 0x7f;
    w2 = (w2 & 0x07) + w1;

    int dw;
    dw = phaseptr[w1];
    w1++; w2++;

    unsigned char digit = 0;
    int count = 4;
    if (flex_decode.type == flex::PageType::NUMBERED_NUMERIC) count += 10;
    else count += 2;

    int idx = 0;
    for (int i = w1; i <= w2; i++) {
        for (int k = 0; k < 21; k++) {
            digit = (digit >> 1) & 0x0F;
            if (dw & 0x01) digit ^= 0x08;
            dw >>= 1;
            if (--count == 0) {
                if (digit != 0x0C && idx < 127) {
                    message[idx++] = flex_bcd[digit];
                }
                count = 4;
            }
        }
        dw = phaseptr[i];
    }
    message[idx] = '\0';

    flex::FlexPacket packet;
    packet.bitrate = flex_sync.baud;
    packet.capcode = flex_decode.capcode;
    packet.function = 0;
    packet.type = 3; 
    packet.status = 0;
    strncpy(packet.message, message, sizeof(packet.message) - 1);
    
    flex_send_packet(packet);
}

void PagerProcessor::flex_parse_tone_only(uint32_t *, char, int) {
    flex::FlexPacket packet;
    packet.bitrate = flex_sync.baud;
    packet.capcode = flex_decode.capcode;
    packet.function = 0;
    packet.type = 2; 
    packet.status = 0;
    snprintf(packet.message, sizeof(packet.message), "Tone Only");
    
    flex_send_packet(packet);
}


int main() {
    audio::dma::init_audio_out();
    EventDispatcher event_dispatcher{std::make_unique<PagerProcessor>()};
    event_dispatcher.run();
    return 0;
}


