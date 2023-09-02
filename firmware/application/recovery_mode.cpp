/*
 * Copyright (C) 2023 Bernd Herzog
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

#include "recovery_mode.hpp"
#include "core_control.hpp"

#define RECOVERY_MODE_GUARD_VALUE 2001
#define RECOVERY_MODE_NORMAL_VALUE 1999

void recovery_mode_set() {
    portapack::persistent_memory::set_recovery_mode_storage(RECOVERY_MODE_GUARD_VALUE);
}

bool recovery_mode_should_enter() {
    return portapack::persistent_memory::recovery_mode_storage() == RECOVERY_MODE_GUARD_VALUE;
}

void recovery_mode_clear() {
    portapack::persistent_memory::set_recovery_mode_storage(RECOVERY_MODE_NORMAL_VALUE);
}

void recovery_mode_run() {
    m0_halt();
}
