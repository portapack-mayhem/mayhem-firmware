/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2023 gullradriel, Nilorea Studio Inc.
 * Copyright (C) 2023 Kyle Reed
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

#ifndef __FREQMAN_H__
#define __FREQMAN_H__

#include "freqman_db.hpp"

// Defined for back-compat.
#define FREQMAN_MAX_PER_FILE freqman_default_max_entries

enum freqman_error : int8_t {
    NO_ERROR = 0,
    ERROR_ACCESS,
    ERROR_NOFILES,
    ERROR_DUPLICATE
};

enum freqman_entry_modulation : uint8_t {
    AM_MODULATION = 0,
    NFM_MODULATION,
    WFM_MODULATION,
    AMFM_MODULATION,  // Added for Wefax.
    SPEC_MODULATION
};

// TODO: Can these be removed after Recon is migrated to FreqmanDB?
int32_t freqman_entry_get_step_value(freqman_index_t step);

#endif /*__FREQMAN_H__*/
