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

/* Ensemble cache at /DAB/ensembles.txt, one user-editable line per service:
 *   block,eid,ensemble_label,sid,service_label,subch,start,size,
 *   prot,protb,bitrate,plus,seen
 * Backslash and comma are escaped in labels. Entries are hints only: live FIC
 * overrides them, as subchannel allocations change. */

#ifndef __DAB_CACHE_H__
#define __DAB_CACHE_H__

#include "dab_ensemble.hpp"

#include <cstdint>
#include <string>

namespace dab {

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

} /* namespace dab */

#endif /*__DAB_CACHE_H__*/
