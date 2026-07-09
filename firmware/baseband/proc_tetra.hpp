#ifndef __PROC_TETRA_H__
#define __PROC_TETRA_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "message.hpp"
#include "dsp_decimate.hpp"
#include "dsp_fir_taps.hpp"
#include "rssi_thread.hpp"

#include <cstdint>
#include <array>
#include <memory>

class TetraProcessor : public BasebandProcessor {
   public:
    TetraProcessor();

    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const msg);

   private:
    static constexpr size_t baseband_fs = 3072000;
    static constexpr size_t channel_fs = 48000;
    static constexpr uint32_t symbol_rate = 18000;

    std::array<complex16_t, 512> dst_0{};
    const buffer_c16_t dst_buffer_0{dst_0.data(), dst_0.size()};

    std::array<complex16_t, 512> dst_1{};
    const buffer_c16_t dst_buffer_1{dst_1.data(), dst_1.size()};

    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0{};
    dsp::decimate::FIRC16xR16x32Decim8 decim_1{};

    // PLL and Timing
    uint32_t symbol_phase{0};
    uint32_t symbol_phase_inc{(uint32_t)((symbol_rate * 4294967296.0) / channel_fs)};

    complex16_t prompt_sample{0, 0};
    complex16_t prev_prompt{0, 0};
    complex16_t mid_sample{0, 0};
    complex16_t delay_sample{0, 0};
    static constexpr uint32_t BURST_BITS = 500;
    static constexpr uint32_t Y_SYNC_BITS = 38;
    static constexpr uint32_t SYNC_OFFSET = 214;
    static constexpr uint64_t Y_SYNC = 0x30673A7067ULL;
    static constexpr uint32_t DNB_TRAIN_BITS = 22;
    static constexpr uint32_t DNB_BLK_BITS = 216;
    static constexpr uint32_t DNB_TRAIN_TO_BLK2 = 22;
    static constexpr uint32_t DNB_TOTAL_T5_BITS = DNB_BLK_BITS * 2;
    static constexpr uint32_t N_SYNC = 0x343A74UL;
    static constexpr uint32_t P_SYNC = 0x1E90DEUL;
    static constexpr bool ENABLE_DNB_MESSAGES = true;

    uint32_t pll_phase{0};
    int32_t pll_freq{0};
    static constexpr int32_t pll_alpha = 150;
    static constexpr int32_t pll_beta = 2;

    uint64_t sync_register{0};
    std::array<uint8_t, 128> bit_history_buffer{};
    size_t history_write_idx{0};
    uint64_t bit_count{0};
    bool configured{false};

    struct PendingDnb {
        bool valid{false};
        uint64_t block1_start{0};
        uint64_t block2_start{0};
        uint64_t ready_at{0};
        bool inverted{false};
        bool p_train{false};
        uint8_t errors{0};
    } pending_dnb{};

    struct PendingDsb {
        bool valid{false};
        uint64_t burst_start{0};
        uint64_t ready_at{0};
        bool inverted{false};
        uint8_t errors{0};
    } pending_dsb{};

    void process_symbol(const complex16_t& sample);
    uint8_t history_bit(uint64_t absolute_bit) const;
    void push_burst_to_ui();
    void push_dnb_to_ui();

    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive, false};
    RSSIThread rssi_thread{};
};

#endif  // __PROC_TETRA_H__