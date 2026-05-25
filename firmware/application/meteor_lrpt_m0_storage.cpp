/*

 * Copyright (C) 2026

 *

 * Meteor LRPT view pool in the M0 external-app slot tail (0x2000D800 region).

 * Keeps ~3–4 KiB out of main .bss so the M0 heap can satisfy RecordView / std::function.

 *

 * SPDX-License-Identifier: GPL-2.0-or-later

 */

#include "meteor_lrpt_m0_storage.hpp"



#include "external/meteor_lrpt_rx/ui_meteor_lrpt_rx.hpp"

#include "memory_map.hpp"

#include "ui_navigation.hpp"



#include "ch.h"

#include "file.hpp"

#include "utility.hpp"



#include <cstddef>

#include <cstdint>

#include <cstdio>

#include <cstring>

#include <memory>

#include <new>

#include <string_view>



namespace ui::external_app::meteor_lrpt_rx {



namespace {



constexpr size_t kSlotBytes = portapack::memory::map::m0_external_app_runtime.size();

constexpr size_t kPreviewBytes = 240u * sizeof(ui::Color);

constexpr size_t kViewBytes = sizeof(MeteorLrptRxView);

constexpr size_t kTailBytes = ((kPreviewBytes + kViewBytes + 7u) / 8u) * 8u;

constexpr uint32_t kTailBase =

    portapack::memory::map::m0_external_app_runtime.base() + static_cast<uint32_t>(kSlotBytes - kTailBytes);

constexpr uint32_t kViewStorage =

    kTailBase + static_cast<uint32_t>(((kPreviewBytes + 7u) / 8u) * 8u);



static_assert(kSlotBytes >= kTailBytes, "Meteor tail storage exceeds M0 slot");
static_assert(kTailBytes <= kMeteorM0TailReserveBytes, "Meteor tail grew past kMeteorM0TailReserveBytes");
static_assert(kSlotBytes - kTailBytes >= kMeteorM0PackagedCodeMaxBytes, "kMeteorM0PackagedCodeMaxBytes mismatch");



ui::Color* preview_storage() {

    return reinterpret_cast<ui::Color*>(kTailBase);

}



void* view_storage() {

    return reinterpret_cast<void*>(kViewStorage);

}



constexpr std::u16string_view kMeteorSettingsPath = u"SETTINGS/rx_meteor_lrpt.ini";



bool parse_lrpt_flags_line(const char* const line, const size_t len, uint8_t& flags) {

    constexpr char kKey[] = "lrpt_flags=";

    constexpr size_t kKeyLen = sizeof(kKey) - 1;

    if (len < kKeyLen || std::memcmp(line, kKey, kKeyLen) != 0)

        return false;

    unsigned v = 0;

    for (size_t i = kKeyLen; i < len; i++) {

        const char c = line[i];

        if (c < '0' || c > '9')

            break;

        v = v * 10u + static_cast<unsigned>(c - '0');

        if (v > 255u)

            return false;

    }

    flags = static_cast<uint8_t>(v);

    return true;

}



}  // namespace



void* MeteorLrptRxView::operator new(const std::size_t size) {

    if (size > kViewBytes)

        chDbgPanic("Meteor view pool");

    return view_storage();

}



void* MeteorLrptRxView::operator new(const std::size_t size, void* const ptr) noexcept {
    (void)size;
    return ptr;
}

void MeteorLrptRxView::operator delete(void* const ptr) noexcept {

    (void)ptr;

}



ui::Color* meteor_lrpt_preview_line_buffer() {

    return preview_storage();

}



void meteor_lrpt_load_pm_flags(uint8_t& flags) {

    File f;

    if (f.open(kMeteorSettingsPath))

        return;



    char buf[96]{};

    const auto rd = f.read(buf, sizeof(buf) - 1);

    if (!rd.is_ok() || rd.value() == 0)

        return;



    const size_t n = static_cast<size_t>(rd.value());

    size_t line_start = 0;

    for (size_t i = 0; i <= n; i++) {

        if (i == n || buf[i] == '\n' || buf[i] == '\r') {

            if (i > line_start) {

                uint8_t parsed = 0;

                if (parse_lrpt_flags_line(buf + line_start, i - line_start, parsed)) {

                    flags = lrpt_flags_from_pm(parsed);

                    return;

                }

            }

            line_start = i + 1;

        }

    }

}



void meteor_lrpt_save_pm_flags(const uint8_t flags) {

    File f;

    ensure_directory(u"SETTINGS");

    if (f.create(kMeteorSettingsPath))

        return;



    char line[32];

    const int n = snprintf(line, sizeof(line), "lrpt_flags=%u\r\n", static_cast<unsigned>(flags));

    if (n <= 0)

        return;

    (void)f.write(line, static_cast<size_t>(n));

}



void meteor_lrpt_push_view(ui::NavigationView& nav) {

    auto view = std::unique_ptr<ui::View>(new (view_storage()) MeteorLrptRxView(nav));

    (void)nav.push_view(std::move(view));

}



}  // namespace ui::external_app::meteor_lrpt_rx

