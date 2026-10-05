/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __MEMORY_MAP_H__
#define __MEMORY_MAP_H__

#include <cstddef>
#include <cstdint>

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

#include "utility.hpp"

namespace portapack {
namespace memory {

struct region_t {
   public:
    constexpr region_t(
        const uint32_t base,
        const size_t size)
        : base_{base},
          size_{size} {
    }

    constexpr uint32_t base() const {
        return base_;
    }

    constexpr uint32_t end() const {
        return base_ + size_;
    }

    constexpr size_t size() const {
        return size_;
    }

   private:
    const uint32_t base_;
    const size_t size_;
};

namespace map {

/* PORTAPACK_BOARD_PRALINE comes from "rules.cmake", next to the memory sizes it
 * selects; PRALINE comes from -D${BOARD} in the application and baseband
 * CMakeLists. A translation unit that saw one but not the other would build a
 * different memory map to the rest of the firmware.
 */
#if defined(PRALINE) != defined(PORTAPACK_BOARD_PRALINE)
#error "PRALINE and PORTAPACK_BOARD_PRALINE disagree, this TU would get the wrong memory map"
#endif

/* Local SRAM bank sizes differ per part: LPC4320 (HackRF One) has 96 KiB in
 * bank 1 and 40 KiB in bank 2, LPC4330 (PRALINE) has 128 KiB and 72 KiB.
 * These must match M4_RAM_SIZE / M4_FLASH_SIZE in "rules.cmake".
 */
#if defined(PORTAPACK_BOARD_PRALINE)
constexpr region_t local_sram_0{0x10000000, 128_KiB};
constexpr region_t local_sram_1{0x10080000, 72_KiB};
#else
constexpr region_t local_sram_0{0x10000000, 96_KiB};
constexpr region_t local_sram_1{0x10080000, 40_KiB};
#endif

constexpr region_t ahb_ram_0{0x20000000, 32_KiB};
constexpr region_t ahb_ram_1{0x20008000, 16_KiB};
constexpr region_t ahb_ram_2{0x2000c000, 16_KiB};

constexpr region_t backup_ram{LPC_BACKUP_REG_BASE, 256};

constexpr region_t spifi_uncached{LPC_SPIFI_DATA_BASE, FLASH_SIZE_MB * 1024 * 1024};
constexpr region_t spifi_cached{LPC_SPIFI_DATA_CACHED_BASE, spifi_uncached.size()};

/////////////////////////////////

/* An external app's M0 code and the M4 baseband image it runs both live in
 * m4_code, so its size is the budget the two of them share (enforced by
 * "export_external_apps.py", which is given the size by "rules.cmake").
 *
 * On the LPC4330 bank 2 has 72 KiB, so 64 KiB is m4_code and the shared memory
 * fits above it. On the LPC4320 bank 2 has only 40 KiB: all of it is m4_code
 * and the shared memory sits in the top 8 KiB of bank 1, which M4_RAM_SIZE in
 * "rules.cmake" is shortened by the same amount to make room for.
 */
#if defined(PORTAPACK_BOARD_PRALINE)
constexpr region_t m4_code{local_sram_1.base(), 64_KiB};
constexpr region_t shared_memory{m4_code.end(), 8_KiB};
#else
constexpr region_t m4_code{local_sram_1.base(), 40_KiB};
constexpr region_t shared_memory{local_sram_0.end() - 8_KiB, 8_KiB};
#endif

/* CMake passes the same size to the packer, which uses it to reject an app
 * that does not fit at run time.
 */
#if defined(PORTAPACK_M4_CODE_SIZE)
static_assert(m4_code.size() == PORTAPACK_M4_CODE_SIZE,
              "m4_code size disagrees with M4_CODE_SIZE in rules.cmake");
#endif
static_assert(m4_code.base() >= local_sram_1.base() &&
                  m4_code.end() <= local_sram_1.end(),
              "m4_code does not fit in local SRAM bank 2");

/* The M4 linker RAM region has to fit in bank 1 on every board. */
#if defined(PORTAPACK_M4_RAM_SIZE)
static_assert(PORTAPACK_M4_RAM_SIZE <= local_sram_0.size(),
              "M4_RAM_SIZE in rules.cmake does not fit in local SRAM bank 1");
#endif

/* The shared memory sits above whatever the M4 linker is told its RAM is. If
 * the two drift apart the M4 heap grows into the message queues.
 */
#if !defined(PORTAPACK_BOARD_PRALINE) && defined(PORTAPACK_M4_RAM_SIZE)
static_assert(shared_memory.base() >= local_sram_0.base() + PORTAPACK_M4_RAM_SIZE,
              "shared memory overlaps the M4 RAM region (M4_RAM_SIZE in rules.cmake)");
static_assert(shared_memory.end() <= local_sram_0.end(),
              "shared memory does not fit in local SRAM bank 1");
#endif

constexpr region_t m4_code_hackrf = local_sram_0;

} /* namespace map */
} /* namespace memory */
} /* namespace portapack */

#endif /*__MEMORY_MAP_H__*/
