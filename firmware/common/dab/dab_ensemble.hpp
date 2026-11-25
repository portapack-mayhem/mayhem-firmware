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

/* Ensemble and service database, merged per SId from FIGs that arrive in any
 * order; a service is playable once its label and subchannel are both known.
 * The subchannel comes from FIG 0/2 or FIG 0/8 (some ensembles use 0/8 only);
 * 0/2 wins, as only it carries ASCTy and the primary flag. Fixed capacity, no
 * heap: this runs on the M0 beside the UI. */

#ifndef __DAB_ENSEMBLE_H__
#define __DAB_ENSEMBLE_H__

#include "dab_fig.hpp"
#include "dab_protection.hpp"

#include <cstddef>
#include <cstdint>

namespace dab {

constexpr size_t kMaxServices = 64;
constexpr size_t kMaxSubchannels = 64;

constexpr uint8_t kAsctyMp2 = 0;
constexpr uint8_t kAsctyAac = 63;

/* Not yet named by any FIG; must never be read as Layer II. */
constexpr uint8_t kAsctyUnknown = 0xFF;

constexpr uint8_t kNoSubchannel = 0xFF;

constexpr uint16_t kUserAppSlideshow = 2;

struct DabService {
    uint32_t sid{0};
    char label[kLabelBufferBytes]{};
    uint8_t pty{0};
    uint8_t subch_id{kNoSubchannel};
    uint16_t start_cu{0};
    uint16_t size_cu{0};
    ProtectionProfile protection{};
    uint32_t bitrate{0};
    uint8_t ascty{kAsctyUnknown};
    bool is_dab_plus{false};
    bool has_slideshow{false};
    bool primary{false};

    /* Set by FIG 0/2 or 0/8. Entries made only by label, PTy or user-app FIGs
     * are not audio services and must not count as waiting to resolve. */
    bool has_audio_component{false};

    /* Subchannel known only from FIG 0/8 (no ASCTy, no P/S flag); a later
     * FIG 0/2 descriptor replaces it. */
    bool component_from_fig08{false};

    /* FIG 0/8 SCIdS of the component taken; the lowest wins. */
    uint8_t scids{0xFF};

    bool has_label() const { return label[0] != '\0'; }
    bool has_subchannel() const { return subch_id != kNoSubchannel && size_cu > 0; }
    bool is_playable() const { return has_audio_component && has_label() && has_subchannel(); }

    /* UEP (MP2 only, no size in FIG 0/1 short form) never becomes playable,
     * so it must not hold completeness open. */
    bool is_uep() const { return protection.kind == ProtectionKind::Uep; }
    bool is_resolved() const { return has_label() && (has_subchannel() || is_uep()); }
};

class EnsembleDatabase : public FigSink {
   public:
    EnsembleDatabase() { clear(); }

    void clear();

    const DabService* services() const { return services_; }
    size_t service_count() const { return service_count_; }

    const DabService* find_service(uint32_t sid) const;

    const char* ensemble_label() const { return ensemble_label_; }
    uint16_t ensemble_id() const { return ensemble_id_; }
    bool has_ensemble_id() const { return has_ensemble_id_; }
    uint8_t country_ecc() const { return ecc_; }

    /* Ensemble label known and every audio service resolved. The scanner
     * leaves a block early on this, so unresolvable entries must not block it. */
    bool is_complete() const;

    /* FIG 0/0 announced a change. The caller must rebuild: a stale start CU
     * decodes to noise, not to an error. */
    bool reconfiguration_pending() const { return reconfiguration_; }
    void acknowledge_reconfiguration() { reconfiguration_ = false; }

    void on_ensemble(uint16_t eid, uint16_t cif_count, bool reconfiguration) override;
    void on_subchannel(uint8_t subch_id, uint16_t start_cu, uint16_t size_cu, const ProtectionProfile& protection) override;
    void on_service_component(uint32_t sid, uint8_t subch_id, ComponentKind kind, uint8_t ascty, bool primary) override;
    void on_component_global_definition(uint32_t sid, uint8_t scids, uint8_t subch_id, bool programme_service) override;
    void on_label(uint32_t id, LabelKind kind, const char* text) override;
    void on_programme_type(uint32_t sid, uint8_t pty) override;
    void on_country(uint8_t ecc) override;
    void on_user_application(uint32_t sid, uint16_t app_type) override;

   private:
    struct Subchannel {
        bool known{false};
        uint16_t start_cu{0};
        uint16_t size_cu{0};
        ProtectionProfile protection{};
    };

    DabService* find_or_add(uint32_t sid);
    void apply_subchannel(DabService& service);

    DabService services_[kMaxServices]{};
    size_t service_count_{0};

    Subchannel subchannels_[kMaxSubchannels]{};

    char ensemble_label_[kLabelBufferBytes]{};
    uint16_t ensemble_id_{0};
    bool has_ensemble_id_{false};
    bool reconfiguration_{false};
    uint8_t ecc_{0};
};

} /* namespace dab */

#endif /*__DAB_ENSEMBLE_H__*/
