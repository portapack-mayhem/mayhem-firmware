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
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "file.hpp"
#include "message.hpp"
#include "portapack_shared_memory.hpp"

#include <string>

using namespace ui;

namespace ui::external_app::meteor_lrpt_rx {

constexpr uint8_t METEOR_PREVIEW_ROW = 9;

class MeteorLrptRxView : public View {
   public:
    explicit MeteorLrptRxView(NavigationView& nav);
    ~MeteorLrptRxView() override;

    void focus() override;

    std::string title() const override { return "Meteor LRPT"; }

   private:
    void push_baseband_config();
    uint8_t encode_lrpt_flags() const;
    void apply_lrpt_flags_to_ui();
    void apply_g4_settings_to_ui();
    void on_status(MeteorLrptRxStatusDataMessage msg);
    void on_preview(MeteorLrptRxPreviewLineMessage msg);
    void draw_g4_ipc_preview_thumb();
    void draw_g4_bmp_scroll_view();
    void close_strip_bmp();
    bool strip_bmp_begin_first_row(const std::filesystem::path& path, const uint8_t* gray, uint16_t n);
    bool strip_bmp_append_row(const uint8_t* gray, uint16_t n);

    bool stopping = false;
    bool paused = false;

    NavigationView& nav_;
    RxRadioState radio_state_{};
    /* Persisted UI flags: b0 M2-x, b1 interleaved, b2 diff, b3 legacy corr, b4 80k sym, b5 BMP strip, b6 G4 MSU-MR, b7 G4 live ring */
    uint8_t pm_lrpt_flags_{0x08};
    /** Default tail file: same extension as `RecordView` CADU raw (`.C8`, 1020 B logical frames). */
    std::string pm_g4_cadu_path_{"/LRPT/g4_cadu.C8"};
    /** bit0: write `.ppm` sidecar next to BMP (M0 G4 worker). */
    uint8_t pm_g4_debug_{0};
    app_settings::SettingsManager settings_{
        "rx_meteor_lrpt",
        app_settings::Mode::RX,
        {
            {"lrpt_flags"sv, &pm_lrpt_flags_},
            {"g4_cadu_path"sv, &pm_g4_cadu_path_},
            {"g4_debug"sv, &pm_g4_debug_},
        }};

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

    Checkbox check_m2x{
        {UI_POS_X(0), UI_POS_Y(2)},
        5,
        "M2-x",
        false};
    Checkbox check_interleaved{
        {UI_POS_X(8), UI_POS_Y(2)},
        7,
        "Interlv",
        false};
    Checkbox check_diff{
        {UI_POS_X(17), UI_POS_Y(2)},
        4,
        "Diff",
        false};
    Checkbox check_legacy_corr{
        {UI_POS_X(0), UI_POS_Y(3)},
        8,
        "Leg.corr",
        false};
    Checkbox check_sym_80k{
        {UI_POS_X(11), UI_POS_Y(3)},
        7,
        "80k sym",
        false};
    Checkbox check_save_bmp{
        {UI_POS_X_RIGHT(5), UI_POS_Y(3)},
        3,
        "BMP",
        false};
    Checkbox check_g4{
        {UI_POS_X(0), UI_POS_Y(4)},
        6,
        "MSU-MR",
        false};
    Checkbox check_g4_live{
        {UI_POS_X(8), UI_POS_Y(4)},
        4,
        "Live",
        false};
    Checkbox check_g4_ppm{
        {UI_POS_X(14), UI_POS_Y(4)},
        3,
        "PPM",
        false};
    Button button_g4_path{
        {UI_POS_X(18), UI_POS_Y(4), UI_POS_WIDTH_REMAINING(19), UI_POS_DEFAULT_HEIGHT},
        "Path"};
    Checkbox check_g4_bmp_view{
        {UI_POS_X(0), UI_POS_Y(5)},
        4,
        "Img",
        false};
    Button button_bmp_pgup{
        {UI_POS_X(5), UI_POS_Y(5), UI_POS_WIDTH(3), UI_POS_DEFAULT_HEIGHT},
        "^"};
    Button button_bmp_pgdn{
        {UI_POS_X(9), UI_POS_Y(5), UI_POS_WIDTH(3), UI_POS_DEFAULT_HEIGHT},
        "v"};

    RecordView record_view{
        {UI_POS_X(0), UI_POS_Y(6), UI_POS_MAXWIDTH, UI_POS_DEFAULT_HEIGHT},
        u"CADU",
        u"LRPT",
        RecordView::FileType::RawS8,
        4096,
        4};

    /* G2: raw int8 soft pairs per Viterbi block (16384 B); stop CADU REC before starting (single M4 capture). */
    RecordView soft_record_view{
        {UI_POS_X(0), UI_POS_Y(8), UI_POS_MAXWIDTH, UI_POS_DEFAULT_HEIGHT},
        u"SOFT",
        u"LRPT",
        RecordView::FileType::RawS8,
        16384,
        4};

    Text txt_status{
        {UI_POS_X(0), UI_POS_Y(1), UI_POS_MAXWIDTH, UI_POS_DEFAULT_HEIGHT},
        ""};

    ui::Color line_buffer[240]{};
    uint32_t last_g4_preview_seq_{0};
    uint32_t g4_bmp_scroll_{0};
    /** Incremental 24bpp BMP strip (avoids linking full `BMPFile` into the 32 KiB external app). */
    File strip_bmp_{};
    uint32_t strip_bpr_{0};
    uint32_t strip_rows_{0};

    MessageHandlerRegistration message_handler_status_{
        Message::ID::MeteorLrptRxStatusData,
        [this](const Message* const p) {
            if (stopping || paused) return;
            on_status(*reinterpret_cast<const MeteorLrptRxStatusDataMessage*>(p));
        }};

    MessageHandlerRegistration message_handler_preview_{
        Message::ID::MeteorLrptRxPreviewLine,
        [this](const Message* const p) {
            if (stopping || paused) return;
            on_preview(*reinterpret_cast<const MeteorLrptRxPreviewLineMessage*>(p));
        }};
};

}  // namespace ui::external_app::meteor_lrpt_rx

#endif /* __UI_METEOR_LRPT_RX_H__ */
