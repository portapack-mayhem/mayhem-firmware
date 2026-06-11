/*
 * Copyright (C) 2026
 *
 * Gardner-style symbol timing loop gains (Meteor LRPT M4 demod). Catmull–Rom helpers
 * live in `meteor_polyphase_interp.hpp`.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef METEOR_LRPT_METEOR_SYMBOL_TIMING_HPP
#define METEOR_LRPT_METEOR_SYMBOL_TIMING_HPP

namespace meteor_lrpt {

/** Default PI-style gains for TED → rate fine path in `MeteorLrptRx::execute`. */
struct MeteorLrptSymbolTimingGains {
    /** Integrator coefficient on Gardner TED (state `sym_pll_i_`). */
    float pll_i_gain{1e-6f};
    /** Proportional term scaling TED → `sym_rate_fine_`. */
    float rate_prop_gain{2e-4f};
    /** Absolute clamp on combined rate adjustment per symbol strobe. */
    float rate_clamp{0.04f};
    /** TED scale applied before gains (matches in-proc `1.f / 4194304.f`). */
    float ted_scale{1.f / 4194304.f};
};

}  // namespace meteor_lrpt

#endif
