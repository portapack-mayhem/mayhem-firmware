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

#include "dab_ensemble.hpp"

#include <cstring>

namespace dab {

void EnsembleDatabase::clear() {
    for (size_t i = 0; i < kMaxServices; ++i)
        services_[i] = DabService{};
    service_count_ = 0;

    for (size_t i = 0; i < kMaxSubchannels; ++i)
        subchannels_[i] = Subchannel{};

    ensemble_label_[0] = '\0';
    ensemble_id_ = 0;
    has_ensemble_id_ = false;
    reconfiguration_ = false;
    ecc_ = 0;
}

DabService* EnsembleDatabase::find_or_add(uint32_t sid) {
    for (size_t i = 0; i < service_count_; ++i) {
        if (services_[i].sid == sid)
            return &services_[i];
    }

    if (service_count_ >= kMaxServices)
        return nullptr;

    DabService& s = services_[service_count_++];
    s = DabService{};
    s.sid = sid;
    return &s;
}

const DabService* EnsembleDatabase::find_service(uint32_t sid) const {
    for (size_t i = 0; i < service_count_; ++i) {
        if (services_[i].sid == sid)
            return &services_[i];
    }
    return nullptr;
}

void EnsembleDatabase::apply_subchannel(DabService& service) {
    if (service.subch_id >= kMaxSubchannels)
        return;

    const Subchannel& sub = subchannels_[service.subch_id];
    if (!sub.known)
        return;

    service.start_cu = sub.start_cu;
    service.size_cu = sub.size_cu;
    service.protection = sub.protection;
    service.bitrate = eep_bitrate(sub.size_cu, sub.protection);

    /* FIG 0/8 gives no ASCTy: UEP is Layer II only, so infer it from the
     * protection. A later FIG 0/2 overwrites this. */
    if (service.ascty == kAsctyUnknown && service.component_from_fig08) {
        service.ascty = (sub.protection.kind == ProtectionKind::Eep) ? kAsctyAac : kAsctyMp2;
        service.is_dab_plus = (service.ascty == kAsctyAac);
    }
}

void EnsembleDatabase::on_ensemble(uint16_t eid, uint16_t /*cif_count*/, bool reconfiguration) {
    ensemble_id_ = eid;
    has_ensemble_id_ = true;
    if (reconfiguration)
        reconfiguration_ = true;
}

void EnsembleDatabase::on_subchannel(uint8_t subch_id, uint16_t start_cu, uint16_t size_cu, const ProtectionProfile& protection) {
    if (subch_id >= kMaxSubchannels)
        return;

    Subchannel& sub = subchannels_[subch_id];
    sub.known = true;
    sub.start_cu = start_cu;
    sub.size_cu = size_cu;
    sub.protection = protection;

    /* A subchannel may arrive after the services that reference it. */
    for (size_t i = 0; i < service_count_; ++i) {
        if (services_[i].subch_id == subch_id)
            apply_subchannel(services_[i]);
    }
}

void EnsembleDatabase::on_service_component(uint32_t sid, uint8_t subch_id, ComponentKind kind, uint8_t ascty, bool primary) {
    if (kind != ComponentKind::MscStreamAudio)
        return;

    DabService* s = find_or_add(sid);
    if (s == nullptr)
        return;

    /* Never replace a primary component with a secondary one. */
    if (s->subch_id != kNoSubchannel && !primary && s->primary)
        return;

    s->has_audio_component = true;
    s->component_from_fig08 = false;
    s->subch_id = subch_id;
    s->ascty = ascty;
    s->is_dab_plus = (ascty == kAsctyAac);
    s->primary = primary;
    apply_subchannel(*s);
}

void EnsembleDatabase::on_component_global_definition(uint32_t sid, uint8_t scids, uint8_t subch_id, bool programme_service) {
    /* A 32-bit SId is a data service (FIG 0/2 P/D flag): never audio. */
    if (!programme_service)
        return;
    if (subch_id >= kMaxSubchannels)
        return;

    DabService* s = find_or_add(sid);
    if (s == nullptr)
        return;

    /* FIG 0/2 carries the ASCTy and the primary flag; never overwrite it. */
    if (s->has_audio_component && !s->component_from_fig08)
        return;

    /* FIG 0/8 has no primary flag: keep the lowest SCIdS. */
    if (s->component_from_fig08 && scids > s->scids)
        return;

    s->has_audio_component = true;
    s->component_from_fig08 = true;
    s->scids = scids;
    s->subch_id = subch_id;
    apply_subchannel(*s);
}

void EnsembleDatabase::on_label(uint32_t id, LabelKind kind, const char* text) {
    if (text == nullptr)
        return;

    if (kind == LabelKind::Ensemble) {
        std::strncpy(ensemble_label_, text, kLabelBytes);
        ensemble_label_[kLabelBytes] = '\0';
        return;
    }

    DabService* s = find_or_add(id);
    if (s == nullptr)
        return;

    std::strncpy(s->label, text, kLabelBytes);
    s->label[kLabelBytes] = '\0';
}

void EnsembleDatabase::on_programme_type(uint32_t sid, uint8_t pty) {
    DabService* s = find_or_add(sid);
    if (s != nullptr)
        s->pty = pty;
}

void EnsembleDatabase::on_country(uint8_t ecc) {
    ecc_ = ecc;
}

void EnsembleDatabase::on_user_application(uint32_t sid, uint16_t app_type) {
    if (app_type != kUserAppSlideshow)
        return;

    DabService* s = find_or_add(sid);
    if (s != nullptr)
        s->has_slideshow = true;
}

bool EnsembleDatabase::is_complete() const {
    if (ensemble_label_[0] == '\0' || !has_ensemble_id_)
        return false;
    if (service_count_ == 0)
        return false;

    size_t audio_services = 0;
    for (size_t i = 0; i < service_count_; ++i) {
        if (!services_[i].has_audio_component)
            continue;

        ++audio_services;
        if (!services_[i].is_resolved())
            return false;
    }

    return audio_services > 0;
}

} /* namespace dab */
