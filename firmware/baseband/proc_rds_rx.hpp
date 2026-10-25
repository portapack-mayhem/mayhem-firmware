#ifndef __PROC_RDS_RX_H__
#define __PROC_RDS_RX_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"
#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"
#include "clock_recovery.hpp"
#include "message.hpp"

#include <cstdint>
#include <array>

class RDSProcessor : public BasebandProcessor {
   public:
    RDSProcessor();
    void execute(const buffer_c8_t& buffer) override;

   private:
    static constexpr size_t baseband_fs = 3072000;
    static constexpr size_t mpx_fs = 384000;
    static constexpr size_t rds_fs = 48000;
    static constexpr float rds_symbolrate = 2375.0f;

    std::array<complex16_t, 512> dst_0{};
    const buffer_c16_t dst_buffer_0{dst_0.data(), dst_0.size()};

    std::array<float, 512> mpx_audio{};
    const buffer_f32_t mpx_buffer{mpx_audio.data(), mpx_audio.size()};

    std::array<complex16_t, 512> mixed{};
    std::array<complex16_t, 512> dst_1{};
    const buffer_c16_t dst_buffer_1{dst_1.data(), dst_1.size()};

    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0{};
    dsp::demodulate::FM demod_fm{};
    dsp::decimate::FIRC16xR16x32Decim8 decim_1{};

    uint32_t nco_phase{0};
    uint32_t nco_inc{0};

    float i_f1{0.0f}, q_f1{0.0f};
    float i_f2{0.0f}, q_f2{0.0f};
    float i_f3{0.0f}, q_f3{0.0f};

    float costas_phase{0.0f};
    float costas_freq{0.0f};
    static constexpr float costas_alpha = 0.005f;
    static constexpr float costas_beta = 0.00001f;
    uint32_t unsynced_bits{0};

    clock_recovery::ClockRecovery<clock_recovery::FixedErrorFilter> clock_recovery{
        rds_fs,
        rds_symbolrate,
        {0.01f},
        [this](const float symbol) { this->consume_symbol(symbol); }};

    bool biphase_clock{false};
    uint8_t first_half_sym{0};
    uint8_t prev_sym{0};

    enum class SyncState { UNSYNCED = 0,
                           EXPECT_B = 1,
                           EXPECT_C = 2,
                           EXPECT_D = 3,
                           EXPECT_A = 4 };
    SyncState sync_state{SyncState::UNSYNCED};

    uint32_t bit_history{0};
    uint8_t bits_counted{0};
    uint32_t total_bits{0};

    uint16_t block_a{0};
    uint16_t block_b{0};
    uint16_t block_c{0};
    uint16_t block_d{0};
    bool is_c_prime{false};

    static constexpr uint16_t SYNDROME_A = 0x00FC;
    static constexpr uint16_t SYNDROME_B = 0x0198;
    static constexpr uint16_t SYNDROME_C = 0x0168;
    static constexpr uint16_t SYNDROME_Cp = 0x0350;
    static constexpr uint16_t SYNDROME_D = 0x01B4;

    void consume_symbol(const float symbol);
    void process_bit(uint8_t bit);
    uint16_t calc_syndrome(uint32_t reg);

    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive, false};
    RSSIThread rssi_thread{};
};

#endif  // __PROC_RDS_RX_H__