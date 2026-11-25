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

#include "doctest.h"

#include "dab/dab_deinterleave.hpp"

#include <cstdint>
#include <cstdlib>
#include <array>
#include <vector>

namespace {

/* Transmitter-side interleaver, mirroring the receiver under test. */
class TimeInterleaver {
   public:
    explicit TimeInterleaver(size_t bits_per_cif)
        : bits_per_cif_{bits_per_cif},
          per_branch_{bits_per_cif / 16} {
        size_t running = 0;
        for (uint8_t j = 0; j < 16; ++j) {
            depth_[j] = dab::TimeDeinterleaver::branch_delay(j);
            offset_[j] = running;
            running += static_cast<size_t>(depth_[j]) * per_branch_;
        }
        storage_.assign(running, 0);
    }

    void push(const int8_t* in, int8_t* out) {
        for (size_t i = 0; i < bits_per_cif_; ++i) {
            const uint8_t j = static_cast<uint8_t>(i % 16);
            const size_t slot = i / 16;
            if (depth_[j] == 0) {
                out[i] = in[i];
                continue;
            }
            const size_t base = offset_[j] + head_[j] * per_branch_ + slot;
            out[i] = storage_[base];
            storage_[base] = in[i];
        }
        for (uint8_t j = 0; j < 16; ++j)
            if (depth_[j] != 0)
                head_[j] = (head_[j] + 1) % depth_[j];
    }

   private:
    size_t bits_per_cif_;
    size_t per_branch_;
    uint8_t depth_[16]{};
    size_t offset_[16]{};
    size_t head_[16]{};
    std::vector<int8_t> storage_;
};

/* Trellis levels, -3..+3, which the packed store carries exactly. */
int8_t pattern(size_t cif, size_t index) {
    return static_cast<int8_t>(((cif * 31 + index * 5) % 7) - 3);
}

/* Levels to a packed code run (dab_soft.hpp), two a byte, low nibble first. */
std::vector<uint8_t> pack(const std::vector<int8_t>& levels) {
    std::vector<uint8_t> run(levels.size() / 2, 0);
    for (size_t i = 0; i < levels.size(); ++i)
        run[i / 2] |= static_cast<uint8_t>((static_cast<uint8_t>(levels[i]) & 0x0Fu) << (4 * (i & 1)));
    return run;
}

int32_t level_at(const std::vector<uint8_t>& run, size_t i) {
    return dab::soft_level(dab::soft_get(run.data(), i));
}

/* Interleaves kCifs CIFs of `bits` through tx and the deinterleaver under test,
 * checking every output after the pipeline fills against its source CIF. */
void round_trip(size_t bits, size_t cifs) {
    TimeInterleaver tx{bits};
    dab::TimeDeinterleaver rx;
    std::vector<uint8_t> store(dab::TimeDeinterleaver::storage_bytes_for(bits));
    REQUIRE(rx.configure(bits, store.data(), store.size()));

    std::vector<int8_t> in(bits), mid(bits);
    for (size_t cif = 0; cif < cifs; ++cif) {
        for (size_t i = 0; i < bits; ++i)
            in[i] = pattern(cif, i);
        tx.push(in.data(), mid.data());
        auto run = pack(mid);
        const bool ready = rx.push_packed(run.data());

        if (cif < dab::kInterleaverLatencyCifs) {
            CHECK_FALSE(ready);
        } else {
            REQUIRE(ready);
            // Output corresponds to the CIF pushed 15 steps earlier.
            const size_t source_cif = cif - dab::kInterleaverLatencyCifs;
            for (size_t i = 0; i < bits; ++i)
                REQUIRE(level_at(run, i) == pattern(source_cif, i));
        }
    }
}

}  // namespace

TEST_CASE("branch delays are the four-bit reversal") {
    const uint8_t expected[16] = {0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15};
    for (uint8_t j = 0; j < 16; ++j)
        CHECK(dab::TimeDeinterleaver::branch_delay(j) == expected[j]);
}

TEST_CASE("branch delays are a permutation of 0..15") {
    bool seen[16]{};
    for (uint8_t j = 0; j < 16; ++j)
        seen[dab::TimeDeinterleaver::branch_delay(j)] = true;
    for (bool s : seen)
        CHECK(s);
}

TEST_CASE("configure rejects invalid sizes") {
    dab::TimeDeinterleaver d;
    std::vector<uint8_t> store(dab::TimeDeinterleaver::storage_bytes_for(dab::kMaxCodedBitsPerCif));

    CHECK_FALSE(d.configure(0, store.data(), store.size()));
    CHECK_FALSE(d.configure(100, store.data(), store.size()));  // not a multiple of 16
    CHECK_FALSE(d.configure(dab::kMaxCodedBitsPerCif + 16, store.data(), store.size()));
    CHECK(d.configure(3072, store.data(), store.size()));
    CHECK(d.bits_per_cif() == 3072);
}

TEST_CASE("configure rejects storage that is too small or missing") {
    dab::TimeDeinterleaver d;
    std::vector<uint8_t> store(dab::TimeDeinterleaver::storage_bytes_for(3072));

    CHECK_FALSE(d.configure(3072, nullptr, store.size()));
    CHECK_FALSE(d.configure(3072, store.data(), store.size() - 1));
    CHECK(d.configure(3072, store.data(), store.size()));
}

TEST_CASE("storage size matches the branch-sum formula") {
    // 7.5 codes per bit at three bits each.
    CHECK(dab::TimeDeinterleaver::storage_bytes_for(1024) == 1024 * 15 / 2 * 3 / 8);
    CHECK(dab::TimeDeinterleaver::storage_bytes_for(4608) == 12960);
    // A branch row is padded to whole groups of eight slots: 65 slots take 72.
    CHECK(dab::TimeDeinterleaver::storage_bytes_for(65 * 16) == 72 * 120 / 8 * 3);
}

TEST_CASE("interleave then deinterleave restores the data after 15 CIFs") {
    round_trip(1024, 40);
}

TEST_CASE("works at the maximum supported subchannel size") {
    round_trip(dab::kMaxCodedBitsPerCif, 20);
}

TEST_CASE("a branch row that is not whole groups of eight restores the data") {
    // 13 slots a branch: one full group and a partial one.
    round_trip(13 * 16, 25);
}

TEST_CASE("reset clears the pipeline") {
    constexpr size_t kBits = 256;
    dab::TimeDeinterleaver rx;
    std::vector<uint8_t> store(dab::TimeDeinterleaver::storage_bytes_for(kBits));
    REQUIRE(rx.configure(kBits, store.data(), store.size()));

    std::vector<uint8_t> run(kBits / 2, 0x11);
    for (size_t i = 0; i < 20; ++i)
        rx.push_packed(run.data());

    rx.reset();
    CHECK_FALSE(rx.push_packed(run.data()));
}

TEST_CASE("push rejects a null buffer") {
    dab::TimeDeinterleaver rx;
    std::vector<uint8_t> store(dab::TimeDeinterleaver::storage_bytes_for(256));
    REQUIRE(rx.configure(256, store.data(), store.size()));
    CHECK_FALSE(rx.push_packed(nullptr));
}

TEST_CASE("push on an unconfigured deinterleaver is refused") {
    dab::TimeDeinterleaver rx;
    std::vector<uint8_t> run(128);
    CHECK_FALSE(rx.push_packed(run.data()));
}
