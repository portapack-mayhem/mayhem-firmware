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

/* Service information (M0): character sets, the Band III / L-Band block
 * table, FIG parsing, the ensemble database and its SD cache, and when
 * to engage a picked station. */

#ifndef __DAB_SI_H__
#define __DAB_SI_H__

#include "dab_common.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace dab {

/* ---- Character sets ---- */

/* Character set field of FIG 1 labels and DLS (ETSI TS 101 756 table 1). */
constexpr uint8_t kCharsetEbuLatin = 0x0;
constexpr uint8_t kCharsetUcs2 = 0x6;
constexpr uint8_t kCharsetUtf8 = 0xF;

/* To bytes the UI font draws: ASCII and Latin-1 as is (the font has glyphs for
 * 0xA0-0xFF), other Latin letters folded to the base letter, the rest '?';
 * control codes dropped. Unknown charsets read as EBU Latin. Writes at most
 * `capacity` bytes, unterminated; returns the count. */
size_t to_display(const uint8_t* in, size_t length, uint8_t charset, char* out, size_t capacity);

/* ---- Channel blocks ---- */

/* DAB channel raster for Band III and L-Band, per ETSI EN 300 401 Annex A. */

struct Block {
    const char* name;
    uint32_t frequency_hz;
};

struct BlockTable {
    const Block* items;
    size_t count;
};

/* 174-240 MHz, 41 blocks including the national 10N/11N/12N. */
BlockTable band_iii_blocks();

/* 1452-1492 MHz, LA through LW. */
BlockTable l_band_blocks();

/* Both bands. nullptr for an unknown or null name (cache input is unvalidated). */
const Block* find_block(const char* name);

/* ---- FIG parsing ---- */

/* FIB/FIG parsing per ETSI EN 300 401 clauses 5.2 and 8. A FIB is 30 bytes of
 * FIGs (3-bit type, 5-bit length, 0xFF ends) then a CRC-16. Handles FIG 0/0,
 * 0/1, 0/2, 0/8, 0/9, 0/10, 0/13, 0/17, 1/0, 1/1, 1/5; the rest are counted
 * via on_unhandled. A FIG claiming to run past its FIB is discarded whole. */

constexpr size_t kLabelBytes = 16;
constexpr size_t kLabelBufferBytes = kLabelBytes + 1;

enum class LabelKind : uint8_t {
    Ensemble,
    ProgrammeService,
    DataService,
};

/* TMId values. */
enum class ComponentKind : uint8_t {
    MscStreamAudio = 0,
    MscStreamData = 1,
    Fidc = 2,
    MscPacketData = 3,
};

struct DabTime {
    uint32_t mjd{0};  // Modified Julian Date
    uint8_t hours{0};
    uint8_t minutes{0};
    uint8_t seconds{0};
    bool has_seconds{false};
};

struct FigSink {
    virtual ~FigSink() = default;

    virtual void on_ensemble(uint16_t /*eid*/, uint16_t /*cif_count*/, bool /*reconfiguration*/) {}
    virtual void on_subchannel(uint8_t /*subch_id*/, uint16_t /*start_cu*/, uint16_t /*size_cu*/, const ProtectionProfile& /*protection*/) {}
    virtual void on_service_component(uint32_t /*sid*/, uint8_t /*subch_id*/, ComponentKind /*kind*/, uint8_t /*ascty*/, bool /*primary*/) {}
    virtual void on_packet_component(uint32_t /*sid*/, uint16_t /*service_component_id*/) {}

    /* FIG 0/8 short form, MSC/FIC 0: where a component lives, without ASCTy
     * or P/S flag. Some ensembles name a subchannel only here. */
    virtual void on_component_global_definition(uint32_t /*sid*/, uint8_t /*scids*/, uint8_t /*subch_id*/, bool /*programme_service*/) {}
    virtual void on_label(uint32_t /*id*/, LabelKind /*kind*/, const char* /*text*/) {}
    virtual void on_programme_type(uint32_t /*sid*/, uint8_t /*pty*/) {}
    virtual void on_country(uint8_t /*ecc*/) {}
    virtual void on_datetime(const DabTime& /*time*/) {}
    virtual void on_user_application(uint32_t /*sid*/, uint16_t /*app_type*/) {}
    virtual void on_unhandled(uint8_t /*type*/, uint8_t /*extension*/) {}
};

class FigParser {
   public:
    /* False, with nothing dispatched, when the CRC fails. */
    bool parse_fib(const uint8_t fib[kFibBytes], FigSink& sink);

    uint32_t fibs_accepted() const { return fibs_accepted_; }
    uint32_t fibs_rejected() const { return fibs_rejected_; }
    uint32_t figs_unhandled() const { return figs_unhandled_; }
    uint32_t figs_malformed() const { return figs_malformed_; }
    void reset_counters();

   private:
    void dispatch(uint8_t type, const uint8_t* body, size_t length, FigSink& sink);
    void parse_type0(const uint8_t* body, size_t length, FigSink& sink);
    void parse_type1(const uint8_t* body, size_t length, FigSink& sink);

    uint32_t fibs_accepted_{0};
    uint32_t fibs_rejected_{0};
    uint32_t figs_unhandled_{0};
    uint32_t figs_malformed_{0};
};

/* ---- Ensemble database ---- */

/* Ensemble and service database, merged per SId from FIGs that arrive in any
 * order; a service is playable once its label and subchannel are both known.
 * The subchannel comes from FIG 0/2 or FIG 0/8 (some ensembles use 0/8 only);
 * 0/2 wins, as only it carries ASCTy and the primary flag. Fixed capacity, no
 * heap: this runs on the M0 beside the UI. */

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

/* ---- Station cache ---- */

/* Ensemble cache at /DAB/ensembles.txt, one user-editable line per service:
 *   block,eid,ensemble_label,sid,service_label,subch,start,size,
 *   prot,protb,bitrate,plus,seen
 * Backslash and comma are escaped in labels. Entries are hints only: live FIC
 * overrides them, as subchannel allocations change. */

struct CachedService {
    std::string block{};
    uint16_t eid{0};
    std::string ensemble_label{};
    uint32_t sid{0};
    std::string service_label{};
    uint8_t subch_id{0};
    uint16_t start_cu{0};
    uint16_t size_cu{0};
    ProtectionProfile protection{};
    uint32_t bitrate{0};
    bool is_dab_plus{false};
    uint32_t last_seen{0};
};

/* Playable services only: an untunable entry is worse than none. Returns
 * the number of lines appended. */
size_t serialise_ensemble(const EnsembleDatabase& db,
                          const char* block,
                          uint32_t timestamp,
                          std::string& out);

/* False for anything malformed, leaving `out` untouched. */
bool parse_cache_line(const char* line, CachedService& out);

/* Exposed for testing. */
std::string escape_field(const std::string& value);
std::string unescape_field(const std::string& value);

/* ---- Station engagement ---- */

/* When to hand the baseband the picked station, and when to let go. The MSC
 * takes ~40% of the frame and acquisition cannot run under it, so a pick is a
 * standing request: engage after `streak` locked frames with r4 >= `gate`
 * outside a holdoff; release after `release_frames` below `release_r4`, then
 * hold off `holdoff` frames. Pure logic, so it can be tested off target. */

class Engagement {
   public:
    struct Config {
        uint16_t gate;
        uint16_t streak;
        uint16_t release_r4;
        uint16_t release_frames;
        uint16_t holdoff;
    };

    enum class Action : uint8_t {
        None,
        Engage,
        Release,
    };

    explicit constexpr Engagement(const Config& config)
        : config_{config} {}

    /* A change while engaged is immediate, with a clean loss count. */
    Action want() {
        wanted_ = true;
        streak_ = 0;
        lost_ = 0;
        return engaged_ ? Action::Engage : Action::None;
    }

    /* Stop, retune, scan: the caller releases whatever the baseband holds. */
    void drop() {
        wanted_ = false;
        engaged_ = false;
        streak_ = 0;
        lost_ = 0;
    }

    Action on_frame(bool locked, uint16_t r4, bool paused) {
        if (holdoff_ > 0)
            --holdoff_;
        if (!wanted_ || paused)
            return Action::None;

        if (!engaged_) {
            if (locked && r4 >= config_.gate && holdoff_ == 0) {
                if (streak_ < 0xFFFF)
                    ++streak_;
            } else {
                streak_ = 0;
            }
            if (streak_ > config_.streak) {
                engaged_ = true;
                streak_ = 0;
                lost_ = 0;
                return Action::Engage;
            }
            return Action::None;
        }

        if (r4 < config_.release_r4) {
            if (lost_ < 0xFFFF)
                ++lost_;
        } else {
            lost_ = 0;
        }
        if (lost_ > config_.release_frames) {
            engaged_ = false;
            lost_ = 0;
            streak_ = 0;
            holdoff_ = config_.holdoff;
            ++releases_;
            return Action::Release;
        }
        return Action::None;
    }

    bool wanted() const { return wanted_; }
    bool engaged() const { return engaged_; }
    uint16_t holdoff() const { return holdoff_; }
    uint32_t releases() const { return releases_; }

   private:
    Config config_;
    bool wanted_{false};
    bool engaged_{false};
    uint16_t streak_{0};
    uint16_t lost_{0};
    uint16_t holdoff_{0};
    uint32_t releases_{0};
};

/* r4 is ~20 on a dead constellation, ~700 on a live one. Engaging inside
 * acquisition starts a limit cycle, hence the holdoff. */
constexpr Engagement::Config kEngageDefaults{30, 8, 120, 60, 64};

} /* namespace dab */

#endif /*__DAB_SI_H__*/
