#ifndef __PROC_WPAN_H__
#define __PROC_WPAN_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"
#include "dsp_demodulate.hpp"
#include "clock_recovery.hpp"
#include "message.hpp"

#include <cstdint>
#include <array>
#include <algorithm>

class WPANProcessor : public BasebandProcessor {
   public:
    WPANProcessor();
    void execute(const buffer_c8_t& buffer) override;

   private:
    static constexpr size_t baseband_fs = 4000000;
    static constexpr float chip_rate = 2000000.0f;

    // --- JAVÍTVA: 2048 méret a 4MHz-es buffer túlcsordulás ellen! ---
    std::array<complex16_t, 2048> c16_buf{};
    std::array<int16_t, 2048> fm_buf{};

    dsp::demodulate::FM demod_fm{};

    clock_recovery::ClockRecovery<clock_recovery::FixedErrorFilter> clock_recovery{
        baseband_fs,
        chip_rate,
        {0.02f},
        [this](const float chip) { this->consume_chip(chip); }};

    uint32_t chip_shift_reg{0};
    uint8_t chip_countdown{0};

    enum class SyncState { UNSYNCED,
                           EXPECT_SFD,
                           EXPECT_LENGTH,
                           EXPECT_PAYLOAD };
    SyncState sync_state{SyncState::UNSYNCED};

    uint8_t preamble_count{0};
    bool sfd_half{false};
    bool byte_half{false};
    uint8_t current_byte{0};
    uint8_t payload_length{0};
    uint8_t payload_idx{0};
    uint8_t payload_buffer[128];

    static constexpr uint32_t SYMBOL_TABLE[16] = {
        0xD9C3522E, 0xED9C3522, 0x2ED9C352, 0x22ED9C35,
        0x522ED9C3, 0x3522ED9C, 0xC3522ED9, 0x9C3522ED,
        0x263CAD51, 0x1263CAD5, 0xD1263CAD, 0xDD1263CA,
        0xADD1263C, 0xCADD1263, 0x3CADD126, 0x63CADD12};

    void consume_chip(float chip_soft);
    void correlate_symbol();
    void process_symbol(uint8_t symbol);

    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive, false};
    RSSIThread rssi_thread{};
};

#endif  // __PROC_WPAN_H__