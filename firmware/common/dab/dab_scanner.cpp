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

#include "dab_scanner.hpp"

namespace dab {

void Scanner::start(const Config& config) {
    config_ = config;
    block_count_ = 0;
    cursor_ = 0;
    candidate_count_ = 0;
    result_count_ = 0;

    const auto biii = band_iii_blocks();
    for (size_t i = 0; i < biii.count && block_count_ < kMaxScanBlocks; ++i)
        blocks_[block_count_++] = &biii.items[i];

    if (config_.include_l_band) {
        const auto lb = l_band_blocks();
        for (size_t i = 0; i < lb.count && block_count_ < kMaxScanBlocks; ++i)
            blocks_[block_count_++] = &lb.items[i];
    }

    for (size_t i = 0; i < block_count_; ++i)
        rssi_[i] = 0;

    if (block_count_ == 0) {
        finish();
        return;
    }

    if (config_.deep) {
        for (size_t i = 0; i < block_count_; ++i)
            candidates_[candidate_count_++] = i;
        begin_harvest();
        return;
    }

    phase_ = Phase::Prescan;
}

void Scanner::abort() {
    phase_ = Phase::Idle;
    cursor_ = 0;
    candidate_count_ = 0;
}

const Block* Scanner::current_block() const {
    switch (phase_) {
        case Phase::Prescan:
            return (cursor_ < block_count_) ? blocks_[cursor_] : nullptr;
        case Phase::Harvest:
            return (cursor_ < candidate_count_) ? blocks_[candidates_[cursor_]] : nullptr;
        default:
            return nullptr;
    }
}

uint32_t Scanner::current_dwell_ms() const {
    switch (phase_) {
        case Phase::Prescan:
            return kDefaultPrescanMs;
        case Phase::Harvest:
            return config_.harvest_ms;
        default:
            return 0;
    }
}

void Scanner::report_rssi(uint8_t rssi) {
    if (phase_ != Phase::Prescan || cursor_ >= block_count_)
        return;

    rssi_[cursor_] = rssi;
    if (rssi >= config_.rssi_threshold)
        candidates_[candidate_count_++] = cursor_;

    ++cursor_;

    if (cursor_ >= block_count_)
        begin_harvest();
}

void Scanner::begin_harvest() {
    cursor_ = 0;
    if (candidate_count_ == 0) {
        finish();
        return;
    }
    phase_ = Phase::Harvest;
}

void Scanner::report_harvest(bool ensemble_found, bool complete, uint32_t elapsed_ms) {
    if (phase_ != Phase::Harvest || cursor_ >= candidate_count_)
        return;

    const bool dwell_expired = elapsed_ms >= config_.harvest_ms;
    if (!complete && !dwell_expired)
        return;

    if (ensemble_found && result_count_ < kMaxScanBlocks) {
        const size_t index = candidates_[cursor_];
        results_[result_count_].block = blocks_[index];
        results_[result_count_].rssi = rssi_[index];
        results_[result_count_].ensemble_found = true;
        ++result_count_;
    }

    ++cursor_;
    if (cursor_ >= candidate_count_)
        finish();
}

void Scanner::finish() {
    phase_ = Phase::Done;
    cursor_ = 0;
}

uint8_t Scanner::progress_percent() const {
    switch (phase_) {
        case Phase::Idle:
            return 0;
        case Phase::Done:
            return 100;
        case Phase::Prescan:
            /* The prescan is cheap: the first 30 percent. */
            if (block_count_ == 0)
                return 0;
            return static_cast<uint8_t>((cursor_ * 30) / block_count_);
        case Phase::Harvest:
            if (candidate_count_ == 0)
                return 30;
            return static_cast<uint8_t>(30 + (cursor_ * 70) / candidate_count_);
    }
    return 0;
}

} /* namespace dab */
