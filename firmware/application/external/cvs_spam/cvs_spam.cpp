// CVS Spam app by RocketGod (@rocketgod-git) https://betaskynet.com
// Original .cu8 files by @jimilinuxguy https://github.com/jimilinuxguy/customer-assistance-buttons-sdr
// If you can read this, you're a nerd. :P
// Come join us at https://discord.gg/thepiratesreborn

#include "cvs_spam.hpp"
#include "io_file.hpp"
#include "metadata_file.hpp"
#include "oversample.hpp"
#include "io_convert.hpp"
#include "lfsr_random.hpp"

using namespace portapack;

namespace ui::external_app::cvs_spam {

void CVSSpamView::file_error(const std::filesystem::path& path, const std::string& error_details) {
    nav_.display_modal("Error",
                       "File error:\n" + path.string() + "\n" + error_details);
}

void CVSSpamView::refresh_list() {
    file_list.clear();
    current_page = page;
    uint32_t count = 0;

    for (const auto& entry : std::filesystem::directory_iterator(cvsfiles_dir, u"*")) {
        if (std::filesystem::is_regular_file(entry.status())) {
            auto filename = entry.path().filename().string();
            auto extension = entry.path().extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

            if (extension == ".c16" || extension == ".cu8") {
                if (count >= (page - 1) * 50 && count < page * 50) {
                    file_list.push_back(entry.path());
                    if (file_list.size() == 50) {
                        page++;
                        break;
                    }
                }
                count++;
            }
        }
    }

    if (file_list.empty()) {
        if (page == 1) {
            menu_view.hidden(true);
            text_empty.hidden(false);
        } else {
            page = 1;
            refresh_list();
            return;
        }
    } else {
        menu_view.hidden(false);
        text_empty.hidden(true);
        menu_view.clear();
        for (const auto& file : file_list) {
            menu_view.add_item({file.filename().string(),
                                ui::Theme::getInstance()->fg_green->foreground,
                                nullptr,
                                [this](KeyEvent key) {
                                    if (key == KeyEvent::Select) {
                                        start_tx(menu_view.highlighted_index());
                                    }
                                }});
        }
        page_info.set("Page " + std::to_string(current_page));
        menu_view.set_highlighted(0);
    }

    if (file_list.size() < 50) {
        page = 1;
    }
}

void CVSSpamView::start_tx(const uint32_t id) {
    if (is_active()) {
        stop_tx();
        return;
    }

    current_file = cvsfiles_dir / file_list[id].filename();

    File capture_file;
    auto open_error = capture_file.open(current_file);
    if (open_error) {
        file_error(current_file,
                   "Cannot open file.\n"
                   "Initial file check failed.\n"
                   "Path: " +
                       cvsfiles_dir.string() +
                       "\n"
                       "Error: " +
                       std::to_string(static_cast<uint32_t>(open_error)));
        return;
    }

    capture_file.close();

    replay_thread.reset();
    transmitter_model.disable();
    ready_signal = false;

    baseband::set_sample_rate(SAMPLE_RATE, get_oversample_rate(SAMPLE_RATE));

    auto reader = std::make_unique<FileConvertReader>();
    if (auto error = reader->open(current_file)) {
        file_error(current_file,
                   "Cannot read C16 data.\n"
                   "Check file format/perms.\n"
                   "Rate: " +
                       to_string_dec_uint(SAMPLE_RATE) +
                       "\n"
                       "Error: " +
                       std::to_string(static_cast<uint32_t>(error)));
        return;
    }

    progressbar.set_value(0);

    transmitter_model.set_sampling_rate(get_actual_sample_rate(SAMPLE_RATE));
    transmitter_model.set_baseband_bandwidth(SAMPLE_RATE <= 500000 ? 1750000 : 2500000);
    transmitter_model.set_target_frequency(TARGET_FREQUENCY);
    transmitter_model.enable();

    chThdSleepMilliseconds(100);

    replay_thread = std::make_unique<ReplayThread>(
        std::move(reader),
        read_size,
        buffer_count,
        &ready_signal,
        [](uint32_t return_code) {
            ReplayThreadDoneMessage message{return_code};
            EventDispatcher::send_message(message);
        });
}

void CVSSpamView::start_random_tx() {
    if (is_active()) {
        stop_tx();
        return;
    }

    if (file_list.empty()) {
        nav_.display_modal("Error", "No files found!");
        return;
    }

    lfsr_v = lfsr_iterate(lfsr_v);
    size_t random_index = lfsr_v % file_list.size();

    current_file = cvsfiles_dir / file_list[random_index].filename();

    File capture_file;
    auto open_error = capture_file.open(current_file);
    if (open_error) {
        file_error(current_file, "Cannot open file.\nInitial check failed.");
        return;
    }

    capture_file.close();

    replay_thread.reset();
    transmitter_model.disable();
    ready_signal = false;

    baseband::set_sample_rate(SAMPLE_RATE, get_oversample_rate(SAMPLE_RATE));

    auto reader = std::make_unique<FileConvertReader>();
    if (auto error = reader->open(current_file)) {
        file_error(current_file, "Cannot read file data.");
        return;
    }

    progressbar.set_value(0);

    transmitter_model.set_sampling_rate(get_actual_sample_rate(SAMPLE_RATE));
    transmitter_model.set_baseband_bandwidth(SAMPLE_RATE <= 500000 ? 1750000 : 2500000);
    transmitter_model.set_target_frequency(TARGET_FREQUENCY);
    transmitter_model.enable();

    chThdSleepMilliseconds(100);

    replay_thread = std::make_unique<ReplayThread>(
        std::move(reader),
        read_size,
        buffer_count,
        &ready_signal,
        [](uint32_t return_code) {
            ReplayThreadDoneMessage message{return_code};
            EventDispatcher::send_message(message);
        });
}

void CVSSpamView::on_tx_progress(const uint32_t progress) {
    if (is_active()) {
        progressbar.set_value(progress % 100);
        progressbar.set_style(Theme::getInstance()->fg_red);
    }
}

void CVSSpamView::focus() {
    menu_view.focus();
}

bool CVSSpamView::is_active() const {
    return (bool)replay_thread;
}

void CVSSpamView::stop_tx() {
    replay_thread.reset();
    transmitter_model.disable();
    ready_signal = false;
    thread_sync_complete = false;
    chaos_mode = false;
    progressbar.set_value(0);
    chThdSleepMilliseconds(50);
}

CVSSpamView::CVSSpamView(NavigationView& nav)
    : nav_{nav},
      current_file{} {
    baseband::run_image(portapack::spi_flash::image_tag_replay);

    add_children({&menu_view,
                  &text_empty,
                  &button_prev_page,
                  &button_next_page,
                  &page_info,
                  &button_send,
                  &button_chaos,
                  &button_stop,
                  &progressbar});

    button_send.set_style(Theme::getInstance()->fg_magenta);
    button_chaos.set_style(Theme::getInstance()->fg_yellow);
    button_stop.set_style(Theme::getInstance()->fg_red);

    transmitter_model.set_sampling_rate(1536000);
    transmitter_model.set_baseband_bandwidth(1750000);
    transmitter_model.set_rf_amp(true);
    transmitter_model.set_tx_gain(47);
    transmitter_model.set_channel_bandwidth(1750000);

    refresh_list();

    button_prev_page.on_select = [this](Button&) {
        if (is_active()) return;
        if (current_page == 1) return;
        if (current_page == 2) page = 1;
        page = current_page - 1;
        refresh_list();
    };

    button_next_page.on_select = [this](Button&) {
        if (is_active()) return;
        refresh_list();
    };

    button_send.on_select = [this](Button&) {
        if (!file_list.empty()) {
            start_tx(menu_view.highlighted_index());
        }
    };

    button_chaos.on_select = [this](Button&) {
        if (is_active()) {
            chaos_mode = false;
            stop_tx();
        } else {
            chaos_mode = true;
            start_random_tx();
        }
    };

    button_stop.on_select = [this](Button&) {
        if (is_active()) {
            stop_tx();
        }
    };
}

CVSSpamView::~CVSSpamView() {
    stop_tx();
    baseband::shutdown();
}

}  // namespace ui::external_app::cvs_spam