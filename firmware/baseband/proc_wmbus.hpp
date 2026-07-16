#ifndef __PROC_WMBUS_H__
#define __PROC_WMBUS_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"
#include "message.hpp"

#include <cstdint>

class WMBusProcessor : public BasebandProcessor {
   public:
    WMBusProcessor();
    void execute(const buffer_c8_t& buffer) override;

   private:
    static constexpr size_t baseband_fs = 1000000;

    // FM és DC Blocker
    int16_t prev_i{0};
    int16_t prev_q{0};
    int32_t fm_hist[8]{};
    uint8_t hist_idx{0};
    int32_t fm_sum{0};
    int32_t dc_acc{0};

    // --- AZ RTL_WMBUS "RUN-LENGTH" ALGORITMUS VÁLTOZÓI (PI CONTROLLER) ---
    uint8_t op_mode{0};
    uint32_t raw_bitstream{0};
    uint8_t current_state{0};

    int32_t run_length{0};
    int32_t bit_length{2560};  // A chip hossza felskálázva 256-tal (PI Célpont)
    int32_t target_bit_length{2560};
    int32_t cum_run_length_error{0};
    // --------------------------------------------------------------------

    // MAC réteg
    uint32_t chip_reg{0};
    uint8_t chip_count{0};
    bool inverted_iq{false};
    bool byte_half{false};

    enum class SyncState { UNSYNCED = 0,
                           READ_LEN_H = 1,
                           READ_LEN_L = 2,
                           READ_DATA_H = 3,
                           READ_DATA_L = 4 };
    SyncState sync_state{SyncState::UNSYNCED};

    uint8_t current_byte{0};
    uint8_t payload_length{0};
    uint8_t physical_length{0};
    uint8_t payload_idx{0};
    uint8_t payload_buffer[255];

    // Telemetria
    uint32_t telemetry_counter{0};
    int32_t peak_fm_val{0};
    uint32_t stat_syncs{0};
    uint32_t stat_errors{0};

    uint8_t decode_3out6(uint8_t chips);
    void consume_chip(uint8_t bit);
    void handle_byte(uint8_t byte);

    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive, false};
    RSSIThread rssi_thread{};
};

#endif  // __PROC_WMBUS_H__