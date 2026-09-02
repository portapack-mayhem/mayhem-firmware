/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "ch.h"
#include "baseband_api.hpp"
#include "core_control.hpp"
#include "hal.h"
#include "lpc43xx_cpp.hpp"
#include "lz4.h"
#include "message.hpp"
#include "file.hpp"

#include <cstring>
#include <new>
#include <memory>
#include <string>

using namespace lpc43xx;
using namespace portapack;

// Start the M4 from an already-in-RAM chunk (either the SPI-flash image or one
// read from the SD card). Both are the same on-disk chunk_t layout, so the load is
// identical: LZ4-decompress into the M4 code region, remap, reset the M4.
static void m4_start_from_chunk(const spi_flash::chunk_t* chunk,
                                const memory::region_t to,
                                const bool full_reset) {
    /* extract and initialize M4 code RAM */
    unlz4_len(&chunk->data[0], reinterpret_cast<void*>(to.base()), chunk->compressed_data_size);

    /* M4 core is assumed to be sleeping with interrupts off, so we can mess
     * with its address space and RAM without concern.
     */
    LPC_CREG->M4MEMMAP = to.base();

    /* Reset M4 core and optionally all peripherals */
    LPC_RGU->RESET_CTRL[0] = (full_reset) ? (1 << 1)    // PERIPH_RST
                                          : (1 << 13);  // M4_RST
}

// Fallback for basebands moved out of the (full) SPI flash to the SD card. Reads
// /BASEBAND/<TAG>.img - which is byte-identical to a flash chunk_t
// ([tag][length][compressed_data_size][lz4 data], see tools/make_image_chunk.py) -
// into a RAM buffer, validates the tag, and starts the M4 from it. Returns false
// (-> caller panics NoImg, i.e. the pre-existing behaviour) if the file is
// absent/short/mismatched, so this only ever ADDS the ability to load; a baseband
// still present in flash never reaches here.
//
// The reading is done on a thread of its own, and that is the whole point of the
// arrangement rather than a flourish. This runs at the deepest place an app launch
// reaches (view constructor -> start_rx -> run_image -> here) on the 4 kB process
// stack that the whole UI shares, and FatFs wants about half a kilobyte on top:
// f_open 144, find_volume 72, follow_path 48, dir_find 64, the sector read 104.
// Measured on the device, the process stack has around 300 bytes left at its
// high-water mark after boot, and under a hundred once a few screens have been
// opened. Reading the file on the caller's stack therefore worked for a shallow app
// and put the deep ones - Looking Glass, Weather - into a hard fault, which is what
// prompted this. On its own 1 kB stack the caller's depth stops mattering. The same
// size and the same call are what capture_thread and replay_thread already use to
// touch the card.

struct SdImageLoad {
    spi_flash::image_tag_t tag;
    uint8_t* raw{nullptr};
    size_t size{0};
};

static msg_t m4_load_from_sd_thread(void* arg) {
    auto* const req = static_cast<SdImageLoad*>(arg);

    // image_tag_t is a 4-byte {char c[4]}; read its bytes to build "<TAG>.img".
    const char* tc = reinterpret_cast<const char*>(&req->tag);
    std::u16string path = u"/BASEBAND/";
    for (int i = 0; i < 4; ++i) path += static_cast<char16_t>(static_cast<uint8_t>(tc[i]));
    path += u".img";

    // File carries FatFs' 512-byte sector buffer. Off the stack, because this thread's
    // kilobyte is meant for the FatFs call frames rather than for a 560-byte local -
    // and off operator new, because that panics with "Out of Memory" instead of
    // returning null, and a baseband that will not load is meant to be a NoImg, not a
    // dead device. Same reasoning as the image buffer below.
    // Freed by the guard below rather than at each exit: this function returns from
    // seven places, and one of them forgotten later is a leak nobody would notice.
    struct FileDeleter {
        void operator()(File* p) const {
            if (!p) return;
            p->~File();
            chHeapFree(p);
        }
    };
    void* const fmem = chHeapAlloc(0x0, sizeof(File));
    if (!fmem) return 0;
    std::unique_ptr<File, FileDeleter> f{new (fmem) File()};

    if (f->open(std::filesystem::path{path})) return 0;  // Optional<Error> truthy = failed
    const uint64_t fsize = f->size();
    if (fsize < 13 || fsize > 48u * 1024u) return 0;  // header(12)+data; M4 region is 32 KiB

    // chHeapAlloc rather than new: this firmware's operator new panics with "Out of
    // Memory" when the heap is short, and a baseband that will not fit is meant to be
    // a NoImg - the same thing that happens when the file is missing - not a dead
    // device. Core memory is handed out and never returned, so whether ten kilobytes
    // are available here depends on which apps have already run this session; a
    // restart is what gets them back.
    //
    // chHeapAlloc is 8-byte aligned, so the uint32 chunk fields stay word-aligned.
    uint8_t* const raw = static_cast<uint8_t*>(chHeapAlloc(0x0, static_cast<size_t>(fsize)));
    if (!raw) return 0;
    const auto rd = f->read(raw, fsize);
    if (!rd || rd.value() != fsize) {
        chHeapFree(raw);
        return 0;
    }

    // The header has to agree with the file before any of it is believed. The tag alone
    // is four bytes an attacker or a bad sector can match by accident, while length and
    // compressed_data_size steer unlz4_len: either one larger than what was read walks
    // the decompressor off the end of this buffer.
    const auto* const head = reinterpret_cast<const spi_flash::chunk_t*>(raw);
    // Derived rather than written as 12, so that a change to chunk_t cannot leave this
    // silently measuring the wrong thing. length has to match the file exactly: a file
    // cut short mid-copy keeps a header whose fields still agree with each other.
    constexpr size_t header_size = sizeof(spi_flash::image_tag_t) + sizeof(uint32_t) * 2;
    if (static_cast<size_t>(fsize) < header_size) {
        chHeapFree(raw);
        return 0;
    }
    const size_t body = static_cast<size_t>(fsize) - header_size;
    if (head->length != body || head->compressed_data_size > body) {
        chHeapFree(raw);
        return 0;
    }

    req->raw = raw;
    req->size = static_cast<size_t>(fsize);
    return 1;
}

static bool m4_init_from_sd(const spi_flash::image_tag_t image_tag,
                            const memory::region_t to,
                            const bool full_reset) {
    SdImageLoad req{};
    req.tag = image_tag;

    // A kilobyte we cannot get is a NoImg, exactly as a missing file is: the device
    // says so and stays alive, rather than reading the card on a stack that has no
    // room for it.
    Thread* const tp = chThdCreateFromHeap(NULL, 1024, NORMALPRIO + 10,
                                           m4_load_from_sd_thread, &req);
    if (!tp) return false;
    chThdWait(tp);  // also returns the working area to the heap
    if (!req.raw) return false;

    const auto* chunk = reinterpret_cast<const spi_flash::chunk_t*>(req.raw);
    if (!(chunk->tag == image_tag)) {
        chHeapFree(req.raw);
        return false;
    }
    m4_start_from_chunk(chunk, to, full_reset);
    // The M4 is running from its own RAM now; the compressed copy has done its job.
    chHeapFree(req.raw);
    return true;
}

void m4_init(const spi_flash::image_tag_t image_tag, const memory::region_t to, const bool full_reset) {
    const spi_flash::chunk_t* chunk = reinterpret_cast<const spi_flash::chunk_t*>(spi_flash::images.base());
    while (chunk->tag) {
        if (chunk->tag == image_tag) {
            m4_start_from_chunk(chunk, to, full_reset);
            return;
        }
        chunk = chunk->next();
    }

    // Not embedded in the SPI flash → try the SD card (baseband moved off to free flash).
    if (m4_init_from_sd(image_tag, to, full_reset)) return;

    chDbgPanic("NoImg");
}

void m4_init_prepared(const uint32_t m4_code, const bool full_reset) {
    /* M4 core is assumed to be sleeping with interrupts off, so we can mess
     * with its address space and RAM without concern.
     */
    LPC_CREG->M4MEMMAP = m4_code;

    /* Reset M4 core and optionally all peripherals */
    LPC_RGU->RESET_CTRL[0] = (full_reset) ? (1 << 1)    // PERIPH_RST
                                          : (1 << 13);  // M4_RST

    return;
}

void m4_request_shutdown() {
    baseband::shutdown();
}

void m0_halt() {
    rgu::reset(rgu::Reset::M0APP);
    while (true) {
        port_wait_for_interrupt();
    }
}
