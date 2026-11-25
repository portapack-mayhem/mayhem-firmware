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

/* FIB/FIG parsing per ETSI EN 300 401 clauses 5.2 and 8. A FIB is 30 bytes of
 * FIGs (3-bit type, 5-bit length, 0xFF ends) then a CRC-16. Handles FIG 0/0,
 * 0/1, 0/2, 0/8, 0/9, 0/10, 0/13, 0/17, 1/0, 1/1, 1/5; the rest are counted
 * via on_unhandled. A FIG claiming to run past its FIB is discarded whole. */

#ifndef __DAB_FIG_H__
#define __DAB_FIG_H__

#include "dab_constants.hpp"
#include "dab_protection.hpp"

#include <cstddef>
#include <cstdint>

namespace dab {

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

} /* namespace dab */

#endif /*__DAB_FIG_H__*/
