// RocketGod's Shopping Cart Lock app
// https://betaskynet.com
#pragma once

#include "ui_widget.hpp"
#include "ui_transmitter.hpp"
#include "replay_thread.hpp"
#include "baseband_api.hpp"
#include "io_wave.hpp"
#include "audio.hpp"
#include "portapack_shared_memory.hpp"

namespace ui::external_app::shoppingcart_lock {

class ShoppingCartLock : public View {
public:
    explicit ShoppingCartLock(NavigationView& nav);
    ~ShoppingCartLock();
    
    ShoppingCartLock(const ShoppingCartLock&) = delete;
    ShoppingCartLock& operator=(const ShoppingCartLock&) = delete;
    
    std::string title() const override { return "Cart Lock"; };

    void focus() override;

private:
    NavigationView& nav_;
    const std::string wav_dir{"/WAV"};
    std::unique_ptr<ReplayThread> replay_thread{};
    bool ready_signal{false};
    bool thread_sync_complete{false};
    bool looping{false};
    std::string current_file{};

    void log_event(const std::string& message);
    std::string list_wav_files();
    void handle_error(const std::string& message);    
    void play_audio(const std::string& filename, bool loop = false);
    void stop();
    bool is_active() const;
    void wait_for_thread();
    void restart_playback();
    
    MenuView menu_view{
        {0, 0, 240, 150},
        true
    };

    Text text_empty{
        {40, 70, 160, 16},
        "RocketGod was here"
    };

    Button button_lock {
        {40, 165, 160, 35},
        "LOCK"
    };
    
    Button button_unlock {
        {40, 205, 160, 35},
        "UNLOCK"
    };
    
    Button button_stop {
        {40, 245, 160, 35},
        "STOP"
    };

    MessageHandlerRegistration message_handler_fifo_signal{
        Message::ID::RequestSignal,
        [this](const Message* const p) {
            const auto message = static_cast<const RequestSignalMessage*>(p);
            if (message->signal == RequestSignalMessage::Signal::FillRequest) {
                ready_signal = true;
            }
        }
    };

    MessageHandlerRegistration message_handler_replay_thread_done{
        Message::ID::ReplayThreadDone,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const ReplayThreadDoneMessage*>(p);
            if (message.return_code == ReplayThread::END_OF_FILE && looping) {
                if (is_active()) {
                    chThdSleepMilliseconds(50);
                    restart_playback();
                }
            } else {
                thread_sync_complete = true;
                stop();
            }
        }
    };
};

}