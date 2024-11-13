/*
 * Copyright (C) 2024 HTotoo
 *
 * This file is part of PortaPack.
 *
 */

#ifndef __UI_flippertx_H__
#define __UI_flippertx_H__

#include "ui.hpp"
#include "ui_language.hpp"
#include "ui_navigation.hpp"
#include "ui_transmitter.hpp"
#include "ui_freq_field.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "utility.hpp"
#include "string_format.hpp"
#include "file_path.hpp"
#include "metadata_file.hpp"
#include "flipper_subfile.hpp"
#include "ui_fileman.hpp"
#include "baseband_api.hpp"

using namespace ui;

namespace ui::external_app::flippertx {

#define OOK_SAMPLERATE 2280000U
class FlipperPlayThread;
class FlipperTxView : public View {
   public:
    FlipperTxView(NavigationView& nav);
    ~FlipperTxView();

    void focus() override;

    std::string title() const override {
        return "FlipperTx";
    };

   private:
    NavigationView& nav_;
    TxRadioState radio_state_{
        433920000 /* frequency */,
        1750000 /* bandwidth */,
        OOK_SAMPLERATE /* sampling rate */
    };

    TxFrequencyField field_frequency{
        {0 * 8, 0 * 16},
        nav_};
    TransmitterView2 tx_view{
        {11 * 8, 0 * 16},
        /*short_ui*/ true};

    app_settings::SettingsManager settings_{
        "tx_flippertx", app_settings::Mode::TX};

    TextField field_filename{
        {0, 2 * 16, 300, 1 * 8},
        "File: -"};

    Button button_startstop{
        {0, 6 * 16, 96, 24},
        LanguageHelper::currentMessages[LANG_START]};

    Button button_browse{
        {0, 3 * 16, 8 * 7, 24},
        LanguageHelper::currentMessages[LANG_BROWSE]};

    bool is_running{false};

    bool start();
    void stop();

    void set_ready();
    void on_tx_progress(const bool done);
    bool on_file_changed(std::filesystem::path new_file_path);

    std::filesystem::path filename = {};
    FlipperProto proto = FLIPPER_PROTO_UNSUPPORTED;
    FlipperPreset preset = FLIPPER_PRESET_UNK;
    uint16_t te = 0;  // for binraw
    uint32_t binraw_bit_count = 0;
    bool ready_signal = false;

    std::unique_ptr<FlipperPlayThread> replay_thread{};

    const std::filesystem::path subghz_dir = u"subghz";

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
            this->on_tx_progress(message.done);
        }};

    MessageHandlerRegistration message_handler_fifo_signal{
        Message::ID::RequestSignal,
        [this](const Message* const p) {
            const auto message = static_cast<const RequestSignalMessage*>(p);
            if (message->signal == RequestSignalMessage::Signal::FillRequest) {
                this->set_ready();
            }
        }};

    MessageHandlerRegistration message_handler_replay_thread_done{
        Message::ID::ReplayThreadDone,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const ReplayThreadDoneMessage*>(p);
            stop();
        }};
};

struct BasebandReplay {
    BasebandReplay(ReplayConfig* const config) {
        baseband::replay_start(config);
    }

    ~BasebandReplay() {
        baseband::replay_stop();
    }
};

class FlipperPlayThread {
   public:
    FlipperPlayThread(
        std::filesystem::path,
        size_t read_size,
        size_t buffer_count,
        bool* ready_signal,
        std::function<void(uint32_t return_code)> terminate_callback);
    ~FlipperPlayThread();

    FlipperPlayThread(const FlipperPlayThread&) = delete;
    FlipperPlayThread(FlipperPlayThread&&) = delete;
    FlipperPlayThread& operator=(const FlipperPlayThread&) = delete;
    FlipperPlayThread& operator=(FlipperPlayThread&&) = delete;

    const ReplayConfig& state() const {
        return config;
    };

    enum FlipperPlayThread_return {
        READ_ERROR = 0,
        END_OF_FILE,
        TERMINATED
    };

   private:
    ReplayConfig config;
    // std::unique_ptr<stream::Reader> reader;
    std::filesystem::path filename;
    bool* ready_sig;
    std::function<void(uint32_t return_code)> terminate_callback;
    Thread* thread{nullptr};

    static msg_t static_fn(void* arg);

    uint32_t run();
};
};  // namespace ui::external_app::flippertx

#endif /*__UI_flippertx_H__*/
