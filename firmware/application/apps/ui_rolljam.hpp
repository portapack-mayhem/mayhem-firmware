/*
 * RollJam — Automated rolling-code capture tool for PortaPack Mayhem
 * ===================================================================
 *
 * Jams the target frequency, captures two rolling-code transmissions,
 * then replays the first (unused) code.  The second code is saved for
 * later use via the Replay app.
 *
 * This file is part of PortaPack.
 *
 * This program is free software; see GPLv2 or later.
 */

#ifndef __UI_ROLLJAM_HPP__
#define __UI_ROLLJAM_HPP__

#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "ui_record_view.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "file_path.hpp"
#include "jammer.hpp"
#include "transmitter_model.hpp"
#include "replay_thread.hpp"
#include "metadata_file.hpp"
#include "io_convert.hpp"
#include "rtc_time.hpp"
#include "string_format.hpp"

#include <string>
#include <vector>
#include <optional>

namespace ui {

enum class RollJamState {
    IDLE,
    JAMMING,
    CAPTURING,
    REPLAYING,
    DONE,
};

class RollJamView : public View {
   public:
    RollJamView(NavigationView& nav);
    ~RollJamView();

    RollJamView(const RollJamView&) = delete;
    RollJamView(RollJamView&&) = delete;
    RollJamView& operator=(const RollJamView&) = delete;
    RollJamView& operator=(RollJamView&&) = delete;

    void focus() override;
    std::string title() const override { return "RollJam"; };

   private:
    static constexpr uint32_t jammer_sample_rate = 3'072'000;
    static constexpr uint32_t jammer_bandwidth = 3'500'000;
    static constexpr uint32_t capture_sample_rate = 500'000;
    static constexpr uint32_t capture_bandwidth = 1'750'000;
    static constexpr uint32_t rx_gain_lna = 32;
    static constexpr uint32_t rx_gain_vga = 32;
    static constexpr uint32_t tx_gain = 47;

    NavigationView& nav_;
    RollJamState state_{RollJamState::IDLE};

    std::vector<std::pair<std::filesystem::path, capture_metadata>> captured_codes_;
    size_t max_codes_{2};

    std::filesystem::path rolljam_dir_{captures_dir / u"ROLLJAM"};

    std::unique_ptr<ReplayThread> replay_thread_{};
    bool replay_ready_signal_{false};

    int32_t capture_counter_{0};
    RecordView record_view{
        {UI_POS_X(0), 15 * 16, screen_width, 1 * 16},
        u"RJ_????.*",
        captures_dir / u"ROLLJAM",
        RecordView::FileType::RawS16,
        16384,
        3};

    Labels labels{
        {{UI_POS_X(0), UI_POS_Y(0)}, "Freq:", Theme::getInstance()->fg_light->foreground},
    };

    RxFrequencyField field_frequency{
        {5 * 8, UI_POS_Y(0)},
        nav_};

    FrequencyStepView field_frequency_step{
        {20 * 8, UI_POS_Y(0)}};

    RFAmpField field_rf_amp_tx{
        {23 * 8, UI_POS_Y(0)}};

    Text text_status{
        {UI_POS_X(0), 6 * 16, screen_width, 16},
        "IDLE"};

    Text text_codes{
        {UI_POS_X(0), 8 * 16, screen_width, 5 * 16},
        "No codes captured"};

    NumberField field_max_codes{
        {UI_POS_X(0), 14 * 16},
        1,
        {1, 5},
        1,
        ' '};

    Text text_max_label{
        {3 * 8, 14 * 16, 14 * 8, 16},
        "Max codes:"};

    Button button_start_stop{
        {UI_POS_X(0), 17 * 16, screen_width / 2 - 4, 40},
        "START"};

    Button button_capture{
        {screen_width / 2 + 4, 17 * 16, screen_width / 2 - 4, 40},
        "CAPTURE"};

    Button button_replay{
        {UI_POS_X(0), 21 * 16, screen_width, 40},
        "REPLAY CODE 1"};

    MessageHandlerRegistration message_handler_capture_done{
        Message::ID::CaptureThreadDone,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const CaptureThreadDoneMessage*>(p);
            this->on_capture_done(message.error);
        }};

    MessageHandlerRegistration message_handler_replay_done{
        Message::ID::ReplayThreadDone,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const ReplayThreadDoneMessage*>(p);
            this->on_replay_done(message.return_code);
        }};

    void update_ui();
    void start_jamming();
    void stop_jamming();
    void start_capture();
    void start_replay();
    void on_capture_done(std::optional<File::Error> error);
    void on_replay_done(uint32_t return_code);
};

}  // namespace ui

#endif
