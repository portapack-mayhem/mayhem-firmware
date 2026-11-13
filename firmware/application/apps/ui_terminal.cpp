/*
 * Copyright (C) 2026
 *
 * This file is part of PortaPack.
 */

#include "ui_terminal.hpp"

#include "analog_audio_app.hpp"
#include "baseband_api.hpp"
#include "file.hpp"
#include "file_reader.hpp"
#include "io_convert.hpp"
#include "metadata_file.hpp"
#include "portapack.hpp"
#include "rtc_time.hpp"
#include "replay_schedule.hpp"
#include "file_path.hpp"
#include "string_format.hpp"
#include "ui_external_items_menu_loader.hpp"
#include "ui_textentry.hpp"

#include <algorithm>
#include <codecvt>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <locale>

using namespace portapack;
namespace fs = std::filesystem;

namespace {

fs::path path_from_utf8(const std::string& value) {
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    return convert.from_bytes(value);
}

std::string normalized_path(const std::string& current, const std::string& supplied) {
    const auto full = !supplied.empty() && supplied[0] == '/'
                          ? supplied
                          : current + "/" + supplied;
    std::string result{"/"};
    std::string part;
    const auto append_part = [&result](const std::string& item) {
        if (item == "..") {
            if (result.size() > 1) {
                const auto separator = result.find_last_of('/');
                result.erase(separator == 0 ? 1 : separator);
            }
        } else if (!item.empty() && item != ".") {
            if (result.size() > 1) result += '/';
            result += item;
        }
    };

    for (const auto c : full) {
        if (c == '/') {
            append_part(part);
            part.clear();
        } else {
            part += c;
        }
    }
    append_part(part);
    return result;
}

bool parse_number(const std::string& text, uint64_t maximum, uint64_t& value) {
    char* end = nullptr;
    const auto parsed = strtoull(text.c_str(), &end, 10);
    if (text.empty() || *end != '\0' || parsed > maximum) return false;
    value = parsed;
    return true;
}

std::string normalize_name(const char* text) {
    std::string result;
    for (; *text != '\0'; ++text) {
        char c = *text;
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) result += c;
    }
    return result;
}

bool is_subsequence(const std::string& needle, const std::string& haystack) {
    size_t i = 0;
    for (const auto c : haystack) {
        if (i < needle.size() && c == needle[i]) ++i;
    }
    return i == needle.size();
}

int match_rank(const std::string& query, const std::string& key) {
    if (key.empty()) return -1;
    if (key == query) return 0;
    if (key.find(query) != std::string::npos) return 1;
    if (is_subsequence(query, key)) return 2;
    return -1;
}

struct AppMatch {
    std::string label;
    ui::ViewFactoryBase* factory;
    std::string call_name;
    int rank;
};

}  // namespace

namespace ui {

TerminalView::TerminalView(NavigationView& nav)
    : nav_{nav} {
    add_children({&text_radio_status, &text_input, &output,
                  &button_command, &button_run, &button_stop, &button_clear,
                  &button_up, &button_down});

    button_command.on_select = [this](Button&) { prompt_command(); };
    button_run.on_select = [this](Button&) { run_input(); };
    button_stop.on_select = [this](Button&) { stop_radio(); };
    button_clear.on_select = [this](Button&) { output.clear_lines(); };
    button_up.on_select = [this](Button&) { history_up(); };
    button_down.on_select = [this](Button&) { history_down(); };

    write_line("Mayhem terminal");
    write_line("Type help for commands.");
    show_input();
}

TerminalView::~TerminalView() {
    if (radio_mode_ != RadioMode::Idle) stop_radio(false);
}

void TerminalView::focus() {
    button_command.focus();
}

bool TerminalView::on_key(const KeyEvent key) {
    if (key == KeyEvent::Up) {
        history_up();
        return true;
    }
    if (key == KeyEvent::Down) {
        history_down();
        return true;
    }
    return false;
}

bool TerminalView::on_encoder(const EncoderEvent delta) {
    output.scroll_older(-static_cast<int>(delta));
    return true;
}

TerminalOutput::TerminalOutput(Rect parent_rect)
    : Widget{parent_rect},
      max_lines_{static_cast<size_t>(parent_rect.height() / UI_POS_DEFAULT_HEIGHT)},
      chars_per_line_{static_cast<size_t>(parent_rect.width() / UI_POS_DEFAULT_WIDTH)} {
}

void TerminalOutput::paint(Painter& painter) {
    const auto rect = screen_rect();
    const auto& s = style();
    painter.fill_rectangle(rect, s.background);

    const size_t total = lines_.size();
    const size_t end = total - std::min(scroll_, total);
    const size_t start = end > max_lines_ ? end - max_lines_ : 0;
    int y = 0;
    for (size_t i = start; i < end; ++i) {
        painter.draw_string({rect.left(), rect.top() + y}, s, lines_[i]);
        y += UI_POS_DEFAULT_HEIGHT;
    }
}

void TerminalOutput::add_line(const std::string& text) {
    size_t offset = 0;
    do {
        lines_.push_back(text.substr(offset, chars_per_line_));
        offset += chars_per_line_;
    } while (offset < text.size());

    if (lines_.size() > max_stored_lines)
        lines_.erase(lines_.begin(), lines_.begin() + (lines_.size() - max_stored_lines));
    scroll_ = 0;
    set_dirty();
}

void TerminalOutput::clear_lines() {
    lines_.clear();
    scroll_ = 0;
    set_dirty();
}

void TerminalOutput::scroll_older(int lines) {
    const size_t max_scroll = lines_.size() > max_lines_ ? lines_.size() - max_lines_ : 0;
    int target = static_cast<int>(scroll_) + lines;
    if (target < 0) target = 0;
    if (static_cast<size_t>(target) > max_scroll) target = static_cast<int>(max_scroll);
    if (static_cast<size_t>(target) != scroll_) {
        scroll_ = static_cast<size_t>(target);
        set_dirty();
    }
}

bool TerminalView::is_directory_path(const std::string& path) {
    return path == "/" || fs::is_directory(path_from_utf8(path));
}

void TerminalView::show_input() {
    constexpr size_t max_chars = 30;
    const std::string prompt = current_path_ + "> ";
    const std::string full = prompt + command_buffer_;
    if (full.size() <= max_chars) {
        text_input.set(full);
    } else {
        if (prompt.size() >= max_chars - 3) {
            std::string short_path = "..." + current_path_.substr(current_path_.size() - std::min(current_path_.size(), max_chars - 6));
            text_input.set(short_path + "> " + command_buffer_);
        } else {
            size_t available_for_buffer = max_chars - prompt.size() - 3;
            if (command_buffer_.size() > available_for_buffer && available_for_buffer > 0) {
                text_input.set(prompt + "..." + command_buffer_.substr(command_buffer_.size() - available_for_buffer));
            } else {
                text_input.set(prompt + command_buffer_);
            }
        }
    }
}

void TerminalView::prompt_command() {
    has_pending_ = false;
    text_prompt(nav_, command_buffer_, 96, ENTER_KEYBOARD_MODE_ALPHA,
                [this](std::string& command) {
                    pending_line_ = command;
                    has_pending_ = !pending_line_.empty();
                });
    nav_.set_on_pop([this]() {
        if (!has_pending_) {
            show_input();
            return;
        }
        has_pending_ = false;
        const std::string line = pending_line_;
        execute_line(line);
    });
}

void TerminalView::run_input() {
    const std::string line = command_buffer_;
    if (line.empty()) {
        write_line("input empty: use Cmd or Up");
        return;
    }
    execute_line(line);
}

void TerminalView::execute_line(const std::string& typed_line) {
    std::string line = typed_line;
    if (line == "!!") {
        if (command_history_.empty()) {
            write_line("history is empty");
            return;
        }
        line = command_history_.back();
    }
    if (!line.empty() && (command_history_.empty() || command_history_.back() != line)) {
        command_history_.push_back(line);
        constexpr size_t max_history = 20;
        if (command_history_.size() > max_history)
            command_history_.erase(command_history_.begin());
    }
    history_index_ = command_history_.size();
    command_buffer_.clear();
    show_input();
    run_command(line);
}

void TerminalView::history_up() {
    if (command_history_.empty()) return;
    if (history_index_ > 0) history_index_--;
    command_buffer_ = command_history_[history_index_];
    show_input();
}

void TerminalView::history_down() {
    if (command_history_.empty()) return;
    if (history_index_ + 1 < command_history_.size()) {
        history_index_++;
        command_buffer_ = command_history_[history_index_];
    } else {
        history_index_ = command_history_.size();
        command_buffer_.clear();
    }
    show_input();
}

void TerminalView::write_line(const std::string& text) {
    output.add_line(text);
}

void TerminalView::print_prompt(const std::string& line) {
    write_line(current_path_ + "> " + line);
}

void TerminalView::print_status() {
    switch (radio_mode_) {
        case RadioMode::Idle:
            text_radio_status.set("");
            break;
        case RadioMode::Receive:
            text_radio_status.set("RADIO: receiving");
            break;
        case RadioMode::Transmit:
            text_radio_status.set("RADIO: transmitting");
            break;
    }
}

void TerminalView::stop_radio(bool report) {
    replay_thread_.reset();
    transmitter_model.disable();
    receiver_model.disable();
    baseband::shutdown();
    radio_mode_ = RadioMode::Idle;
    print_status();
    if (report) write_line("radio stopped");
}

void TerminalView::start_receive(const std::string& frequency, const std::string& mode_text) {
    uint64_t value_frequency = 0;
    if (!parse_number(frequency, 7250000000ULL, value_frequency) || value_frequency == 0) {
        write_line("usage: receivesignal <hz> [am|nfm|wfm]");
        return;
    }

    auto mode = ReceiverModel::Mode::AMAudio;
    if (mode_text.empty() || mode_text == "am")
        mode = ReceiverModel::Mode::AMAudio;
    else if (mode_text == "nfm")
        mode = ReceiverModel::Mode::NarrowbandFMAudio;
    else if (mode_text == "wfm")
        mode = ReceiverModel::Mode::WidebandFMAudio;
    else {
        write_line("mode must be am, nfm or wfm");
        return;
    }

    if (radio_mode_ != RadioMode::Idle) stop_radio(false);

    auto settings = receiver_model.settings();
    settings.frequency_app_override = static_cast<rf::Frequency>(value_frequency);
    settings.mode = mode;

    write_line("Audio RX opened. Back returns here.");
    nav_.push<AnalogAudioView>(settings);
}

void TerminalView::start_transmit(const std::string& path_text, const std::string& frequency,
                                  const std::string& gain, const std::string& amp) {
    uint64_t value_frequency = 0;
    uint64_t value_gain = 0;
    uint64_t value_amp = 0;
    if (!parse_number(frequency, 7250000000ULL, value_frequency) || value_frequency == 0 ||
        !parse_number(gain, 47, value_gain) || !parse_number(amp, 1, value_amp)) {
        write_line("usage: sendsignal <file> <hz> <gain> <amp>");
        return;
    }

    const auto path = path_from_utf8(normalized_path(current_path_, path_text));
    if (!fs::file_exists(path) || !fs::is_cxx_capture_file(path)) {
        write_line("capture file not found or unsupported");
        return;
    }

    auto reader = std::make_unique<FileConvertReader>();
    if (reader->open(path)) {
        write_line("cannot open capture file");
        return;
    }

    const auto metadata = read_metadata_file(get_metadata_path(path));
    const auto sample_rate = metadata ? metadata->sample_rate : 500000;
    const auto schedule = replay::select(sample_rate);
    if (!schedule.valid()) {
        write_line("unsupported capture sample rate");
        return;
    }

    stop_radio(false);
    baseband::run_image(portapack::spi_flash::image_tag_replay);
    baseband::set_sample_rate(sample_rate, static_cast<OversampleRate>(schedule.total_factor));
    transmitter_model.initialize();
    transmitter_model.set_target_frequency(static_cast<rf::Frequency>(value_frequency));
    transmitter_model.set_sampling_rate(schedule.output_rate);
    transmitter_model.set_baseband_bandwidth(sample_rate <= 500000 ? 1750000 : 2500000);
    transmitter_model.set_tx_gain(static_cast<uint8_t>(value_gain));
    transmitter_model.set_rf_amp(value_amp != 0);
    transmitter_model.enable();

    const bool input_is_c8 = schedule.total_factor == 1 && reader->convert_c8_to_c16;
    if (input_is_c8) reader->convert_c8_to_c16 = false;
    replay_ready_ = false;
    replay_thread_ = std::make_unique<ReplayThread>(
        std::move(reader), 0x4000, 3, &replay_ready_,
        [](uint32_t return_code) {
            ReplayThreadDoneMessage message{return_code};
            EventDispatcher::send_message(message);
        },
        true, input_is_c8);
    radio_mode_ = RadioMode::Transmit;
    print_status();
    write_line("transmitting; use stop to cancel");
}

void TerminalView::list_apps(const std::string& filter) {
    const auto query = normalize_name(filter.c_str());
    std::string line;
    const auto add = [&](const std::string& name) {
        if (!query.empty() && match_rank(query, normalize_name(name.c_str())) < 0) return;
        if (!line.empty() && line.size() + 1 + name.size() > 30) {
            write_line(line);
            line.clear();
        }
        line += (line.empty() ? "" : " ") + name;
    };

    for (const auto& app : NavigationView::appList) {
        if (app.id != nullptr) add(app.id);
    }
    ExternalItemsMenuLoader::load_all_external_items_callback(
        [&](AppInfoConsole& info) { add(info.appCallName); }, false);
    if (!line.empty()) write_line(line);
    write_line("start one with ./name");
}

void TerminalView::launch_app(const std::string& name) {
    const auto query = normalize_name(name.c_str());
    if (query.empty()) {
        write_line("usage: ./<app name>");
        return;
    }

    std::vector<AppMatch> matches;
    const auto consider = [&](const std::string& label, const char* key_a, const char* key_b,
                              ViewFactoryBase* factory, const std::string& call_name) {
        int rank = match_rank(query, normalize_name(key_a));
        const int rank_b = key_b != nullptr ? match_rank(query, normalize_name(key_b)) : -1;
        if (rank < 0 || (rank_b >= 0 && rank_b < rank)) rank = rank_b;
        if (rank >= 0) matches.push_back({label, factory, call_name, rank});
    };

    for (const auto& app : NavigationView::appList) {
        if (app.id == nullptr || app.viewFactory == nullptr) continue;
        if (strcmp(app.id, "terminal") == 0) continue;
        consider(app.id, app.id, app.displayName, app.viewFactory, {});
    }
    ExternalItemsMenuLoader::load_all_external_items_callback(
        [&](AppInfoConsole& info) {
            consider(info.appCallName, info.appCallName, info.appFriendlyName, nullptr, info.appCallName);
        },
        false);

    if (matches.empty()) {
        write_line("no app matches. Try: apps");
        return;
    }

    int best = matches[0].rank;
    for (const auto& match : matches) best = std::min(best, match.rank);
    std::vector<const AppMatch*> best_matches;
    for (const auto& match : matches) {
        if (match.rank == best) best_matches.push_back(&match);
    }

    if (best_matches.size() > 1 && best != 0) {
        write_line("several apps match, be specific:");
        for (const auto* match : best_matches) write_line(" " + match->label);
        return;
    }

    const auto& chosen = *best_matches[0];
    if (radio_mode_ != RadioMode::Idle) stop_radio(false);
    write_line("starting " + chosen.label);

    if (chosen.factory != nullptr) {
        nav_.push_view(chosen.factory->produce(nav_));
        return;
    }

    std::string app_path = "/" + apps_dir.string() + "/" + chosen.call_name + ".ppma";
    if (ExternalItemsMenuLoader::run_external_app(nav_, path_from_utf8(app_path))) return;
    app_path = "/" + apps_dir.string() + "/" + chosen.call_name + ".ppmp";
    if (ExternalItemsMenuLoader::run_standalone_app(nav_, path_from_utf8(app_path))) return;
    write_line("failed to start " + chosen.call_name);
}

void TerminalView::on_replay_done(uint32_t return_code) {
    if (radio_mode_ != RadioMode::Transmit) return;
    replay_thread_.reset();
    transmitter_model.disable();
    baseband::shutdown();
    radio_mode_ = RadioMode::Idle;
    print_status();
    write_line(return_code == ReplayThread::END_OF_FILE ? "transmit complete" : "transmit stopped/error");
}

void TerminalView::print_help() {
    write_line("FILES");
    write_line(" pwd cd ls mkdir rm touch");
    write_line(" cp mv cat head echo df");
    write_line("SYSTEM");
    write_line(" history !! source date");
    write_line(" uptime version free clear");
    write_line("RADIO");
    write_line(" sendsignal receivesignal");
    write_line(" stop status playwav");
    write_line("APPS");
    write_line(" ./name  apps");
    write_line("Up/Dn history, dial scroll");
}

void TerminalView::show_text_file(const std::string& path_text, size_t max_lines) {
    File file;
    if (file.open(path_from_utf8(normalized_path(current_path_, path_text)))) {
        write_line("cannot open file");
        return;
    }
    char buffer[512];
    const auto result = file.read(buffer, sizeof(buffer));
    if (result.is_error()) {
        write_line("read error");
        return;
    }
    const size_t count = static_cast<size_t>(result.value());
    std::string line;
    size_t lines = 0;
    for (size_t i = 0; i < count && lines < max_lines; ++i) {
        const char c = buffer[i];
        if (c == '\n') {
            write_line(line);
            line.clear();
            ++lines;
        } else if (c >= 32 && c < 127) {
            line += c;
        } else if (c != '\r') {
            line += '.';
        }
    }
    if (!line.empty() && lines < max_lines) write_line(line);
    if (count == sizeof(buffer) || file.size() > count) write_line("...(first 512 bytes shown)");
}

void TerminalView::run_script(const std::string& path_text) {
    if (in_script_) {
        write_line("scripts cannot nest");
        return;
    }
    const auto result = File::read_file(path_from_utf8(normalized_path(current_path_, path_text)));
    if (result.is_error()) {
        write_line("cannot read script");
        return;
    }
    const std::string script = result.value();
    in_script_ = true;
    size_t start = 0;
    while (start <= script.size()) {
        auto end = script.find('\n', start);
        if (end == std::string::npos) end = script.size();
        std::string line = script.substr(start, end - start);
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        if (!line.empty() && line[0] != '#') run_command(line);
        start = end + 1;
    }
    in_script_ = false;
}

bool TerminalView::run_file_command(const std::string_view command, const std::vector<std::string_view>& arguments) {
    const auto arg_path = [&](size_t index) { return std::string{arguments[index]}; };
    const auto full_path = [&](size_t index) { return path_from_utf8(normalized_path(current_path_, arg_path(index))); };
    const auto report = [&](const std::filesystem::filesystem_error& error) {
        write_line(error.ok() ? "ok" : error.what());
    };

    if (command == "touch" && arguments.size() == 2) {
        if (fs::file_exists(full_path(1))) {
            write_line("already exists");
        } else {
            report(make_new_file(full_path(1)));
        }
    } else if (command == "cp" && arguments.size() == 3) {
        report(copy_file(full_path(1), full_path(2)));
    } else if (command == "mv" && arguments.size() == 3) {
        report(rename_file(full_path(1), full_path(2)));
    } else if (command == "cat" && arguments.size() == 2) {
        show_text_file(arg_path(1), 40);
    } else if (command == "head" && arguments.size() >= 2 && arguments.size() <= 3) {
        uint64_t lines = 10;
        if (arguments.size() == 3 && !parse_number(arg_path(2), 40, lines)) {
            write_line("usage: head <file> [n]");
            return true;
        }
        show_text_file(arg_path(1), static_cast<size_t>(lines));
    } else if (command == "df" && arguments.size() == 1) {
        const auto info = fs::space(fs::path{u"/"});
        write_line(to_string_dec_uint(info.free >> 20) + " MB free of " +
                   to_string_dec_uint(info.capacity >> 20) + " MB");
    } else if (command == "echo") {
        size_t redirect = 0;
        for (size_t i = 1; i < arguments.size(); ++i) {
            if (arguments[i] == ">" || arguments[i] == ">>") redirect = i;
        }
        const size_t text_end = redirect ? redirect : arguments.size();
        std::string text;
        for (size_t i = 1; i < text_end; ++i) text += (i > 1 ? " " : "") + std::string{arguments[i]};
        if (redirect == 0) {
            write_line(text);
        } else if (redirect + 2 != arguments.size()) {
            write_line("usage: echo text > file");
        } else {
            File file;
            const auto target = full_path(redirect + 1);
            const auto error = arguments[redirect] == ">" ? file.create(target) : file.append(target);
            if (error) {
                write_line("cannot write file");
            } else {
                const auto written = file.write_line(text);
                write_line(written ? "write error" : "ok");
            }
        }
    } else if (command == "history" && arguments.size() == 1) {
        for (size_t i = 0; i < command_history_.size(); ++i)
            write_line(to_string_dec_uint(i + 1) + " " + command_history_[i]);
    } else if (command == "source" && arguments.size() == 2) {
        run_script(arg_path(1));
    } else if (command == "date" && arguments.size() == 1) {
        write_line(to_string_datetime(rtc_time::now(), YMDHMS));
    } else if (command == "uptime" && arguments.size() == 1) {
        const uint32_t seconds = chTimeNow() / CH_FREQUENCY;
        write_line(to_string_dec_uint(seconds / 3600) + "h " + to_string_dec_uint((seconds / 60) % 60) +
                   "m " + to_string_dec_uint(seconds % 60) + "s");
    } else if (command == "version" && arguments.size() == 1) {
#ifdef VERSION_STRING
        write_line(std::string{"Mayhem "} + VERSION_STRING);
#else
        write_line("Mayhem (unknown version)");
#endif
    } else if (command == "free" && arguments.size() == 1) {
        write_line("free memory: " + to_string_dec_uint(chCoreStatus()) + " bytes");
    } else {
        return false;
    }
    return true;
}

void TerminalView::run_command(const std::string& line) {
    const auto arguments = split_string(line, ' ');
    if (arguments.empty() || arguments[0].empty()) return;
    print_prompt(line);

    const auto command = arguments[0];
    if (in_script_ && ((command.size() > 1 && command[0] == '.') || command == "receivesignal" ||
                       command == "playwav")) {
        write_line("not allowed in scripts");
        return;
    }
    if (run_file_command(command, arguments)) {
        // handled
    } else if (command.size() > 2 && command[0] == '.' && command[1] == '/') {
        launch_app(std::string{command.substr(2)});
    } else if (command == "apps") {
        list_apps(arguments.size() > 1 ? std::string{arguments[1]} : "");
    } else if (command == "help") {
        print_help();
    } else if (command == "pwd") {
        write_line(current_path_);
    } else if (command == "cd") {
        const auto next = arguments.size() > 1 ? normalized_path(current_path_, std::string{arguments[1]}) : "/";
        if (is_directory_path(next)) {
            current_path_ = next;
            show_input(); // Updates the cursor path ASAP
        } else {
            write_line("not a directory");
        }
    } else if (command == "ls") {
        const bool long_format = arguments.size() > 1 && arguments[1] == "-l";
        const size_t path_index = long_format ? 2 : 1;
        const auto path_text = path_index < arguments.size()
                                   ? normalized_path(current_path_, std::string{arguments[path_index]})
                                   : current_path_;
        const auto path = path_from_utf8(path_text);
        if (!is_directory_path(path_text)) {
            write_line("not a directory");
            return;
        }
        for (const auto& entry : fs::directory_iterator(path, "*")) {
            const bool directory = fs::is_directory(entry.status());
            std::string text = entry.path().string() + (directory ? "/" : "");
            if (long_format) text = (directory ? "d " : "- ") + to_string_dec_uint(entry.size()) + " " + text;
            write_line(text);
        }
    } else if (command == "mkdir" && arguments.size() == 2) {
        const auto error = make_new_directory(path_from_utf8(normalized_path(current_path_, std::string{arguments[1]})));
        write_line(error.ok() ? "ok" : error.what());
    } else if ((command == "rm" || command == "unlink") && arguments.size() == 2) {
        const auto error = delete_file(path_from_utf8(normalized_path(current_path_, std::string{arguments[1]})));
        write_line(error.ok() ? "ok" : error.what());
    } else if (command == "clear") {
        output.clear_lines();
    } else if (command == "status") {
        print_status();
        write_line(text_radio_status.get());
    } else if (command == "stop") {
        stop_radio();
    } else if ((command == "sendsignal" || command == "sendsignl") && arguments.size() == 5) {
        start_transmit(std::string{arguments[1]}, std::string{arguments[2]},
                       std::string{arguments[3]}, std::string{arguments[4]});
    } else if (command == "playwav") {
        write_line("opening WAV Viewer: pick the file, press play");
        launch_app("wavview");
    } else if ((command == "receivesignal" || command == "receivesignl") && arguments.size() >= 2 && arguments.size() <= 3) {
        start_receive(std::string{arguments[1]},
                      arguments.size() > 2 ? std::string{arguments[2]} : "");
    } else {
        write_line("unknown command or invalid arguments");
    }
}

}  // namespace ui
