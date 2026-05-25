/*
 * Copyright (C) 2026
 *
 * This file is part of PortaPack / Mayhem.
 */
#include "ui_meteor_lrpt_rx.hpp"

#include "baseband_api.hpp"
#include "event_m0.hpp"
#include "lcd_ili9341.hpp"
#include "ui_spectrum.hpp"
#include "utility.hpp"

#include "lpc43xx_cpp.hpp"

#include "ch.h"

#include <cstdio>

using namespace portapack;

namespace ui::external_app::meteor_lrpt_rx {

namespace {

NavigationView* g_record_error_nav{nullptr};
MeteorLrptRxView* g_meteor_active{nullptr};

void on_record_error(std::string message) {
    if (g_record_error_nav)
        g_record_error_nav->display_modal("Error", message);
}

}  // namespace

uint8_t lrpt_flags_from_pm(uint8_t pm) {
    /* Never restore live interleaved (bit1); post-process offline on H4M. */
    pm = static_cast<uint8_t>(pm & ~0x02u);
    if ((pm & kMeteorPmFlagM2x) == 0)
        pm = static_cast<uint8_t>((pm & ~0x08u) | kMeteorPmFlagM2x);
    return pm;
}

void MeteorLrptRxView::focus() {
    creg::m4txevent::disable();
    register_message_handlers();
    ensure_baseband_started();
    creg::m4txevent::enable();
    field_frequency.focus();
}

void MeteorLrptRxView::ensure_baseband_started() {
    if (baseband_started_ || stopping)
        return;

    if (baseband::is_image_running())
        baseband::shutdown();

    const uint32_t m4_base = portapack::memory::map::m4_code.base();
    const uint32_t sp = *reinterpret_cast<const uint32_t*>(m4_base);
    if (sp < 0x10000000u || sp > 0x10020000u)
        chDbgPanic("Bad M4 in PPMA");

    /* Match WeFax/NOAA: M4_RST only — never PERIPH_RST while M0 owns SD/SPI/LCD. */
    baseband::run_prepared_image(m4_base);

    constexpr uint32_t kLrptSampleRate = 3072000;
    receiver_model.set_sampling_rate(kLrptSampleRate);
    receiver_model.set_baseband_bandwidth(filter_bandwidth_for_sampling_rate((int32_t)kLrptSampleRate));

    push_baseband_config();

    receiver_model.set_hidden_offset(0);
    receiver_model.set_modulation(ReceiverModel::Mode::Capture);
    receiver_model.set_squelch_level(0);
    receiver_model.enable();

    /* PMLS: RecordView write_size 16384 selects soft_stream on M4. */
    (void)record_view.set_sampling_rate(kLrptSampleRate);

    baseband_started_ = true;
    txt_status.set("LRPT soft capture");
}

bool MeteorLrptRxView::push_baseband_config() {
    uint8_t flags = 0;
    if (check_m2x.value()) flags |= kMeteorPmFlagM2x;
    if (!(flags & kMeteorPmFlagM2x))
        flags &= static_cast<uint8_t>(~kMeteorPmFlagLegacyCorr);
    if (check_diff.value()) flags |= kMeteorPmFlagDiff;
    if (check_legacy_corr.value()) flags |= kMeteorPmFlagLegacyCorr;
    const uint8_t sym_k = check_sym_80k.value() ? 80 : 72;
    baseband::set_meteor_lrpt_rx_config(flags, sym_k);
    return true;
}

void MeteorLrptRxView::frequency_updated_static(const rf::Frequency) {
    if (g_meteor_active && g_meteor_active->baseband_started_)
        g_meteor_active->push_baseband_config();
}

void MeteorLrptRxView::on_edit_shown_static() {
    if (g_meteor_active)
        g_meteor_active->paused = true;
}

void MeteorLrptRxView::on_edit_hidden_static() {
    if (g_meteor_active)
        g_meteor_active->paused = false;
}

void MeteorLrptRxView::checkbox_changed_static(Checkbox&, const bool) {
    if (g_meteor_active && g_meteor_active->baseband_started_)
        g_meteor_active->push_baseband_config();
}

MeteorLrptRxView::MeteorLrptRxView(NavigationView& nav)
    : nav_{nav} {
    g_meteor_active = this;
    meteor_lrpt_load_pm_flags(pm_lrpt_flags_);

    /* Loader already copied PMLR into m4_code; M4 start deferred to focus() (WeFax pattern). */
    add_children({&rssi,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &field_volume,
                  &field_frequency,
                  &txt_status,
                  &check_m2x,
                  &check_diff,
                  &check_legacy_corr,
                  &check_sym_80k,
                  &record_view});

    record_view.set_filename_date_frequency(true);
    g_record_error_nav = &nav;
    record_view.on_error = on_record_error;

    field_frequency.set_value(137'900'000);
    field_frequency.set_step(1000);
    field_frequency.updated = frequency_updated_static;
    field_frequency.on_edit_shown = on_edit_shown_static;
    field_frequency.on_edit_hidden = on_edit_hidden_static;

    apply_lrpt_flags_to_ui();

    check_m2x.on_select = checkbox_changed_static;
    check_diff.on_select = checkbox_changed_static;
    check_legacy_corr.on_select = checkbox_changed_static;
    check_sym_80k.on_select = checkbox_changed_static;

    txt_status.set("Starting Meteor LRPT...");
}

MeteorLrptRxView::~MeteorLrptRxView() {
    stopping = true;
    unregister_message_handlers();
    record_view.stop();
    pm_lrpt_flags_ = encode_lrpt_flags();
    meteor_lrpt_save_pm_flags(pm_lrpt_flags_);
    receiver_model.disable();
    baseband::shutdown();
    if (g_meteor_active == this)
        g_meteor_active = nullptr;
}

uint8_t MeteorLrptRxView::encode_lrpt_flags() const {
    uint8_t v = 0;
    if (check_m2x.value())
        v |= kMeteorPmFlagM2x;
    if (check_diff.value())
        v |= kMeteorPmFlagDiff;
    if (check_legacy_corr.value())
        v |= kMeteorPmFlagLegacyCorr;
    if (check_sym_80k.value())
        v |= kMeteorPmFlagSym80k;
    return v;
}

void MeteorLrptRxView::apply_lrpt_flags_to_ui() {
    pm_lrpt_flags_ = lrpt_flags_from_pm(pm_lrpt_flags_);
    (void)check_m2x.set_value((pm_lrpt_flags_ & kMeteorPmFlagM2x) != 0);
    (void)check_diff.set_value((pm_lrpt_flags_ & kMeteorPmFlagDiff) != 0);
    (void)check_legacy_corr.set_value((pm_lrpt_flags_ & kMeteorPmFlagLegacyCorr) != 0);
    (void)check_sym_80k.set_value((pm_lrpt_flags_ & kMeteorPmFlagSym80k) != 0);
}

void MeteorLrptRxView::register_message_handlers() {
    if (handlers_registered_)
        return;
    MessageHandlerRegistration::register_handler(
        Message::ID::MeteorLrptRxStatusData,
        &MeteorLrptRxView::status_message_thunk,
        this);
    handlers_registered_ = true;
}

void MeteorLrptRxView::unregister_message_handlers() {
    if (!handlers_registered_)
        return;
    MessageHandlerRegistration::unregister_id(Message::ID::MeteorLrptRxStatusData);
    handlers_registered_ = false;
}

void MeteorLrptRxView::status_message_thunk(Message* const p, void* const ctx) {
    auto* const self = static_cast<MeteorLrptRxView*>(ctx);
    if (!self || self->stopping || self->paused)
        return;
    self->on_status(*reinterpret_cast<const MeteorLrptRxStatusDataMessage*>(p));
}

void MeteorLrptRxView::on_status(MeteorLrptRxStatusDataMessage msg) {
    char s[96];
    const int n = snprintf(
        s,
        sizeof(s),
        "soft:%u L%d TED:%d blk:%u",
        (unsigned)msg.soft_sym_count,
        (int)msg.demod_lock,
        (int)msg.sym_timing_err,
        (unsigned)msg.cadu_frames);
    if (n > 0)
        txt_status.set(s);
}

}  // namespace ui::external_app::meteor_lrpt_rx
