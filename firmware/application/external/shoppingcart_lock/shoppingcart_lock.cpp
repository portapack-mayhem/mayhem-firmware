// RocketGod's Shopping Cart Lock app
// https://betaskynet.com 
#include "shoppingcart_lock.hpp"

using namespace portapack;

namespace ui::external_app::shoppingcart_lock {

void ShoppingCartLock::log_event(const std::string& message) {
    menu_view.add_item({message,
        ui::Theme::getInstance()->fg_green->foreground,
        nullptr,
        [](KeyEvent){}});
    menu_view.set_highlighted(menu_view.item_count() - 1);
}

bool ShoppingCartLock::is_active() const {
    return (bool)replay_thread;
}

void ShoppingCartLock::focus() {
    menu_view.focus();
}

void ShoppingCartLock::stop() {
    log_event(">>> STOP_SEQUENCE_START");
    if (is_active()) {
        log_event("... Resetting Replay Thread");
        replay_thread.reset();
    }
    log_event("... Stopping Audio Output");
    audio::output::stop();

    log_event("... Resetting State Variables");
    transmitter_model.disable();
    ready_signal = false;
    thread_sync_complete = false;
    looping = false;
    current_file = "";
    log_event("<<< STOP_SEQUENCE_COMPLETE");
}

std::string ShoppingCartLock::list_wav_files() {
    log_event(">>> WAV_SCAN_START");
    auto reader = std::make_unique<WAVFileReader>();
    bool found_lock = false;
    bool found_unlock = false;
    
    for (const auto& entry : std::filesystem::directory_iterator(wav_dir, u"*")) {
        if (std::filesystem::is_regular_file(entry.status())) {
            auto filename = entry.path().filename().string();
            std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
            
            if (filename == "lock.wav") {
                found_lock = true;
                log_event("... Found: lock.wav");
            }
            if (filename == "unlock.wav") {
                found_unlock = true;
                log_event("... Found: unlock.wav");
            }
            
            if (found_lock && found_unlock) {
                break;
            }
        }
    }
    
    if (!found_lock || !found_unlock) {
        log_event("!!! Missing Required Files:");
        if (!found_lock) log_event("!!! Missing: lock.wav");
        if (!found_unlock) log_event("!!! Missing: unlock.wav");
        menu_view.hidden(true);
        text_empty.hidden(false);
    } else {
        log_event("... All Required Files Found");
        menu_view.hidden(false);
        text_empty.hidden(true);
    }
    
    log_event("<<< WAV_SCAN_COMPLETE");
    return found_lock && found_unlock ? "Required WAV files found" : "Missing required WAV files";
}

void ShoppingCartLock::wait_for_thread() {
    uint32_t timeout = 100;
    while (!ready_signal && timeout > 0) {
        chThdYield();
        timeout--;
    }
}

void ShoppingCartLock::restart_playback() {
    auto reader = std::make_unique<WAVFileReader>();
    std::string file_path = wav_dir + "/" + current_file;
    
    if (!reader->open(file_path)) return;

    replay_thread = std::make_unique<ReplayThread>(
        std::move(reader),
        4096,
        4,
        &ready_signal,
        [](uint32_t return_code) {
            ReplayThreadDoneMessage message{return_code};
            EventDispatcher::send_message(message);
        }
    );

    log_event(">> SENDING <<");
    audio::output::start();
    transmitter_model.enable();
}

void ShoppingCartLock::play_audio(const std::string& filename, bool loop) {
    auto reader = std::make_unique<WAVFileReader>();
    stop();

    std::string file_path = wav_dir + "/" + filename;
    if (!reader->open(file_path)) {
        nav_.display_modal("Error", "Cannot open " + filename);
        return;
    }

    current_file = filename;
    looping = loop;

    replay_thread = std::make_unique<ReplayThread>(
        std::move(reader),
        4096,
        4,
        &ready_signal,
        [](uint32_t return_code) {
            ReplayThreadDoneMessage message{return_code};
            EventDispatcher::send_message(message);
        }
    );

    wait_for_thread();

    log_event("... Configuring Baseband");
    baseband::set_audiotx_config(
        1536000 / 24,
        2000000,
        2.0f,
        16,
        16,
        0,
        false,
        false,
        false,
        false
    );

    baseband::set_sample_rate(48000);
    log_event("... Starting Audio Output");
    audio::output::start();
    log_event("... Maxing volume");
    audio::headphone::set_volume(audio::headphone::volume_range().max);

    transmitter_model.enable();
}

ShoppingCartLock::ShoppingCartLock(NavigationView& nav)
    : nav_{nav} {    
    add_children({
        &menu_view,
        &text_empty,
        &button_lock,
        &button_unlock,
        &button_stop
    });

    button_lock.on_select = [this](Button&) {
        if (is_active()) stop();
        log_event(">>> LOCK_SEQUENCE_START");
        play_audio("lock.wav", true);
    };

    button_unlock.on_select = [this](Button&) {
        if (is_active()) stop();
        log_event(">>> UNLOCK_SEQUENCE_START");
        play_audio("unlock.wav", true);
    };

    button_stop.on_select = [this](Button&) {
        log_event(">>> STOPPING AUDIO");
        stop();
    };

    list_wav_files();
   
    log_event("[+] APP BY: ROCKETGOD");
    log_event("[+] BETASKYNET.COM");
    log_event("[*] STATUS: READY");

}

ShoppingCartLock::~ShoppingCartLock() {
    stop();
    baseband::shutdown();
}

}