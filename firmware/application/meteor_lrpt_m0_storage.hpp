/*
 * Copyright (C) 2026
 *
 * Meteor LRPT M0 runtime storage in the external-app AHB slot tail (not .bss / heap).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef APPLICATION_METEOR_LRPT_M0_STORAGE_HPP
#define APPLICATION_METEOR_LRPT_M0_STORAGE_HPP

#include <cstddef>
#include <cstdint>

namespace ui {
class Color;
class NavigationView;
}

namespace ui::external_app::meteor_lrpt_rx {

class MeteorLrptRxView;

ui::Color* meteor_lrpt_preview_line_buffer();
void meteor_lrpt_push_view(NavigationView& nav);
void meteor_lrpt_load_pm_flags(uint8_t& flags);
void meteor_lrpt_save_pm_flags(uint8_t flags);
uint8_t lrpt_flags_from_pm(uint8_t pm);

constexpr size_t kMeteorM0SlotBytes = 10240u;
/** Preview line + MeteorLrptRxView at slot tail; keep .ppma M0 code below this. */
constexpr size_t kMeteorM0TailReserveBytes = 3072u;
constexpr size_t kMeteorM0PackagedCodeMaxBytes = kMeteorM0SlotBytes - kMeteorM0TailReserveBytes;

}  // namespace ui::external_app::meteor_lrpt_rx

#endif /* APPLICATION_METEOR_LRPT_M0_STORAGE_HPP */
