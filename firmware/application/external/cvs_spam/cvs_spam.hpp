// CVS Spam app by RocketGod (@rocketgod-git) https://betaskynet.com
// Original .cu8 files by @jimilinuxguy https://github.com/jimilinuxguy/customer-assistance-buttons-sdr
// If you can read this, you're a nerd. :P
// Come join us at https://discord.gg/thepiratesreborn

#pragma once

#include "ui_widget.hpp"
#include "ui_transmitter.hpp"
#include "replay_thread.hpp"
#include "baseband_api.hpp"
#include "ui_language.hpp"
#include "file_path.hpp"

using namespace portapack;

namespace ui::external_app::cvs_spam {

constexpr uint32_t SAMPLE_RATE = 250000;
constexpr uint64_t TARGET_FREQUENCY = 433920000;

class CVSSpamView : public View {
   public:
    explicit CVSSpamView(NavigationView& nav);
    ~CVSSpamView();

    void focus() override;
    std::string title() const override { return "CVS Spam"; };

   private:
    static constexpr size_t read_size = 0x4000;
    static constexpr size_t buffer_count = 3;

    lfsr_word_t lfsr_v = 1;
    bool chaos_mode{false};

    NavigationView& nav_;
    std::unique_ptr<ReplayThread> replay_thread{};
    bool ready_signal{false};
    bool thread_sync_complete{false};
    std::filesystem::path current_file;

    bool is_active() const;
    void stop_tx();
    void file_error(const std::filesystem::path& path, const std::string& error_details);
    void refresh_list();
    void start_tx(const uint32_t id);
    void start_random_tx();
    void on_tx_progress(const uint32_t progress);

    uint32_t page = 1;
    uint32_t current_page = 1;
    std::vector<std::filesystem::path> file_list{};

    MenuView menu_view{
        {0, 0, 240, 180},
        true};

    Text text_empty{
        {7 * 8, 12 * 8, 16 * 8, 16},
        "Empty directory!"};

    Button button_prev_page{
        {0, 180, 50, 32},
        "<"};

    Button button_next_page{
        {190, 180, 50, 32},
        ">"};

    Text page_info{
        {95, 188, 50, 16}};

    Button button_send{
        {0, 212, 75, 36},
        "CALL"};

    Button button_chaos{
        {82, 212, 75, 36},
        "CHAOS"};

    Button button_stop{
        {165, 212, 75, 36},
        LanguageHelper::currentMessages[LANG_STOP]};

    ProgressBar progressbar{
        {0, 256, 240, 44}};

    MessageHandlerRegistration message_handler_fifo_signal{
        Message::ID::RequestSignal,
        [this](const Message* const p) {
            const auto message = static_cast<const RequestSignalMessage*>(p);
            if (message->signal == RequestSignalMessage::Signal::FillRequest) {
                ready_signal = true;
            }
        }};

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
            this->on_tx_progress(message.progress);
        }};

    MessageHandlerRegistration message_handler_replay_thread_done{
        Message::ID::ReplayThreadDone,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const ReplayThreadDoneMessage*>(p);
            if (message.return_code == ReplayThread::END_OF_FILE) {
                if (chaos_mode) {
                    replay_thread.reset();
                    transmitter_model.disable();
                    ready_signal = false;
                    lfsr_v = lfsr_iterate(lfsr_v);
                    size_t random_index = lfsr_v % file_list.size();
                    menu_view.set_highlighted(random_index);
                    chThdSleepMilliseconds(100);
                    start_tx(random_index);
                } else {
                    thread_sync_complete = true;
                    stop_tx();
                }
            } else if (message.return_code == ReplayThread::READ_ERROR) {
                file_error(file_list[menu_view.highlighted_index()], "Read error during playback");
                if (chaos_mode) {
                    replay_thread.reset();
                    transmitter_model.disable();
                    ready_signal = false;
                    lfsr_v = lfsr_iterate(lfsr_v);
                    size_t random_index = lfsr_v % file_list.size();
                    menu_view.set_highlighted(random_index);
                    chThdSleepMilliseconds(100);
                    start_tx(random_index);
                }
            }
        }};
};

}  // namespace ui::external_app::cvs_spam