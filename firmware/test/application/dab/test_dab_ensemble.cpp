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

#include "dab/dab_ensemble.hpp"

#include <cstring>

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
