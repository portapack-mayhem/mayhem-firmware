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

/* MPEG-4 AudioSpecificConfig from a DAB+ superframe header (ETSI TS 102 563).
 * Unlike plain AAC, DAB+ uses a 960-sample transform (frameLengthFlag set; a
 * 1024-only decoder cannot play it), and with SBR the config carries the core
 * rate, half the output rate. */

#ifndef __DAB_AAC_H__
#define __DAB_AAC_H__

#include "dab_superframe.hpp"

#include <cstddef>
#include <cstdint>

namespace dab {

constexpr uint8_t kAacObjectTypeLc = 2;

struct AacConfig {
    uint8_t bytes[2]{};       // AudioSpecificConfig
    uint8_t length{0};        // always 2 when valid
    uint32_t core_rate{0};    // the rate the config declares
    uint32_t output_rate{0};  // after SBR, if any
    uint8_t channels{0};      // 1 or 2
    bool sbr{false};
    bool parametric_stereo{false};
    bool valid{false};
};

/* 0x0F if the rate is not a standard one. */
uint8_t aac_sampling_frequency_index(uint32_t rate);

AacConfig aac_config_from_superframe(const SuperframeHeader& header);

} /* namespace dab */

#endif /*__DAB_AAC_H__*/
