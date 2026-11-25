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

#include "dab_cache.hpp"

#include <cstdio>
#include <cstring>
#include <vector>

namespace dab {

namespace {

/* Splits on unescaped commas. */
bool split_fields(const char* line, std::vector<std::string>& fields) {
    fields.clear();
    std::string current;
    bool escaped = false;

    for (const char* p = line; *p != '\0'; ++p) {
        if (*p == '\r' || *p == '\n')
            break;

        if (escaped) {
            current.push_back(*p);
            escaped = false;
            continue;
        }

        if (*p == '\\') {
            current.push_back(*p);
            escaped = true;
            continue;
        }

        if (*p == ',') {
            fields.push_back(current);
            current.clear();
            continue;
        }

        current.push_back(*p);
    }

    if (escaped)
        return false;  // trailing backslash: malformed

    fields.push_back(current);
    return true;
}

bool parse_u32(const std::string& s, uint32_t& out) {
    /* Ten digits is the widest a uint32 can be. */
    if (s.empty() || s.size() > 10)
        return false;
    for (char c : s) {
        if (c < '0' || c > '9')
            return false;
    }
    /* Not strtoul: on the 32-bit target it saturates at ULONG_MAX, so overflow
     * would go unnoticed. */
    uint64_t v = 0;
    for (char c : s) {
        v = v * 10 + static_cast<uint64_t>(c - '0');
        if (v > 0xFFFFFFFFull)
            return false;
    }

    out = static_cast<uint32_t>(v);
    return true;
}

}  // namespace

std::string escape_field(const std::string& value) {
    std::string out;
    for (char c : value) {
        if (c == '\\' || c == ',')
            out.push_back('\\');
        out.push_back(c);
    }
    return out;
}

std::string unescape_field(const std::string& value) {
    std::string out;
    bool escaped = false;
    for (char c : value) {
        if (escaped) {
            out.push_back(c);
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else {
            out.push_back(c);
        }
    }
    return out;
}

size_t serialise_ensemble(const EnsembleDatabase& db,
                          const char* block,
                          uint32_t timestamp,
                          std::string& out) {
    if (block == nullptr)
        return 0;

    size_t written = 0;
    char numbuf[64];

    for (size_t i = 0; i < db.service_count(); ++i) {
        const DabService& s = db.services()[i];
        if (!s.is_playable())
            continue;

        out += escape_field(block);
        out += ',';

        std::snprintf(numbuf, sizeof(numbuf), "%u", static_cast<unsigned>(db.ensemble_id()));
        out += numbuf;
        out += ',';

        out += escape_field(db.ensemble_label());
        out += ',';

        std::snprintf(numbuf, sizeof(numbuf), "%u", static_cast<unsigned>(s.sid));
        out += numbuf;
        out += ',';

        out += escape_field(s.label);
        out += ',';

        std::snprintf(numbuf, sizeof(numbuf), "%u,%u,%u,%u,%u,%u,%u,%u",
                      static_cast<unsigned>(s.subch_id),
                      static_cast<unsigned>(s.start_cu),
                      static_cast<unsigned>(s.size_cu),
                      static_cast<unsigned>(s.protection.level),
                      static_cast<unsigned>(s.protection.option_b ? 1 : 0),
                      static_cast<unsigned>(s.bitrate),
                      static_cast<unsigned>(s.is_dab_plus ? 1 : 0),
                      static_cast<unsigned>(timestamp));
        out += numbuf;
        out += '\n';

        ++written;
    }

    return written;
}

bool parse_cache_line(const char* line, CachedService& out) {
    if (line == nullptr || line[0] == '\0' || line[0] == '#')
        return false;

    std::vector<std::string> f;
    if (!split_fields(line, f))
        return false;
    if (f.size() != 13)
        return false;

    CachedService parsed;
    parsed.block = unescape_field(f[0]);
    if (parsed.block.empty())
        return false;

    uint32_t v = 0;

    if (!parse_u32(f[1], v) || v > 0xFFFF)
        return false;
    parsed.eid = static_cast<uint16_t>(v);

    parsed.ensemble_label = unescape_field(f[2]);

    if (!parse_u32(f[3], v))
        return false;
    parsed.sid = v;

    parsed.service_label = unescape_field(f[4]);
    if (parsed.service_label.empty())
        return false;

    if (!parse_u32(f[5], v) || v > 63)
        return false;
    parsed.subch_id = static_cast<uint8_t>(v);

    /* A CIF is 864 CUs. */
    if (!parse_u32(f[6], v) || v > 863)
        return false;
    parsed.start_cu = static_cast<uint16_t>(v);

    if (!parse_u32(f[7], v) || v == 0 || v > 864)
        return false;
    parsed.size_cu = static_cast<uint16_t>(v);

    if (!parse_u32(f[8], v) || v < 1 || v > 4)
        return false;
    parsed.protection.kind = ProtectionKind::Eep;
    parsed.protection.level = static_cast<uint8_t>(v);

    if (!parse_u32(f[9], v) || v > 1)
        return false;
    parsed.protection.option_b = (v == 1);

    if (!parse_u32(f[10], v))
        return false;
    parsed.bitrate = v;

    if (!parse_u32(f[11], v) || v > 1)
        return false;
    parsed.is_dab_plus = (v == 1);

    if (!parse_u32(f[12], v))
        return false;
    parsed.last_seen = v;

    out = parsed;
    return true;
}

} /* namespace dab */
