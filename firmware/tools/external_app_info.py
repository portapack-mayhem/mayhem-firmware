#!/usr/bin/env python3

#
# Copyright (C) 2024 Mark Thompson
#
# This file is part of PortaPack.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#

# external app address ranges below must match those in linker file "external.ld"
# Packaged .ppma size = M0 external section + optional M4 chunk (see export_external_apps.py).
# Meteor LRPT (PMLR) includes a large M4 baseband image; 32 KiB was insufficient.
maximum_application_size = 48 * 1024
external_apps_address_start = 0xADB00000
external_apps_address_end = 0xAE080000

# M0 external .ppma with M4 baseband: reserve the tail of M0 AHB SRAM.
# Do NOT use local SRAM0 (0x10000000): M4 baseband uses it for RAM.
# Do NOT use bank2 after m4_code (0x10086800): that is SharedMemory for M4 IPC.
m0_ahb_base = 0x20000000
m0_ahb_size_bytes = 64 * 1024
external_m0_runtime_slot_bytes = 10 * 1024
external_m0_load_base_with_m4 = m0_ahb_base + m0_ahb_size_bytes - external_m0_runtime_slot_bytes
external_m0_max_bytes = external_m0_runtime_slot_bytes
