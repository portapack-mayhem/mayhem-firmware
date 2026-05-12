/*
 * Copyright (C) 2026
 *
 * RAM-backed deinterleaver ring for tests / host tooling (see meteor_lrpt_ring_iface.hpp).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef METEOR_LRPT_EXTERNAL_RING_HPP
#define METEOR_LRPT_EXTERNAL_RING_HPP

#include "meteor_lrpt_ring_iface.hpp"

#include <cstddef>
#include <cstdint>

namespace meteor_lrpt {

/** Full ring in RAM (host tests / tooling only — do not instantiate on LPC43 M4 data RAM). */
class MeteorDeintRamRing final : public IMeteorDeintRing {
   public:
    MeteorDeintRamRing();
    ~MeteorDeintRamRing() override;
    void write_byte(uint32_t index_mod, int8_t v) override;
    int8_t read_byte(uint32_t index_mod) const override;

   private:
    int8_t* data_{nullptr};
};

}  // namespace meteor_lrpt

#endif
