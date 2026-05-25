/*
 * Copyright (C) 2026
 *
 * This file is part of PortaPack / Mayhem.
 */
#ifndef __UI_METEOR_LRPT_RX_H__
#define __UI_METEOR_LRPT_RX_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "ui_record_view.hpp"
#include "meteor_lrpt_m0_storage.hpp"
#include "message.hpp"
#include "portapack_shared_memory.hpp"
#include "portapack.hpp"

#include <cstddef>

using namespace ui;

namespace ui::external_app::meteor_lrpt_rx {

constexpr uint8_t METEOR_PREVIEW_ROW = 9;

/** Live M0 deinterleave is disabled on H4M (offline post-process only). */
constexpr uint8_t kMeteorPmFlagM2x = 1u << 0;
constexpr uint8_t kMeteorPmFlagDiff = 1u << 2;
constexpr uint8_t kMeteorPmFlagLegacyCorr = 1u << 3;
constexpr uint8_t kMeteorPmFlagSym80k = 1u << 4;

class MeteorLrptRxView : public View {
   public:
    static void* operator new(std::size_t size);
    static void* operator new(std::size_t size, void* ptr) noexcept;
    static void operator delete(void* ptr) noexcept;

    explicit MeteorLrptRxView(NavigationView& nav);
    ~MeteorLrptRxView() override;

    void focus() override;

    std::string title() const override { return "MeteorRx"; }

   private:
    void ensure_baseband_started();
    bool push_baseband_config();
    uint8_t encode_lrpt_flags() const;
    void apply_lrpt_flags_to_ui();
    void register_message_handlers();
    void unregister_message_handlers();

    static void status_message_thunk(Message* const p, void* const ctx);
    static void frequency_updated_static(rf::Frequency f);
    static void on_edit_shown_static();
    static void on_edit_hidden_static();
    static void checkbox_changed_static(Checkbox&, bool);

    void on_status(MeteorLrptRxStatusDataMessage msg);

    bool stopping = false;
    bool paused = false;
    bool handlers_registered_{false};
    bool baseband_started_{false};

    NavigationView& nav_;
    /* Persisted UI flags (no live interleaved bit — offline post-process only). */
    uint8_t pm_lrpt_flags_{kMeteorPmFlagM2x};

    RFAmpField field_rf_amp{{UI_POS_X(13), UI_POS_Y(0)}};
    LNAGainField field_lna{{UI_POS_X(15), UI_POS_Y(0)}};
    VGAGainField field_vga{{UI_POS_X(18), UI_POS_Y(0)}};
    RSSI rssi{
        {UI_POS_X(21), UI_POS_Y(0), UI_POS_WIDTH_REMAINING(24), 4}};
    AudioVolumeField field_volume{
        {UI_POS_X_RIGHT(2), UI_POS_Y(0)}};

    RxFrequencyField field_frequency{
        {UI_POS_X(0), UI_POS_Y(0)},
        nav_};

    /* Checkbox widgets are 24px tall; use every other UI row (32px) to avoid overlap. */
    Checkbox check_m2x{
        {UI_POS_X(0), UI_POS_Y(2)},
        10,
        "M2-x",
        false};
    Checkbox check_diff{
        {UI_POS_X(0), UI_POS_Y(4)},
        10,
        "Diff",
        false};
    Checkbox check_legacy_corr{
        {UI_POS_X(0), UI_POS_Y(6)},
        14,
        "Legacy corr",
        false};
    Checkbox check_sym_80k{
        {UI_POS_X(0), UI_POS_Y(8)},
        14,
        "80k symbols",
        false};

    RecordView record_view{
        {UI_POS_X(0), UI_POS_Y(10), UI_POS_MAXWIDTH, UI_POS_DEFAULT_HEIGHT},
        u"SOFT",
        u"LRPT",
        RecordView::FileType::RawS8,
        16384,
        2};

    Text txt_status{
        {UI_POS_X(0), UI_POS_Y(1), UI_POS_MAXWIDTH, UI_POS_DEFAULT_HEIGHT},
        ""};
};

}  // namespace ui::external_app::meteor_lrpt_rx

#endif /* __UI_METEOR_LRPT_RX_H__ */
