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

#include "dab/dab_msc.hpp"
#include "dab/dab_protection.hpp"
#include "dab/dab_viterbi.hpp"

#include <cstdint>
#include <vector>

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
