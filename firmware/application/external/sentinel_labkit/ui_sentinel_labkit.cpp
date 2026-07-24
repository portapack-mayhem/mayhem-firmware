/*
 * ui_sentinel_labkit.cpp — Sentinel labkit PortaPack views (GPLv2-or-later).
 *
 * See ui_sentinel_labkit.hpp. The TX bring-up mirrors the in-tree Signal Gen app
 * (baseband::run_prepared_image + transmitter_model + baseband::set_siggen_*), but
 * every emission first passes core::pre_emit_gate(). String formatting uses snprintf
 * to stay independent of firmware string helpers.
 */
#include "ui_sentinel_labkit.hpp"

#include "baseband_api.hpp"
#include "ui_textentry.hpp"  // text_prompt (verify signature against your tree — see docs)

#include <cstdio>

using namespace portapack;

namespace ui::external_app::sentinel_labkit {

static std::string fnum(float v, int prec = 3) {
    char b[32];
    std::snprintf(b, sizeof(b), "%.*f", prec, v);
    return std::string(b);
}

// ---------------------------------------------------------------- BandsView
BandsView::BandsView(NavigationView& nav) : nav_{nav} {
    add_children({&console, &button_done});
    console.writeln("HARD GNSS DENYLIST (no override):");
    for (int i = 0; i < core::GNSS_DENYLIST_COUNT; ++i) {
        const auto& r = core::GNSS_DENYLIST[i];
        console.writeln(fnum(r.low_mhz, 1) + "-" + fnum(r.high_mhz, 1) + " MHz");
        console.writeln("  " + std::string(r.label));
    }
    console.writeln("");
    console.writeln("Reference ISM/control bands");
    console.writeln("(NOT a grant of authority):");
    for (int i = 0; i < core::EXAMPLE_CONTROL_BANDS_COUNT; ++i) {
        const auto& b = core::EXAMPLE_CONTROL_BANDS[i];
        console.writeln(std::string(b.key) + ": " + fnum(b.range.low_mhz, 1) + "-" +
                        fnum(b.range.high_mhz, 1));
    }
    button_done.on_select = [this](Button&) { nav_.pop(); };
}

void BandsView::focus() { button_done.focus(); }

// ---------------------------------------------------------------- AttestView
AttestView::AttestView(NavigationView& nav, AttestState& state)
    : nav_{nav}, state_{state} {
    add_children({&labels, &text_op, &button_set_op, &checkbox_affirm, &button_done});
    checkbox_affirm.set_value(state_.affirmed);

    button_set_op.on_select = [this](Button&) {
        text_prompt(nav_, state_.operator_id, 10, ENTER_KEYBOARD_MODE_ALPHA,
                    [this](std::string&) { refresh(); });
    };
    checkbox_affirm.on_select = [this](Checkbox&, bool v) {
        state_.affirmed = v;
        refresh();
    };
    button_done.on_select = [this](Button&) { nav_.pop(); };
    refresh();
}

void AttestView::refresh() {
    std::string op = state_.operator_id.empty() ? std::string("(unset)") : state_.operator_id;
    text_op.set("Operator: " + op);
    set_dirty();
}

void AttestView::focus() { button_set_op.focus(); }

// ---------------------------------------------------------------- root view
SentinelLabkitView::SentinelLabkitView(NavigationView& nav) : nav_{nav} {
    // Bundled M4 baseband (proc_siggen — see external.cmake). Same call as siggen.
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&labels, &field_waveform, &field_center, &field_bw_mhz, &field_sr,
                  &field_power_dbm, &field_duration_s, &checkbox_arm, &checkbox_owned,
                  &console, &button_validate, &button_emit, &button_bands, &button_attest});

    // Defaults: 2.44 GHz, 5 MHz BW, 20 MS/s, 0 dBm, 2 s, band-noise.
    field_waveform.set_by_value(1);
    field_center.set_value(2440000000);
    field_bw_mhz.set_value(5);
    field_sr.set_by_value(20000000);
    field_power_dbm.set_value(0);
    field_duration_s.set_value(2);

    // Any field change re-runs the dry-run verdict (no RF).
    field_waveform.on_change = [this](size_t, OptionsField::value_t) { update_verdict(); };
    field_center.on_change = [this](rf::Frequency) { update_verdict(); };
    field_bw_mhz.on_change = [this](int32_t) { update_verdict(); };
    field_sr.on_change = [this](size_t, OptionsField::value_t) { update_verdict(); };
    field_power_dbm.on_change = [this](int32_t) { update_verdict(); };
    field_duration_s.on_change = [this](int32_t) { update_verdict(); };

    button_validate.on_select = [this](Button&) { update_verdict(); };
    button_emit.on_select = [this](Button&) { on_emit(); };
    button_bands.on_select = [this](Button&) { nav_.push<BandsView>(); };
    button_attest.on_select = [this](Button&) { nav_.push<AttestView>(attest_state_); };

    update_verdict();
}

SentinelLabkitView::~SentinelLabkitView() {
    transmitter_model.disable();
    baseband::shutdown();
}

void SentinelLabkitView::focus() { button_validate.focus(); }

uint32_t SentinelLabkitView::sample_rate_hz() const {
    return static_cast<uint32_t>(field_sr.selected_index_value());
}

core::EffectPlan SentinelLabkitView::plan_from_fields() const {
    core::EffectPlan p;
    switch (field_waveform.selected_index_value()) {
        case 0: p.waveform = core::Waveform::CW; break;
        case 2: p.waveform = core::Waveform::SWEEP; break;
        default: p.waveform = core::Waveform::BAND_NOISE; break;
    }
    p.center_mhz = static_cast<float>(field_center.value()) / 1e6f;
    p.bandwidth_mhz = static_cast<float>(field_bw_mhz.value());
    p.power_dbm = static_cast<float>(field_power_dbm.value());
    p.duration_s = static_cast<float>(field_duration_s.value());
    return p;
}

core::GateResult SentinelLabkitView::evaluate() const {
    return core::pre_emit_gate(plan_from_fields(), static_cast<float>(sample_rate_hz()),
                               attest_state_.as_attestation());
}

void SentinelLabkitView::update_verdict() {
    const core::EffectPlan p = plan_from_fields();
    const core::GateResult g = evaluate();

    console.clear(true);
    console.writeln(std::string(core::waveform_name(p.waveform)) + " @ " +
                    fnum(p.center_mhz, 3) + " MHz");
    console.writeln("BW " + fnum(p.bandwidth_mhz, 1) + " SR " +
                    fnum(sample_rate_hz() / 1e6f, 0) + "M " +
                    fnum(p.power_dbm, 0) + "dBm " + fnum(p.duration_s, 0) + "s");
    console.writeln("Emitted " + fnum(g.emitted_low_mhz, 2) + "-" +
                    fnum(g.emitted_high_mhz, 2) + " MHz");
    console.writeln(g.gnss_blocked ? "GNSS: BLOCKED" : "GNSS: clear");
    console.writeln(std::string("Attest: ") +
                    (attest_state_.affirmed ? "affirmed" : "NOT affirmed"));
    console.writeln(g.allowed ? "GATE: OK (armed run emits)"
                              : (std::string("GATE blocked: ") + g.reason));
    set_dirty();
}

void SentinelLabkitView::configure_baseband_for_waveform() {
    const core::EffectPlan p = plan_from_fields();
    const uint32_t bw = static_cast<uint32_t>(p.bandwidth_mhz * 1e6f);   // proc shaping (calibrate)
    const uint32_t duration = static_cast<uint32_t>(p.duration_s);       // seconds; 0 = continuous
    uint32_t shape;
    switch (p.waveform) {
        case core::Waveform::CW:         shape = (0u << 4) | 0u; break;  // mod=CW
        case core::Waveform::SWEEP:      shape = (1u << 4) | 2u; break;  // FM + saw-up ~ sweep
        case core::Waveform::BAND_NOISE:
        default:                         shape = (4u << 4) | 5u; break;  // DSB + pseudo-noise
    }
    if (p.waveform == core::Waveform::SWEEP) {
        baseband::set_siggen_tone(bw);   // sweep span driven by bandwidth (calibrate on bench)
    }
    baseband::set_siggen_config(bw, shape, duration);
}

void SentinelLabkitView::on_emit() {
    if (transmitting_) {                 // toggle: EMIT acts as STOP while transmitting
        stop_tx();
        return;
    }

    // 1) The gate — GNSS emitted-footprint block + attestation + plan validity. No bypass.
    const core::GateResult g = evaluate();
    if (!g.allowed) {
        nav_.display_modal("Blocked", g.reason);
        update_verdict();
        return;
    }
    // 2) Dry-run is the default: real emission needs BOTH arm checkboxes explicitly.
    if (!checkbox_arm.value() || !checkbox_owned.value()) {
        nav_.display_modal("Not armed",
                           "Tick 'Arm TX' and 'Owned tgt' to emit. Default is dry-run.");
        return;
    }

    // 3) Configure the radio from the plan, then enable(). The firmware's own TX kill
    //    switch (config_tx_disabled) still applies beneath this app's gate.
    const core::EffectPlan p = plan_from_fields();
    transmitter_model.set_target_frequency(static_cast<rf::Frequency>(p.center_mhz * 1e6f));
    transmitter_model.set_sampling_rate(sample_rate_hz());
    transmitter_model.set_baseband_bandwidth(transmitter_model.baseband_bandwidth());
    transmitter_model.set_tx_gain(static_cast<uint8_t>(core::power_dbm_to_tx_gain(p.power_dbm)));
    transmitter_model.set_rf_amp(false);   // +14 dB PA stays off; operator opts in per SOP
    transmitter_model.enable();

    configure_baseband_for_waveform();

    transmitting_ = true;
    button_emit.set_text("STOP");
    console.writeln("EMITTING (tx_gain " +
                    std::to_string(core::power_dbm_to_tx_gain(p.power_dbm)) + ")");
    set_dirty();
}

void SentinelLabkitView::stop_tx() {
    transmitter_model.disable();
    transmitting_ = false;
    button_emit.set_text("EMIT");
    console.writeln("stopped");
    set_dirty();
}

void SentinelLabkitView::on_tx_progress(uint32_t progress, bool done) {
    (void)progress;
    if (done && transmitting_) {
        stop_tx();
        console.writeln("emission complete");
    }
}

}  // namespace ui::external_app::sentinel_labkit
