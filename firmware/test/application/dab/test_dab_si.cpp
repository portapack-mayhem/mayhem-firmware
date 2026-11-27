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

#include "dab/dab_si.hpp"
#include "dab/dab_common.hpp"

#include <string>
#include <cstring>
#include <cstdint>
#include <vector>

/* ---- Character sets ---- */

namespace {

std::string convert(const std::string& in, uint8_t charset) {
    char out[64];
    const size_t n = dab::to_display(reinterpret_cast<const uint8_t*>(in.data()), in.size(), charset, out, sizeof(out));
    return std::string(out, n);
}

}  // namespace

TEST_SUITE_BEGIN("DAB charset");

TEST_CASE("EBU Latin o-acute becomes Latin-1") {
    CHECK(convert("G\x86ra", dab::kCharsetEbuLatin) ==
          "G\xF3"
          "ra");
    CHECK(convert("Kr\x86l", dab::kCharsetEbuLatin) == "Kr\xF3l");
}

TEST_CASE("EBU Latin Polish letters outside Latin-1 fold to ASCII") {
    /* z-dot, n-acute, a-ogonek, e-ogonek, s-acute, c-acute, z-acute */
    CHECK(convert("\x0E\x1E\xB6\x10\x11\xFC\xFB\xFD", dab::kCharsetEbuLatin) == "Zznaescz");
}

TEST_CASE("UTF-8 Polish text: Latin-1 kept, the rest folded") {
    CHECK(convert("\xC5\xBB\xC3\xB3\xC5\x82\xC4\x87", dab::kCharsetUtf8) ==
          "Z\xF3"
          "lc");
    CHECK(convert("Pi\xC4\x99kna \xC5\x81\xC3\xB3"
                  "dka",
                  dab::kCharsetUtf8) ==
          "Piekna L\xF3"
          "dka");
}

TEST_CASE("controls and padding are dropped, unknown code points become ?") {
    CHECK(convert(std::string("ab\0\0", 4), dab::kCharsetEbuLatin) == "ab");
    CHECK(convert("\xE2\x82\xAC 5", dab::kCharsetUtf8) == "E 5");
    CHECK(convert("\xE2\x98\x83", dab::kCharsetUtf8) == "?");
    CHECK(convert(std::string("\x00\x41\x01\x42", 4), dab::kCharsetUcs2) == "Al");
}

TEST_CASE("output never exceeds capacity") {
    char out[4];
    const std::string in = "abcdefgh";
    CHECK(dab::to_display(reinterpret_cast<const uint8_t*>(in.data()), in.size(), dab::kCharsetEbuLatin, out, sizeof(out)) == 4);
}

TEST_SUITE_END();

/* ---- Channel blocks ---- */

TEST_CASE("dab mode I constants") {
    CHECK(dab::kSampleRate == 2048000);
    CHECK(dab::kCarriers == 1536);
    CHECK(dab::kFftSize == 2048);
    CHECK(dab::kGuardSamples == 504);
    CHECK(dab::kSymbolSamples == 2552);
    CHECK(dab::kNullSamples == 2656);
    CHECK(dab::kSymbolsPerFrame == 76);
    CHECK(dab::kFicSymbols == 3);
    CHECK(dab::kMscSymbols == 72);
    CHECK(dab::kCifsPerFrame == 4);
    CHECK(dab::kCuPerCif == 864);
    CHECK(dab::kBitsPerCif == 55296);
    CHECK(dab::kFibsPerFrame == 12);
    CHECK(dab::kFibBytes == 32);
    CHECK(dab::kFrameDurationMs == 96);
}

TEST_CASE("band III block table") {
    const auto blocks = dab::band_iii_blocks();
    REQUIRE(blocks.count == 41);

    CHECK(std::strcmp(blocks.items[0].name, "5A") == 0);
    CHECK(blocks.items[0].frequency_hz == 174928000);

    CHECK(std::strcmp(blocks.items[blocks.count - 1].name, "13F") == 0);
    CHECK(blocks.items[blocks.count - 1].frequency_hz == 239200000);

    // Frequencies must be strictly ascending so a sweep can walk the table in order.
    for (size_t i = 1; i < blocks.count; ++i) {
        CHECK(blocks.items[i].frequency_hz > blocks.items[i - 1].frequency_hz);
    }
}

TEST_CASE("band III includes the national N blocks") {
    CHECK(dab::find_block("10N") != nullptr);
    CHECK(dab::find_block("11N") != nullptr);
    CHECK(dab::find_block("12N") != nullptr);
    CHECK(dab::find_block("10N")->frequency_hz == 210096000);
}

TEST_CASE("L-band block table") {
    const auto blocks = dab::l_band_blocks();
    REQUIRE(blocks.count == 23);

    CHECK(std::strcmp(blocks.items[0].name, "LA") == 0);
    CHECK(blocks.items[0].frequency_hz == 1452960000);
    CHECK(std::strcmp(blocks.items[blocks.count - 1].name, "LW") == 0);

    for (size_t i = 1; i < blocks.count; ++i) {
        CHECK(blocks.items[i].frequency_hz > blocks.items[i - 1].frequency_hz);
    }
}

TEST_CASE("block lookup by name") {
    const auto* b = dab::find_block("12B");
    REQUIRE(b != nullptr);
    CHECK(b->frequency_hz == 225648000);

    CHECK(dab::find_block("99Z") == nullptr);
    CHECK(dab::find_block("") == nullptr);
    CHECK(dab::find_block(nullptr) == nullptr);
}

TEST_CASE("every block is spaced clear of its neighbour") {
    // DAB occupies 1.536 MHz; the raster must never place two blocks closer
    // than that or a scan would decode the same ensemble twice.
    const auto blocks = dab::band_iii_blocks();
    for (size_t i = 1; i < blocks.count; ++i) {
        const auto gap = blocks.items[i].frequency_hz - blocks.items[i - 1].frequency_hz;
        CHECK(gap >= 160000);
    }
}

/* ---- FIG parsing ---- */

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

/* ---- Ensemble database ---- */

namespace {

dab::ProtectionProfile eep3a() {
    return dab::ProtectionProfile{dab::ProtectionKind::Eep, 3, false};
}

}  // namespace

TEST_CASE("service is assembled when the label arrives before the subchannel") {
    dab::EnsembleDatabase db;
    db.on_service_component(0xC0FF, 5, dab::ComponentKind::MscStreamAudio, dab::kAsctyAac, true);
    db.on_label(0xC0FF, dab::LabelKind::ProgrammeService, "Radio One");
    db.on_subchannel(5, 100, 96, eep3a());

    const auto* s = db.find_service(0xC0FF);
    REQUIRE(s != nullptr);
    CHECK(std::strcmp(s->label, "Radio One") == 0);
    CHECK(s->start_cu == 100);
    CHECK(s->size_cu == 96);
    CHECK(s->bitrate == 128000);
    CHECK(s->is_dab_plus);
    CHECK(s->is_playable());
}

TEST_CASE("service is assembled when the subchannel arrives first") {
    dab::EnsembleDatabase db;
    db.on_subchannel(5, 100, 96, eep3a());
    db.on_service_component(0xC0FF, 5, dab::ComponentKind::MscStreamAudio, dab::kAsctyAac, true);
    db.on_label(0xC0FF, dab::LabelKind::ProgrammeService, "Radio One");

    const auto* s = db.find_service(0xC0FF);
    REQUIRE(s != nullptr);
    CHECK(s->bitrate == 128000);
    CHECK(s->is_playable());
}

TEST_CASE("classic DAB services are distinguished from DAB+") {
    dab::EnsembleDatabase db;
    db.on_service_component(0x1111, 1, dab::ComponentKind::MscStreamAudio, dab::kAsctyMp2, true);
    db.on_service_component(0x2222, 2, dab::ComponentKind::MscStreamAudio, dab::kAsctyAac, true);

    CHECK_FALSE(db.find_service(0x1111)->is_dab_plus);
    CHECK(db.find_service(0x2222)->is_dab_plus);
}

TEST_CASE("data components are ignored") {
    dab::EnsembleDatabase db;
    db.on_service_component(0x3333, 9, dab::ComponentKind::MscStreamData, 0, true);
    CHECK(db.service_count() == 0);
}

TEST_CASE("slideshow capability comes from the user application FIG") {
    dab::EnsembleDatabase db;
    db.on_service_component(0xC0FF, 5, dab::ComponentKind::MscStreamAudio, dab::kAsctyAac, true);
    CHECK_FALSE(db.find_service(0xC0FF)->has_slideshow);

    db.on_user_application(0xC0FF, dab::kUserAppSlideshow);
    CHECK(db.find_service(0xC0FF)->has_slideshow);
}

TEST_CASE("an unrelated user application does not set slideshow") {
    dab::EnsembleDatabase db;
    db.on_service_component(0xC0FF, 5, dab::ComponentKind::MscStreamAudio, dab::kAsctyAac, true);
    db.on_user_application(0xC0FF, 0x44);
    CHECK_FALSE(db.find_service(0xC0FF)->has_slideshow);
}

TEST_CASE("completeness requires ensemble label, id and every service resolved") {
    dab::EnsembleDatabase db;
    CHECK_FALSE(db.is_complete());

    db.on_ensemble(0x1234, 0, false);
    db.on_label(0x1234, dab::LabelKind::Ensemble, "Test Mux");
    CHECK_FALSE(db.is_complete());  // no services yet

    db.on_service_component(0xC0FF, 5, dab::ComponentKind::MscStreamAudio, dab::kAsctyAac, true);
    CHECK_FALSE(db.is_complete());  // no label, no subchannel

    db.on_label(0xC0FF, dab::LabelKind::ProgrammeService, "Radio One");
    CHECK_FALSE(db.is_complete());  // still no subchannel

    db.on_subchannel(5, 100, 96, eep3a());
    CHECK(db.is_complete());
}

TEST_CASE("one unresolved service blocks completeness") {
    dab::EnsembleDatabase db;
    db.on_ensemble(0x1234, 0, false);
    db.on_label(0x1234, dab::LabelKind::Ensemble, "Test Mux");
    db.on_service_component(0xC0FF, 5, dab::ComponentKind::MscStreamAudio, dab::kAsctyAac, true);
    db.on_label(0xC0FF, dab::LabelKind::ProgrammeService, "Radio One");
    db.on_subchannel(5, 100, 96, eep3a());
    REQUIRE(db.is_complete());

    db.on_service_component(0xD0FF, 6, dab::ComponentKind::MscStreamAudio, dab::kAsctyAac, true);
    CHECK_FALSE(db.is_complete());
}

TEST_CASE("reconfiguration flag latches until acknowledged") {
    dab::EnsembleDatabase db;
    db.on_ensemble(0x1234, 0, false);
    CHECK_FALSE(db.reconfiguration_pending());

    db.on_ensemble(0x1234, 1, true);
    CHECK(db.reconfiguration_pending());

    // A later FIG without the flag must not clear it; only the consumer may.
    db.on_ensemble(0x1234, 2, false);
    CHECK(db.reconfiguration_pending());

    db.acknowledge_reconfiguration();
    CHECK_FALSE(db.reconfiguration_pending());
}

TEST_CASE("clear resets everything") {
    dab::EnsembleDatabase db;
    db.on_ensemble(0x1234, 0, true);
    db.on_label(0x1234, dab::LabelKind::Ensemble, "Test Mux");
    db.on_service_component(0xC0FF, 5, dab::ComponentKind::MscStreamAudio, dab::kAsctyAac, true);

    db.clear();
    CHECK(db.service_count() == 0);
    CHECK_FALSE(db.has_ensemble_id());
    CHECK_FALSE(db.reconfiguration_pending());
    CHECK(db.ensemble_label()[0] == '\0');
}

TEST_CASE("repeated FIGs do not duplicate services") {
    dab::EnsembleDatabase db;
    for (int i = 0; i < 10; ++i) {
        db.on_service_component(0xC0FF, 5, dab::ComponentKind::MscStreamAudio, dab::kAsctyAac, true);
        db.on_label(0xC0FF, dab::LabelKind::ProgrammeService, "Radio One");
    }
    CHECK(db.service_count() == 1);
}

TEST_CASE("service capacity is bounded and does not overflow") {
    dab::EnsembleDatabase db;
    for (uint32_t i = 0; i < dab::kMaxServices + 20; ++i)
        db.on_service_component(0x1000 + i, 5, dab::ComponentKind::MscStreamAudio, dab::kAsctyAac, true);

    CHECK(db.service_count() == dab::kMaxServices);
}

TEST_CASE("out of range subchannel ids are ignored") {
    dab::EnsembleDatabase db;
    db.on_subchannel(200, 0, 96, eep3a());  // valid ids are 0..63
    CHECK(db.service_count() == 0);
}

TEST_CASE("programme type and country are recorded") {
    dab::EnsembleDatabase db;
    db.on_service_component(0xC0FF, 5, dab::ComponentKind::MscStreamAudio, dab::kAsctyAac, true);
    db.on_programme_type(0xC0FF, 10);
    db.on_country(0xE1);

    CHECK(db.find_service(0xC0FF)->pty == 10);
    CHECK(db.country_ecc() == 0xE1);
}

TEST_CASE("unknown service lookup returns null") {
    dab::EnsembleDatabase db;
    CHECK(db.find_service(0xDEAD) == nullptr);
}

TEST_CASE("entries created only by a label do not block completeness") {
    // FIG 1, 0/17 and 0/13 all reference identifiers that may have no audio
    // component. Treating those as services waiting to resolve would keep
    // is_complete() false forever and make every scan run its full dwell.
    dab::EnsembleDatabase db;
    db.on_ensemble(0x1234, 0, false);
    db.on_label(0x1234, dab::LabelKind::Ensemble, "Test Mux");

    db.on_service_component(0xC0FF, 5, dab::ComponentKind::MscStreamAudio, dab::kAsctyAac, true);
    db.on_label(0xC0FF, dab::LabelKind::ProgrammeService, "Radio One");
    db.on_subchannel(5, 100, 96, eep3a());
    REQUIRE(db.is_complete());

    // A data-service label and a programme type for identifiers with no audio.
    db.on_label(0xDEAD, dab::LabelKind::DataService, "Data");
    db.on_programme_type(0xBEEF, 10);
    db.on_user_application(0xFEED, dab::kUserAppSlideshow);

    CHECK(db.is_complete());
}

TEST_CASE("a UEP service does not hold completeness open") {
    // Short-form FIG 0/1 carries no subchannel size, so a UEP service can
    // never become playable and must not block the scanner.
    dab::EnsembleDatabase db;
    db.on_ensemble(0x1234, 0, false);
    db.on_label(0x1234, dab::LabelKind::Ensemble, "Test Mux");

    db.on_service_component(0xC0FF, 5, dab::ComponentKind::MscStreamAudio, dab::kAsctyAac, true);
    db.on_label(0xC0FF, dab::LabelKind::ProgrammeService, "Radio One");
    db.on_subchannel(5, 100, 96, eep3a());

    db.on_service_component(0xD0FF, 6, dab::ComponentKind::MscStreamAudio, dab::kAsctyMp2, true);
    db.on_label(0xD0FF, dab::LabelKind::ProgrammeService, "Legacy");
    db.on_subchannel(6, 200, 0, dab::ProtectionProfile{dab::ProtectionKind::Uep, 9, false});

    CHECK(db.is_complete());
    CHECK_FALSE(db.find_service(0xD0FF)->is_playable());
}

TEST_CASE("completeness still requires at least one audio service") {
    dab::EnsembleDatabase db;
    db.on_ensemble(0x1234, 0, false);
    db.on_label(0x1234, dab::LabelKind::Ensemble, "Test Mux");
    db.on_label(0xDEAD, dab::LabelKind::DataService, "Data only");

    CHECK_FALSE(db.is_complete());
}

/* ---- Station cache ---- */

namespace {

dab::EnsembleDatabase build_db() {
    dab::EnsembleDatabase db;
    db.on_ensemble(0x1234, 0, false);
    db.on_label(0x1234, dab::LabelKind::Ensemble, "Test Mux");
    db.on_service_component(0xC0FF, 5, dab::ComponentKind::MscStreamAudio, dab::kAsctyAac, true);
    db.on_label(0xC0FF, dab::LabelKind::ProgrammeService, "Radio One");
    db.on_subchannel(5, 100, 96, dab::ProtectionProfile{dab::ProtectionKind::Eep, 3, false});
    return db;
}

}  // namespace

TEST_CASE("escaping round trips commas and backslashes") {
    const std::string tricky = "Rock, Pop \\ Jazz";
    const std::string escaped = dab::escape_field(tricky);
    CHECK(escaped.find("\\,") != std::string::npos);
    CHECK(dab::unescape_field(escaped) == tricky);
}

TEST_CASE("a playable service serialises to one line") {
    auto db = build_db();
    std::string out;
    CHECK(dab::serialise_ensemble(db, "12B", 1000, out) == 1);
    CHECK(out.find("12B") == 0);
    CHECK(out.find("Radio One") != std::string::npos);
    CHECK(out.back() == '\n');
}

TEST_CASE("serialise and parse round trip preserves every field") {
    auto db = build_db();
    std::string out;
    REQUIRE(dab::serialise_ensemble(db, "12B", 4242, out) == 1);

    dab::CachedService cs;
    REQUIRE(dab::parse_cache_line(out.c_str(), cs));

    CHECK(cs.block == "12B");
    CHECK(cs.eid == 0x1234);
    CHECK(cs.ensemble_label == "Test Mux");
    CHECK(cs.sid == 0xC0FF);
    CHECK(cs.service_label == "Radio One");
    CHECK(cs.subch_id == 5);
    CHECK(cs.start_cu == 100);
    CHECK(cs.size_cu == 96);
    CHECK(cs.protection.level == 3);
    CHECK_FALSE(cs.protection.option_b);
    CHECK(cs.bitrate == 128000);
    CHECK(cs.is_dab_plus);
    CHECK(cs.last_seen == 4242);
}

TEST_CASE("labels containing commas survive the round trip") {
    dab::EnsembleDatabase db;
    db.on_ensemble(0x1234, 0, false);
    db.on_label(0x1234, dab::LabelKind::Ensemble, "News, Talk");
    db.on_service_component(0xC0FF, 5, dab::ComponentKind::MscStreamAudio, dab::kAsctyAac, true);
    db.on_label(0xC0FF, dab::LabelKind::ProgrammeService, "Rock, Pop");
    db.on_subchannel(5, 100, 96, dab::ProtectionProfile{dab::ProtectionKind::Eep, 3, false});

    std::string out;
    REQUIRE(dab::serialise_ensemble(db, "5A", 1, out) == 1);

    dab::CachedService cs;
    REQUIRE(dab::parse_cache_line(out.c_str(), cs));
    CHECK(cs.service_label == "Rock, Pop");
    CHECK(cs.ensemble_label == "News, Talk");
}

TEST_CASE("services that cannot be tuned are not cached") {
    dab::EnsembleDatabase db;
    db.on_ensemble(0x1234, 0, false);
    // Label but no subchannel.
    db.on_service_component(0xC0FF, 5, dab::ComponentKind::MscStreamAudio, dab::kAsctyAac, true);
    db.on_label(0xC0FF, dab::LabelKind::ProgrammeService, "Radio One");

    std::string out;
    CHECK(dab::serialise_ensemble(db, "5A", 1, out) == 0);
    CHECK(out.empty());
}

TEST_CASE("the EEP option survives the round trip") {
    dab::EnsembleDatabase db;
    db.on_ensemble(0x1234, 0, false);
    db.on_label(0x1234, dab::LabelKind::Ensemble, "Mux");
    db.on_service_component(0xC0FF, 5, dab::ComponentKind::MscStreamAudio, dab::kAsctyAac, true);
    db.on_label(0xC0FF, dab::LabelKind::ProgrammeService, "Radio");
    db.on_subchannel(5, 100, 54, dab::ProtectionProfile{dab::ProtectionKind::Eep, 1, true});

    std::string out;
    REQUIRE(dab::serialise_ensemble(db, "5A", 1, out) == 1);

    dab::CachedService cs;
    REQUIRE(dab::parse_cache_line(out.c_str(), cs));
    CHECK(cs.protection.option_b);
    CHECK(cs.protection.level == 1);
}

TEST_CASE("malformed lines are rejected without crashing") {
    dab::CachedService cs;

    CHECK_FALSE(dab::parse_cache_line(nullptr, cs));
    CHECK_FALSE(dab::parse_cache_line("", cs));
    CHECK_FALSE(dab::parse_cache_line("# a comment", cs));
    CHECK_FALSE(dab::parse_cache_line("too,few,fields", cs));
    CHECK_FALSE(dab::parse_cache_line("12B,4660,Mux,49407,Radio,5,abc,96,3,0,128000,1,42", cs));   // non-numeric CU
    CHECK_FALSE(dab::parse_cache_line("12B,4660,Mux,49407,Radio,5,100,0,3,0,128000,1,42", cs));    // zero size
    CHECK_FALSE(dab::parse_cache_line("12B,4660,Mux,49407,Radio,99,100,96,3,0,128000,1,42", cs));  // subch out of range
    CHECK_FALSE(dab::parse_cache_line("12B,4660,Mux,49407,Radio,5,100,96,9,0,128000,1,42", cs));   // bad protection level
    CHECK_FALSE(dab::parse_cache_line("12B,4660,Mux,49407,,5,100,96,3,0,128000,1,42", cs));        // empty label
    CHECK_FALSE(dab::parse_cache_line("12B,4660,Mux,49407,Radio,5,100,96,3,0,128000,1,42,extra", cs));
}

TEST_CASE("a trailing backslash is rejected") {
    dab::CachedService cs;
    CHECK_FALSE(dab::parse_cache_line("12B,4660,Mux,49407,Radio,5,100,96,3,0,128000,1,42\\", cs));
}

TEST_CASE("a rejected line leaves the output untouched") {
    dab::CachedService cs;
    cs.block = "sentinel";
    CHECK_FALSE(dab::parse_cache_line("garbage", cs));
    CHECK(cs.block == "sentinel");
}

TEST_CASE("trailing newline and carriage return are tolerated") {
    dab::CachedService cs;
    CHECK(dab::parse_cache_line("12B,4660,Mux,49407,Radio,5,100,96,3,0,128000,1,42\r\n", cs));
    CHECK(cs.bitrate == 128000);
}

TEST_CASE("multiple services produce multiple lines") {
    dab::EnsembleDatabase db = build_db();
    db.on_service_component(0xD0FF, 6, dab::ComponentKind::MscStreamAudio, dab::kAsctyMp2, true);
    db.on_label(0xD0FF, dab::LabelKind::ProgrammeService, "Radio Two");
    db.on_subchannel(6, 200, 72, dab::ProtectionProfile{dab::ProtectionKind::Eep, 2, false});

    std::string out;
    CHECK(dab::serialise_ensemble(db, "12B", 7, out) == 2);

    size_t newlines = 0;
    for (char c : out)
        if (c == '\n')
            ++newlines;
    CHECK(newlines == 2);
}
