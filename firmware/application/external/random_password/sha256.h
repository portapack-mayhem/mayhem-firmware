/*
 * Copyright (C) 2026 zxkmm
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

/* FIPS 180-4 SHA-256, streaming, raw bytes in and out.
 *
 * Two notes on why this replaced the previous SHA-512:
 *
 * - The M0 is a 32-bit core with no 64-bit datapath. SHA-512's 64-bit rotates
 *   cost roughly 3x per byte there, and its round-constant table is 640 bytes
 *   against 256. The entropy budget this app enforces is 256 bits, so a
 *   512-bit digest buys nothing that is ever used.
 *
 * - It deals in raw bytes and pulls in no libc formatting. SHA-512 did: it
 *   formatted its digest with sprintf, which is a newlib object in main flash.
 *
 *   Being precise about what that was worth, because it is less than it looks:
 *   checked in application.map, the old sha512.cpp's own code was correctly
 *   namespaced and already lived in this app's 32k RAM bank, NOT in flash. Its
 *   only main-flash cost was pulling in lib_a-sprintf.o -- and removing it did
 *   not free that either, because external/epirb_rx also uses sprintf and now
 *   pulls the same object in. So the flash saving here is currently zero. The
 *   real, measured saving is in this app's bank (SHA-512's k table alone is
 *   640 bytes against 256), plus the speed, which is the actual reason.
 *
 * EVERYTHING here must stay inside namespace ui::external_app::random_password.
 * external.ld places this app's bank with a wildcard over input section names,
 * which under -ffunction-sections keys off the mangled symbol: the pattern
 * wants "ui", then "external_app", then "random_password". A symbol declared
 * at global scope in this directory would miss the pattern and land in main
 * flash instead. Do not move anything out of the namespace.
 *
 * The context is a plain trivially-copyable struct on purpose: the generator
 * needs to finalize a *copy* of the running pool without disturbing it.
 */

#ifndef __RANDOM_PASSWORD_SHA256_H__
#define __RANDOM_PASSWORD_SHA256_H__

#include <cstddef>
#include <cstdint>

namespace ui::external_app::random_password {

class SHA256 {
   public:
    static constexpr size_t digest_size = 32;
    static constexpr size_t block_size = 64;

    void init();
    void update(const void* data, size_t len);
    /* Finalizes into `digest`. The context must not be used afterwards. */
    void final(uint8_t* digest);

   private:
    static const uint32_t k[64];

    void transform(const uint8_t* block);

    uint32_t h[8]{};
    uint64_t total_len{0};
    uint32_t buffer_len{0};
    uint8_t buffer[block_size]{};
};

}  // namespace ui::external_app::random_password

#endif /*__RANDOM_PASSWORD_SHA256_H__*/
