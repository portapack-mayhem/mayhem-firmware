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

#ifndef __CONFIG_MODE_H__
#define __CONFIG_MODE_H__

#include "ch.h"

#include "portapack.hpp"
#include "portapack_shared_memory.hpp"
#include "portapack_persistent_memory.hpp"

// number of boot failures before entering config menu mode
#define BOOT_FAILURES_BEFORE_CONFIG_MODE 3

#define CONFIG_MODE_GUARD_VALUE 0xbadb0000
#define CONFIG_MODE_LIMIT_VALUE (CONFIG_MODE_GUARD_VALUE + BOOT_FAILURES_BEFORE_CONFIG_MODE - 1)
#define CONFIG_MODE_NORMAL_VALUE 0x000007cf

void config_mode_set();
bool config_mode_should_enter();
void config_mode_clear();
void config_mode_enable(bool v);
bool config_mode_disabled();

void config_mode_run();

#endif /* __CONFIG_MODE_H__ */
