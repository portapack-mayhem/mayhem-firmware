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

#include "dab/dab_fig.hpp"
#include "dab/dab_crc.hpp"

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace {

/* Assembles a FIB from FIG bodies and stamps a valid CRC. */
class FibBuilder {
   public:
    FibBuilder() {
        std::memset(fib_, 0xFF, sizeof(fib_));
        pos_ = 0;
    }

    void add(uint8_t type, const std::vector<uint8_t>& body) {
        fib_[pos_++] = static_cast<uint8_t>((type << 5) | (body.size() & 0x1F));
        for (uint8_t b : body)
            fib_[pos_++] = b;
    }

    const uint8_t* finish() {
        for (size_t i = pos_; i < dab::kFibPayloadBytes; ++i)
            fib_[i] = 0xFF;
        dab::fib_crc_stamp(fib_);
        return fib_;
    }

    uint8_t* raw() { return fib_; }

   private:
    uint8_t fib_[dab::kFibBytes];
    size_t pos_{0};
};

struct Recorder : dab::FigSink {
    uint16_t eid{0};
    uint16_t cif{0};
    bool reconfig{false};
    int ensemble_calls{0};

    struct Sub {
        uint8_t id;
        uint16_t start;
        uint16_t size;
        dab::ProtectionProfile prot;
    };
    std::vector<Sub> subs;

    struct Comp {
        uint32_t sid;
        uint8_t subch;
        dab::ComponentKind kind;
        uint8_t ascty;
        bool primary;
    };
    std::vector<Comp> comps;

    struct Lab {
        uint32_t id;
        dab::LabelKind kind;
        std::string text;
    };
    std::vector<Lab> labels;

    std::vector<std::pair<uint32_t, uint8_t>> ptys;
    std::vector<std::pair<uint32_t, uint16_t>> apps;
    uint8_t ecc{0};
    dab::DabTime time{};
    bool got_time{false};
    int unhandled{0};

    void on_ensemble(uint16_t e, uint16_t c, bool r) override {
        eid = e;
        cif = c;
        reconfig = r;
        ++ensemble_calls;
    }
    void on_subchannel(uint8_t id, uint16_t s, uint16_t sz, const dab::ProtectionProfile& p) override {
        subs.push_back({id, s, sz, p});
    }
    void on_service_component(uint32_t sid, uint8_t subch, dab::ComponentKind k, uint8_t a, bool prim) override {
        comps.push_back({sid, subch, k, a, prim});
    }
    void on_label(uint32_t id, dab::LabelKind k, const char* t) override {
        labels.push_back({id, k, std::string(t)});
    }
    void on_programme_type(uint32_t sid, uint8_t pty) override { ptys.push_back({sid, pty}); }
    void on_user_application(uint32_t sid, uint16_t at) override { apps.push_back({sid, at}); }
    void on_country(uint8_t e) override { ecc = e; }
    void on_datetime(const dab::DabTime& t) override {
        time = t;
        got_time = true;
    }
    void on_unhandled(uint8_t, uint8_t) override { ++unhandled; }
};

}  // namespace

TEST_CASE("a bad FIB CRC dispatches nothing") {
    FibBuilder b;
    b.add(0, {0x00, 0x12, 0x34, 0x00, 0x05});
    b.finish();
    b.raw()[3] ^= 0xFF;  // corrupt after stamping

    Recorder r;
    dab::FigParser parser;
    CHECK_FALSE(parser.parse_fib(b.raw(), r));
    CHECK(r.ensemble_calls == 0);
    CHECK(parser.fibs_rejected() == 1);
}

TEST_CASE("FIG 0/0 yields ensemble id, CIF count and reconfiguration flag") {
    FibBuilder b;
    // extension 0; EId 0x1234; change flag set; CIF count 0x0105.
    b.add(0, {0x00, 0x12, 0x34, static_cast<uint8_t>(0x40 | 0x01), 0x05});

    Recorder r;
    dab::FigParser parser;
    REQUIRE(parser.parse_fib(b.finish(), r));

    CHECK(r.ensemble_calls == 1);
    CHECK(r.eid == 0x1234);
    CHECK(r.cif == 0x0105);
    CHECK(r.reconfig);
}

TEST_CASE("FIG 0/1 long form yields EEP subchannel parameters") {
    FibBuilder b;
    // subch id 5, start 100, long form, option A, level index 2 (=level 3), size 96.
    const uint8_t sub_hi = static_cast<uint8_t>((5 << 2) | ((100 >> 8) & 0x03));
    const uint8_t sub_lo = static_cast<uint8_t>(100 & 0xFF);
    const uint8_t f1 = static_cast<uint8_t>(0x80 | (0 << 4) | (2 << 2) | ((96 >> 8) & 0x03));
    const uint8_t f2 = static_cast<uint8_t>(96 & 0xFF);
    b.add(0, {0x01, sub_hi, sub_lo, f1, f2});

    Recorder r;
    dab::FigParser parser;
    REQUIRE(parser.parse_fib(b.finish(), r));

    REQUIRE(r.subs.size() == 1);
    CHECK(r.subs[0].id == 5);
    CHECK(r.subs[0].start == 100);
    CHECK(r.subs[0].size == 96);
    CHECK(r.subs[0].prot.kind == dab::ProtectionKind::Eep);
    CHECK(r.subs[0].prot.level == 3);
    CHECK(r.subs[0].prot.option_b == false);
}

TEST_CASE("FIG 0/1 short form is reported as UEP") {
    FibBuilder b;
    const uint8_t sub_hi = static_cast<uint8_t>((7 << 2) | 0);
    b.add(0, {0x01, sub_hi, 0x20, 0x09});

    Recorder r;
    dab::FigParser parser;
    REQUIRE(parser.parse_fib(b.finish(), r));

    REQUIRE(r.subs.size() == 1);
    CHECK(r.subs[0].id == 7);
    CHECK(r.subs[0].prot.kind == dab::ProtectionKind::Uep);
}

TEST_CASE("FIG 0/2 binds a service to its audio subchannel") {
    FibBuilder b;
    // SId 0xC0FF, one component, TMId 0 (audio), ASCTy 63, SubChId 5, primary.
    const uint8_t c0 = static_cast<uint8_t>((0 << 6) | 63);
    const uint8_t c1 = static_cast<uint8_t>((5 << 2) | (1 << 1));
    b.add(0, {0x02, 0xC0, 0xFF, 0x01, c0, c1});

    Recorder r;
    dab::FigParser parser;
    REQUIRE(parser.parse_fib(b.finish(), r));

    REQUIRE(r.comps.size() == 1);
    CHECK(r.comps[0].sid == 0xC0FF);
    CHECK(r.comps[0].subch == 5);
    CHECK(r.comps[0].kind == dab::ComponentKind::MscStreamAudio);
    CHECK(r.comps[0].ascty == 63);
    CHECK(r.comps[0].primary);
}

TEST_CASE("FIG 1/0 and 1/1 produce trimmed labels") {
    FibBuilder b;
    std::vector<uint8_t> ens = {0x00, 0x12, 0x34};
    const char* ens_text = "BBC National   ";
    for (size_t i = 0; i < 16; ++i)
        ens.push_back(i < std::strlen(ens_text) ? ens_text[i] : ' ');
    ens.push_back(0xFF);
    ens.push_back(0xFF);
    b.add(1, ens);

    Recorder r;
    dab::FigParser parser;
    REQUIRE(parser.parse_fib(b.finish(), r));

    REQUIRE(r.labels.size() == 1);
    CHECK(r.labels[0].kind == dab::LabelKind::Ensemble);
    CHECK(r.labels[0].id == 0x1234);
    CHECK(r.labels[0].text == "BBC National");
}

TEST_CASE("FIG 0/17 yields programme type") {
    FibBuilder b;
    b.add(0, {0x11, 0xC0, 0xFF, 0x00, 0x0A});

    Recorder r;
    dab::FigParser parser;
    REQUIRE(parser.parse_fib(b.finish(), r));

    REQUIRE(r.ptys.size() == 1);
    CHECK(r.ptys[0].first == 0xC0FF);
    CHECK(r.ptys[0].second == 10);
}

TEST_CASE("FIG 0/10 yields date and time") {
    FibBuilder b;
    // MJD 60000 = 0xEA60 within 17 bits; hours 13, minutes 45.
    const uint32_t mjd = 60000;
    const uint8_t b0 = static_cast<uint8_t>((mjd >> 10) & 0x7F);
    const uint8_t b1 = static_cast<uint8_t>((mjd >> 2) & 0xFF);
    const uint8_t b2 = static_cast<uint8_t>(((mjd & 0x03) << 6) | ((13 >> 2) & 0x07));
    const uint8_t b3 = static_cast<uint8_t>(((13 & 0x03) << 6) | 45);
    b.add(0, {0x0A, b0, b1, b2, b3});

    Recorder r;
    dab::FigParser parser;
    REQUIRE(parser.parse_fib(b.finish(), r));

    REQUIRE(r.got_time);
    CHECK(r.time.mjd == mjd);
    CHECK(r.time.hours == 13);
    CHECK(r.time.minutes == 45);
}

TEST_CASE("FIG 0/9 yields the country code") {
    FibBuilder b;
    b.add(0, {0x09, 0x00, 0xE1, 0x00});

    Recorder r;
    dab::FigParser parser;
    REQUIRE(parser.parse_fib(b.finish(), r));
    CHECK(r.ecc == 0xE1);
}

TEST_CASE("FIG 0/13 yields user application types") {
    FibBuilder b;
    // SId 0xC0FF, one app, type 0x002 (slideshow), zero data bytes.
    b.add(0, {0x0D, 0xC0, 0xFF, 0x01, 0x00, 0x40});

    Recorder r;
    dab::FigParser parser;
    REQUIRE(parser.parse_fib(b.finish(), r));

    REQUIRE(r.apps.size() == 1);
    CHECK(r.apps[0].first == 0xC0FF);
    CHECK(r.apps[0].second == 2);
}

TEST_CASE("an unknown FIG reaches on_unhandled and is counted") {
    FibBuilder b;
    b.add(5, {0x01, 0x02, 0x03});

    Recorder r;
    dab::FigParser parser;
    REQUIRE(parser.parse_fib(b.finish(), r));
    CHECK(r.unhandled == 1);
    CHECK(parser.figs_unhandled() == 1);
}

TEST_CASE("a FIG claiming to run past the FIB end is discarded") {
    uint8_t fib[dab::kFibBytes];
    std::memset(fib, 0xFF, sizeof(fib));
    fib[0] = static_cast<uint8_t>((0 << 5) | 29);  // claims 29 bytes from offset 1
    fib[1] = 0x00;
    // Only 29 payload bytes remain after the header, so 1 + 29 == 30 is legal;
    // push it one further to make it illegal.
    fib[0] = static_cast<uint8_t>((0 << 5) | 30);
    dab::fib_crc_stamp(fib);

    Recorder r;
    dab::FigParser parser;
    REQUIRE(parser.parse_fib(fib, r));
    CHECK(parser.figs_malformed() >= 1);
    CHECK(r.ensemble_calls == 0);
}

TEST_CASE("multiple FIGs in one FIB are all dispatched") {
    FibBuilder b;
    b.add(0, {0x00, 0x12, 0x34, 0x00, 0x05});
    b.add(0, {0x11, 0xC0, 0xFF, 0x00, 0x0A});

    Recorder r;
    dab::FigParser parser;
    REQUIRE(parser.parse_fib(b.finish(), r));

    CHECK(r.ensemble_calls == 1);
    CHECK(r.ptys.size() == 1);
}

TEST_CASE("a truncated FIG 0/0 is rejected without reading out of bounds") {
    FibBuilder b;
    b.add(0, {0x00, 0x12});  // needs 4 payload bytes, has 2

    Recorder r;
    dab::FigParser parser;
    REQUIRE(parser.parse_fib(b.finish(), r));
    CHECK(r.ensemble_calls == 0);
    CHECK(parser.figs_malformed() >= 1);
}

TEST_CASE("an all-0xFF FIB parses as empty") {
    uint8_t fib[dab::kFibBytes];
    std::memset(fib, 0xFF, sizeof(fib));
    dab::fib_crc_stamp(fib);

    Recorder r;
    dab::FigParser parser;
    CHECK(parser.parse_fib(fib, r));
    CHECK(r.ensemble_calls == 0);
    CHECK(r.unhandled == 0);
}

TEST_CASE("null fib is refused") {
    Recorder r;
    dab::FigParser parser;
    CHECK_FALSE(parser.parse_fib(nullptr, r));
}
