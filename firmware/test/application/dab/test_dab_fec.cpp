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

#include "dab/dab_fec.hpp"
#include "dab/dab_common.hpp"

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <array>
#include <vector>

/* ---- Energy dispersal ---- */

TEST_CASE("prbs matches the reference sequence") {
    // The generator emits the feedback bit, not the bit shifted out. Emitting
    // the latter gives the same m-sequence delayed by nine bits: it still has
    // period 511 and 256 ones, so only a known vector catches the mistake.
    dab::PrbsGenerator g;
    CHECK(g.next_byte() == 0x07);
    CHECK(g.next_byte() == 0xBE);
    CHECK(g.next_byte() == 0x2E);
    CHECK(g.next_byte() == 0x64);
}

TEST_CASE("prbs opens with five zeros then three ones") {
    dab::PrbsGenerator g;
    for (int i = 0; i < 5; ++i)
        CHECK(g.next_bit() == 0);
    for (int i = 0; i < 3; ++i)
        CHECK(g.next_bit() == 1);
}

TEST_CASE("prbs sequence period is 511 bits") {
    dab::PrbsGenerator g;
    uint8_t first[511];
    for (auto& b : first)
        b = g.next_bit();

    for (size_t i = 0; i < 511; ++i)
        CHECK(g.next_bit() == first[i]);
}

TEST_CASE("prbs is not a constant sequence") {
    // Guards against a generator that never shifts.
    dab::PrbsGenerator g;
    int ones = 0;
    for (int i = 0; i < 511; ++i)
        ones += g.next_bit();

    // A maximal-length 9-bit LFSR emits 256 ones per period.
    CHECK(ones == 256);
}

TEST_CASE("descrambling is self inverse") {
    uint8_t data[64];
    uint8_t original[64];
    for (size_t i = 0; i < sizeof(data); ++i)
        data[i] = original[i] = static_cast<uint8_t>(i * 11 + 5);

    dab::prbs_descramble(data, sizeof(data));
    CHECK(std::memcmp(data, original, sizeof(data)) != 0);

    dab::prbs_descramble(data, sizeof(data));
    CHECK(std::memcmp(data, original, sizeof(data)) == 0);
}

TEST_CASE("descrambling an all-zero buffer yields the raw sequence") {
    uint8_t data[4]{};
    dab::prbs_descramble(data, sizeof(data));
    CHECK(data[0] == 0x07);
    CHECK(data[1] == 0xBE);
}

TEST_CASE("descramble restarts the sequence on every call") {
    uint8_t a[8]{};
    uint8_t b[8]{};
    dab::prbs_descramble(a, sizeof(a));
    dab::prbs_descramble(b, sizeof(b));
    CHECK(std::memcmp(a, b, sizeof(a)) == 0);
}

/* ---- Time interleaving ---- */

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

/* Levels to a packed code run (dab_fec.hpp), two a byte, low nibble first. */
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

/* ---- FIC depuncturing ---- */

TEST_CASE("FIC sizes match the OFDM capacity they must fill") {
    // Three FIC symbols carry 3 * 1536 * 2 = 9216 bits per frame, split
    // across four codewords of 2304.
    CHECK(dab::kFicPuncturedBits * dab::kFicCodewordsPerFrame == 3 * 1536 * 2);
    CHECK(dab::kFicPuncturedBits == 2304);
    // Mother code is rate 1/4 over payload plus the 6-bit tail.
    CHECK(dab::kFicMotherBits == 4 * (dab::kFicPayloadBits + 6));
    // Four codewords carry the frame's 12 FIBs.
    CHECK(dab::kFicCodewordsPerFrame * 3 == 12);
}

TEST_CASE("the puncturing schedule consumes exactly the transmitted bits") {
    // 21 blocks of PI_16, 3 of PI_15, then a 12-bit tail.
    const size_t pi16_kept = dab::puncturing_weight(16);
    const size_t pi15_kept = dab::puncturing_weight(15);

    CHECK(pi16_kept == 24);
    CHECK(pi15_kept == 23);

    const size_t total = 21 * 4 * pi16_kept + 3 * 4 * pi15_kept + 12;
    CHECK(total == dab::kFicPuncturedBits);
}

TEST_CASE("depuncturing expands to the full mother codeword") {
    std::vector<int8_t> punctured(dab::kFicPuncturedBits, 64);
    std::vector<int8_t> out(dab::kFicMotherBits, 0);

    CHECK(dab::depuncture_fic(punctured.data(), out.data(), out.size()) == dab::kFicMotherBits);
}

TEST_CASE("depunctured erasures land exactly where bits were removed") {
    std::vector<int8_t> punctured(dab::kFicPuncturedBits, 64);
    std::vector<int8_t> out(dab::kFicMotherBits, 0);
    dab::depuncture_fic(punctured.data(), out.data(), out.size());

    size_t kept = 0;
    for (int8_t v : out)
        if (v != 0)
            ++kept;

    // Every transmitted bit must appear exactly once.
    CHECK(kept == dab::kFicPuncturedBits);
}

TEST_CASE("a FIC codeword survives encode, puncture and decode") {
    // Build a payload, encode at rate 1/4, puncture with the FIC schedule,
    // then depuncture and decode. This exercises the schedule against the
    // Viterbi decoder rather than only counting bits.
    std::vector<uint8_t> payload((dab::kFicPayloadBits + 7) / 8);
    uint32_t x = 12345;
    for (auto& b : payload) {
        x = x * 1103515245u + 12345u;
        b = static_cast<uint8_t>(x >> 16);
    }

    std::vector<uint8_t> coded(dab::kFicMotherBits);
    REQUIRE(dab::convolutional_encode(payload.data(), dab::kFicPayloadBits, coded.data()) == dab::kFicMotherBits);

    // Puncture: keep the bits the schedule keeps, in order.
    std::vector<int8_t> mother_soft(dab::kFicMotherBits);
    for (size_t i = 0; i < dab::kFicMotherBits; ++i)
        mother_soft[i] = coded[i] ? 64 : -64;

    // Derive the keep-mask by depuncturing a marker sequence.
    std::vector<int8_t> ones(dab::kFicPuncturedBits, 1);
    std::vector<int8_t> mask(dab::kFicMotherBits, 0);
    dab::depuncture_fic(ones.data(), mask.data(), mask.size());

    std::vector<int8_t> punctured;
    punctured.reserve(dab::kFicPuncturedBits);
    for (size_t i = 0; i < dab::kFicMotherBits; ++i)
        if (mask[i] != 0)
            punctured.push_back(mother_soft[i]);

    REQUIRE(punctured.size() == dab::kFicPuncturedBits);

    // Through the packed depuncturer the receiver runs, which fetches words.
    punctured.resize(dab::kFicPuncturedBits + 4, 0);
    std::vector<dab::ViterbiDecoder::Soft> restored(dab::kFicMotherBits / 4, 0);
    REQUIRE(dab::depuncture_fic_packed(punctured.data(), restored.data(), restored.size()) == dab::kFicMotherBits);

    dab::ViterbiDecoder decoder;
    std::vector<uint8_t> decoded((dab::kFicPayloadBits + 7) / 8, 0);
    REQUIRE(decoder.decode(restored.data(), dab::kFicMotherBits, decoded.data(), decoded.size()) == dab::kFicPayloadBits);

    for (size_t i = 0; i < payload.size(); ++i)
        REQUIRE(decoded[i] == payload[i]);
}

TEST_CASE("depuncturing rejects bad arguments") {
    std::vector<int8_t> punctured(dab::kFicPuncturedBits, 1);
    std::vector<int8_t> out(dab::kFicMotherBits, 0);

    CHECK(dab::depuncture_fic(nullptr, out.data(), out.size()) == 0);
    CHECK(dab::depuncture_fic(punctured.data(), nullptr, out.size()) == 0);
    CHECK(dab::depuncture_fic(punctured.data(), out.data(), 100) == 0);
}

/* ---- MSC subchannels ---- */

using dab::ProtectionKind;
using dab::ProtectionProfile;

namespace {

ProtectionProfile eep(uint8_t level, bool option_b) {
    return ProtectionProfile{ProtectionKind::Eep, level, option_b};
}

}  // namespace

TEST_CASE("the canonical 128 kbit/s EEP 3-A service resolves") {
    const auto s = dab::msc_schedule(96, eep(3, false));
    REQUIRE(s.valid);

    CHECK(s.payload_bits == 3072);
    CHECK(s.punctured_bits == 96 * 64);
    CHECK(s.mother_bits == 4 * (3072 + 6));

    // 96 blocks split 93/3 with PI_8 and PI_7, as the standard specifies.
    CHECK(s.l1 == 93);
    CHECK(s.l2 == 3);
    CHECK(s.pi1 == 8);
    CHECK(s.pi2 == 7);
}

TEST_CASE("EEP 1-A resolves to the rate 1/4 schedule") {
    const auto s = dab::msc_schedule(96, eep(1, false));
    REQUIRE(s.valid);
    CHECK(s.payload_bits == 1536);
    CHECK(s.l1 == 45);
    CHECK(s.l2 == 3);
    CHECK(s.pi1 == 24);
    CHECK(s.pi2 == 23);
}

TEST_CASE("EEP 2-A resolves to the rate 3/8 schedule") {
    const auto s = dab::msc_schedule(96, eep(2, false));
    REQUIRE(s.valid);
    CHECK(s.payload_bits == 2304);
    CHECK(s.l1 == 21);
    CHECK(s.l2 == 51);
    CHECK(s.pi1 == 14);
    CHECK(s.pi2 == 13);
}

TEST_CASE("every valid subchannel and profile produces a balanced schedule") {
    // The schedule must reproduce the allocated capacity exactly, for every
    // size the profile admits, in both options.
    size_t checked = 0;

    for (int option = 0; option < 2; option++) {
        for (uint8_t level = 1; level <= 4; level++) {
            const auto profile = eep(level, option != 0);
            const uint16_t per_unit = dab::eep_cu_per_unit(profile);
            REQUIRE(per_unit > 0);

            for (uint16_t units = 1; units * per_unit <= 864; units++) {
                const uint16_t size_cu = static_cast<uint16_t>(units * per_unit);
                const auto s = dab::msc_schedule(size_cu, profile);
                if (!s.valid)
                    continue;

                const uint32_t kept =
                    4 * (s.l1 * dab::puncturing_weight(s.pi1) + s.l2 * dab::puncturing_weight(s.pi2)) + 12;
                REQUIRE(kept == static_cast<uint32_t>(size_cu) * 64);
                REQUIRE(s.l1 + s.l2 == s.payload_bits / 32);
                REQUIRE(s.pi1 == s.pi2 + 1);
                REQUIRE(s.pi1 >= 1);
                REQUIRE(s.pi1 <= 24);
                ++checked;
            }
        }
    }

    CHECK(checked > 100);
}

TEST_CASE("a size that does not fit the profile is rejected") {
    // 95 CU is not a whole number of EEP 3-A units.
    CHECK_FALSE(dab::msc_schedule(95, eep(3, false)).valid);
    CHECK_FALSE(dab::msc_schedule(0, eep(3, false)).valid);
    CHECK_FALSE(dab::msc_schedule(96, eep(9, false)).valid);
}

TEST_CASE("UEP subchannels are not handled here") {
    CHECK_FALSE(dab::msc_schedule(96, ProtectionProfile{ProtectionKind::Uep, 3, false}).valid);
}

TEST_CASE("depuncturing consumes the capacity and fills the mother codeword") {
    const auto s = dab::msc_schedule(96, eep(3, false));
    REQUIRE(s.valid);

    std::vector<int8_t> punctured(s.punctured_bits, 64);
    std::vector<int8_t> out(s.mother_bits, 0);

    CHECK(dab::depuncture_msc(s, punctured.data(), out.data(), out.size()) == s.mother_bits);

    size_t kept = 0;
    for (int8_t v : out)
        if (v != 0)
            ++kept;
    CHECK(kept == s.punctured_bits);
}

TEST_CASE("an MSC codeword survives encode, puncture and decode") {
    // The widest subchannel the receiver decodes: 72 CU, 96 kbit/s at EEP 3-A.
    const auto s = dab::msc_schedule(72, eep(3, false));
    REQUIRE(s.valid);
    REQUIRE(s.payload_bits <= dab::kViterbiMaxPayloadBits);

    std::vector<uint8_t> payload(s.payload_bits / 8);
    uint32_t x = 777;
    for (auto& b : payload) {
        x = x * 1103515245u + 12345u;
        b = static_cast<uint8_t>(x >> 16);
    }

    std::vector<uint8_t> coded(s.mother_bits);
    REQUIRE(dab::convolutional_encode(payload.data(), s.payload_bits, coded.data()) == s.mother_bits);

    // Derive the keep-mask by depuncturing a marker, then puncture with it.
    std::vector<int8_t> ones(s.punctured_bits, 1);
    std::vector<int8_t> mask(s.mother_bits, 0);
    dab::depuncture_msc(s, ones.data(), mask.data(), mask.size());

    std::vector<int8_t> punctured;
    punctured.reserve(s.punctured_bits);
    for (size_t i = 0; i < s.mother_bits; i++)
        if (mask[i] != 0)
            punctured.push_back(coded[i] ? 64 : -64);

    REQUIRE(punctured.size() == s.punctured_bits);

    // As the receiver carries it: packed codes, readable four bytes past the
    // end, into one step word per four mother bits.
    std::vector<uint8_t> codes(s.punctured_bits / 2 + 4, 0);
    dab::pack_codes(punctured.data(), codes.data(), punctured.size());
    std::vector<dab::ViterbiDecoder::Soft> restored(s.mother_bits / 4, 0);
    REQUIRE(dab::depuncture_msc_packed(s, codes.data(), restored.data(), restored.size()) == s.mother_bits);

    dab::ViterbiDecoder decoder;
    std::vector<uint8_t> decoded(payload.size(), 0);
    REQUIRE(decoder.decode(restored.data(), s.mother_bits, decoded.data(), decoded.size()) == s.payload_bits);

    for (size_t i = 0; i < payload.size(); i++)
        REQUIRE(decoded[i] == payload[i]);
}

TEST_CASE("depuncturing rejects bad arguments") {
    const auto s = dab::msc_schedule(96, eep(3, false));
    std::vector<int8_t> punctured(s.punctured_bits, 1);
    std::vector<int8_t> out(s.mother_bits, 0);

    CHECK(dab::depuncture_msc(s, nullptr, out.data(), out.size()) == 0);
    CHECK(dab::depuncture_msc(s, punctured.data(), nullptr, out.size()) == 0);
    CHECK(dab::depuncture_msc(s, punctured.data(), out.data(), 10) == 0);
    CHECK(dab::depuncture_msc(dab::MscSchedule{}, punctured.data(), out.data(), out.size()) == 0);
}

/* ---- Viterbi decoder ---- */

namespace {

constexpr size_t kPayloadBits = 200;

std::vector<uint8_t> make_payload(size_t bits, uint32_t seed) {
    std::vector<uint8_t> data((bits + 7) / 8, 0);
    uint32_t x = seed;
    for (auto& b : data) {
        x = x * 1103515245u + 12345u;
        b = static_cast<uint8_t>(x >> 16);
    }
    return data;
}

std::vector<int8_t> to_soft(const std::vector<uint8_t>& coded_bits, int8_t confidence) {
    std::vector<int8_t> soft(coded_bits.size());
    for (size_t i = 0; i < coded_bits.size(); ++i)
        soft[i] = coded_bits[i] ? confidence : static_cast<int8_t>(-confidence);
    return soft;
}

/* The trellis input: one step word per four soft values, reduced to codes
 * (dab_fec.hpp), the step's first value in the low nibble. */
std::vector<dab::ViterbiDecoder::Soft> to_steps(const std::vector<int8_t>& soft) {
    std::vector<dab::ViterbiDecoder::Soft> steps(soft.size() / 4, 0);
    for (size_t i = 0; i < steps.size() * 4; ++i)
        steps[i / 4] = static_cast<dab::ViterbiDecoder::Soft>(steps[i / 4] | (dab::soft_code(soft[i]) << (4 * (i % 4))));
    return steps;
}

size_t bit_errors(const uint8_t* a, const uint8_t* b, size_t bits) {
    size_t errors = 0;
    for (size_t i = 0; i < bits; ++i) {
        const uint8_t x = (a[i >> 3] >> (7 - (i & 7))) & 1;
        const uint8_t y = (b[i >> 3] >> (7 - (i & 7))) & 1;
        if (x != y)
            ++errors;
    }
    return errors;
}

}  // namespace

TEST_CASE("generator polynomials produce the documented outputs") {
    // All-zero register must produce all-zero output.
    CHECK(dab::convolutional_output(0) == 0);
    // 133 and the fourth generator are identical, so bit 3 and bit 0 always agree.
    for (int reg = 0; reg < 128; ++reg) {
        const uint8_t out = dab::convolutional_output(static_cast<uint8_t>(reg));
        CHECK(((out >> 3) & 1) == (out & 1));
    }
}

TEST_CASE("encoder emits four bits per input bit plus the flush") {
    auto payload = make_payload(kPayloadBits, 1);
    std::vector<uint8_t> coded(4 * (kPayloadBits + 6));
    const size_t written = dab::convolutional_encode(payload.data(), kPayloadBits, coded.data());
    CHECK(written == 4 * (kPayloadBits + 6));
}

TEST_CASE("clean round trip is bit exact") {
    auto payload = make_payload(kPayloadBits, 42);
    std::vector<uint8_t> coded(4 * (kPayloadBits + 6));
    dab::convolutional_encode(payload.data(), kPayloadBits, coded.data());

    auto soft = to_soft(coded, 64);

    dab::ViterbiDecoder decoder;
    std::vector<uint8_t> out((kPayloadBits + 7) / 8, 0);
    const size_t decoded = decoder.decode(to_steps(soft).data(), soft.size(), out.data(), out.size());

    REQUIRE(decoded == kPayloadBits);
    CHECK(bit_errors(payload.data(), out.data(), kPayloadBits) == 0);
}

TEST_CASE("corrects scattered bit errors") {
    auto payload = make_payload(kPayloadBits, 7);
    std::vector<uint8_t> coded(4 * (kPayloadBits + 6));
    dab::convolutional_encode(payload.data(), kPayloadBits, coded.data());

    auto soft = to_soft(coded, 64);
    // Flip three well-separated coded bits; a K=7 rate-1/4 code absorbs these.
    soft[40] = static_cast<int8_t>(-soft[40]);
    soft[300] = static_cast<int8_t>(-soft[300]);
    soft[600] = static_cast<int8_t>(-soft[600]);

    dab::ViterbiDecoder decoder;
    std::vector<uint8_t> out((kPayloadBits + 7) / 8, 0);
    const size_t decoded = decoder.decode(to_steps(soft).data(), soft.size(), out.data(), out.size());

    REQUIRE(decoded == kPayloadBits);
    CHECK(bit_errors(payload.data(), out.data(), kPayloadBits) == 0);
}

TEST_CASE("erasures from depuncturing are tolerated") {
    auto payload = make_payload(kPayloadBits, 99);
    std::vector<uint8_t> coded(4 * (kPayloadBits + 6));
    dab::convolutional_encode(payload.data(), kPayloadBits, coded.data());

    auto soft = to_soft(coded, 64);
    // Erase every fourth coded bit, as puncturing to rate 1/3 would.
    for (size_t i = 3; i < soft.size(); i += 4)
        soft[i] = 0;

    dab::ViterbiDecoder decoder;
    std::vector<uint8_t> out((kPayloadBits + 7) / 8, 0);
    const size_t decoded = decoder.decode(to_steps(soft).data(), soft.size(), out.data(), out.size());

    REQUIRE(decoded == kPayloadBits);
    CHECK(bit_errors(payload.data(), out.data(), kPayloadBits) == 0);
}

TEST_CASE("a long burst of corruption is not silently accepted") {
    // Guards against a decoder that ignores its input and returns something
    // plausible regardless.
    auto payload = make_payload(kPayloadBits, 5);
    std::vector<uint8_t> coded(4 * (kPayloadBits + 6));
    dab::convolutional_encode(payload.data(), kPayloadBits, coded.data());

    auto soft = to_soft(coded, 64);
    for (size_t i = 100; i < 240; ++i)
        soft[i] = static_cast<int8_t>(-soft[i]);

    dab::ViterbiDecoder decoder;
    std::vector<uint8_t> out((kPayloadBits + 7) / 8, 0);
    decoder.decode(to_steps(soft).data(), soft.size(), out.data(), out.size());

    CHECK(bit_errors(payload.data(), out.data(), kPayloadBits) > 0);
}

TEST_CASE("decoder is reusable across blocks") {
    dab::ViterbiDecoder decoder;
    for (uint32_t seed = 1; seed <= 4; ++seed) {
        auto payload = make_payload(kPayloadBits, seed);
        std::vector<uint8_t> coded(4 * (kPayloadBits + 6));
        dab::convolutional_encode(payload.data(), kPayloadBits, coded.data());
        auto soft = to_soft(coded, 64);

        std::vector<uint8_t> out((kPayloadBits + 7) / 8, 0);
        const size_t decoded = decoder.decode(to_steps(soft).data(), soft.size(), out.data(), out.size());
        REQUIRE(decoded == kPayloadBits);
        CHECK(bit_errors(payload.data(), out.data(), kPayloadBits) == 0);
    }
}

TEST_CASE("a full FIC codeword decodes") {
    // FIC codewords carry 768 payload bits.
    constexpr size_t kFicBits = 768;
    auto payload = make_payload(kFicBits, 31337);
    std::vector<uint8_t> coded(4 * (kFicBits + 6));
    dab::convolutional_encode(payload.data(), kFicBits, coded.data());
    auto soft = to_soft(coded, 64);

    dab::ViterbiDecoder decoder;
    std::vector<uint8_t> out((kFicBits + 7) / 8, 0);
    const size_t decoded = decoder.decode(to_steps(soft).data(), soft.size(), out.data(), out.size());

    REQUIRE(decoded == kFicBits);
    CHECK(bit_errors(payload.data(), out.data(), kFicBits) == 0);
}

TEST_CASE("rejects malformed arguments") {
    dab::ViterbiDecoder decoder;
    const size_t coded = 4 * (kPayloadBits + 6);
    std::vector<dab::ViterbiDecoder::Soft> steps(coded / 4, 0x1111);
    std::vector<uint8_t> out(64, 0);

    CHECK(decoder.decode(nullptr, coded, out.data(), out.size()) == 0);
    CHECK(decoder.decode(steps.data(), coded, nullptr, out.size()) == 0);
    CHECK(decoder.decode(steps.data(), 0, out.data(), out.size()) == 0);
    CHECK(decoder.decode(steps.data(), 7, out.data(), out.size()) == 0);  // not a multiple of 4
    CHECK(decoder.decode(steps.data(), coded, out.data(), 1) == 0);       // output too small
}

TEST_CASE("rejects a payload beyond the supported bound") {
    dab::ViterbiDecoder decoder;
    const size_t too_many = dab::kViterbiMaxPayloadBits + 8;
    std::vector<dab::ViterbiDecoder::Soft> steps(too_many + 6, 0x1111);
    std::vector<uint8_t> out((too_many + 7) / 8, 0);
    CHECK(decoder.decode(steps.data(), 4 * (too_many + 6), out.data(), out.size()) == 0);
}
