/*
 * Copyright (C) 2026 Mayhem firmware contributors
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

/* Ensemble search sequencing: a power prescan keeps blocks above a threshold,
 * then a harvest dwells on each until its configuration is complete or the
 * dwell expires. Deep search harvests every block. Sequencing only: the caller
 * tunes current_block() and reports back, so this is testable on the host. */

#ifndef __DAB_SCANNER_H__
#define __DAB_SCANNER_H__

#include "dab_blocks.hpp"

#include <cstddef>
#include <cstdint>

namespace dab {

constexpr size_t kMaxScanBlocks = 64;
constexpr uint32_t kDefaultHarvestMs = 3000;
constexpr uint32_t kDefaultPrescanMs = 50;

class Scanner {
   public:
    enum class Phase : uint8_t {
        Idle,
        Prescan,
        Harvest,
        Done,
    };

    struct Config {
        bool include_l_band{false};
        bool deep{false};
        uint8_t rssi_threshold{40};
        uint32_t harvest_ms{kDefaultHarvestMs};
    };

    struct Result {
        const Block* block{nullptr};
        uint8_t rssi{0};
        bool ensemble_found{false};
    };

    void start(const Config& config);
    void abort();

    Phase phase() const { return phase_; }

    /* nullptr when idle or done. */
    const Block* current_block() const;

    uint32_t current_dwell_ms() const;

    /* Prescan: wideband power of current_block(). */
    void report_rssi(uint8_t rssi);

    /* `complete` (configuration fully decoded) ends the dwell early;
     * otherwise the block advances once elapsed_ms reaches the dwell. */
    void report_harvest(bool ensemble_found, bool complete, uint32_t elapsed_ms);

    /* Only blocks where an ensemble was decoded. */
    const Result* results() const { return results_; }
    size_t result_count() const { return result_count_; }

    uint8_t progress_percent() const;

   private:
    void begin_harvest();
    void finish();

    Config config_{};
    Phase phase_{Phase::Idle};

    const Block* blocks_[kMaxScanBlocks]{};
    uint8_t rssi_[kMaxScanBlocks]{};
    size_t block_count_{0};
    size_t cursor_{0};

    size_t candidates_[kMaxScanBlocks]{};
    size_t candidate_count_{0};

    Result results_[kMaxScanBlocks]{};
    size_t result_count_{0};
};

} /* namespace dab */

#endif /*__DAB_SCANNER_H__*/
