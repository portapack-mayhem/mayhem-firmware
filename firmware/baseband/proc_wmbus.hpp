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
    void on_message(const Message* const message) override;

   private:
    static constexpr size_t baseband_fs = 1000000;

    int32_t i_dc_acc{0};
    int32_t q_dc_acc{0};

    int32_t i_sum{0};
    int32_t q_sum{0};
    int16_t i_hist[4]{0};
    int16_t q_hist[4]{0};
    uint8_t iq_idx{0};

    int16_t prev_i{0};
    int16_t prev_q{0};
    int32_t fm_sum{0};
    int32_t fm_hist[4]{0};
    uint8_t fm_idx{0};

    int32_t fm_dc_avg{0};
    uint32_t phase{0};
    bool last_bit{false};

    uint8_t op_mode{0};
    uint32_t chip_reg{0};
    uint8_t chip_count{0};
    bool inverted_iq{false};

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

    uint32_t telemetry_counter{0};
    uint32_t peak_fm_val{0};
    uint32_t stat_syncs{0};
    uint32_t stat_errors{0};
    uint8_t last_err_reason{0};
    uint8_t last_err_data{0};

    uint8_t decode_3out6(uint8_t chips);
    void consume_chip(uint8_t bit);
    void handle_byte(uint8_t byte);

    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive, false};
    RSSIThread rssi_thread{};
};

#endif  // __PROC_WMBUS_H__