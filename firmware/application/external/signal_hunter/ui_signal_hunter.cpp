#include "ui_signal_hunter.hpp"
#include "receiver_model.hpp"
#include "baseband_api.hpp"
#include "ui_fileman.hpp"
#include "freqman_db.hpp"
#include "string_format.hpp"
#include "io_file.hpp"                 // For SD card file writing
#include "rtc_time.hpp"                // For timestamped capture filenames

using namespace portapack;

namespace ui::external_app::signal_hunter {

// --- TAB 1: Main View ---
HunterMainView::HunterMainView(Rect parent_rect, SignalHunterAppView& parent)
    : View(parent_rect), parent_app(parent) {
    add_children({&big_display, &text_status, &button_start_stop, &text_hits});
    big_display.set(433920000);

    button_start_stop.on_select = [this](Button&) {
        parent_app.is_hunting = !parent_app.is_hunting;
        if (parent_app.is_hunting) {
            button_start_stop.set_text("STOP");
            update_status("HUNTING...", Theme::getInstance()->fg_green);
            parent_app.send_hunter_config(true); // Send START to M4
        } else {
            button_start_stop.set_text("START");
            update_status("IDLE", Theme::getInstance()->fg_light);
            parent_app.send_hunter_config(false); // Send STOP to M4
        }
    };
}

void HunterMainView::focus() {
    big_display.set(current_freq_);
    button_start_stop.focus();
}

void HunterMainView::update_frequency(rf::Frequency freq) {
    current_freq_ = freq;
    big_display.set(current_freq_);
}

void HunterMainView::update_status(const std::string& status, const Style* style) {
    text_status.set(status);
    text_status.set_style(style);
}

void HunterMainView::update_hits(uint32_t hits) {
    text_hits.set("Hits: " + to_string_dec_uint(hits));
}

// --- TAB 2: Freqs View ---
HunterFreqsView::HunterFreqsView(Rect parent_rect, SignalHunterAppView& parent)
    : View(parent_rect), parent_app(parent) {
    add_children({&button_load_file, &button_clear, &text_loaded_info});

    button_load_file.on_select = [this](Button&) {
        auto* view = parent_app.get_nav().push<FileLoadView>(".TXT");
        if (view) {
            view->on_changed = [this](std::filesystem::path path) {
                parent_app.frequency_list.clear();
                parent_app.current_freq_index = 0;

                FreqmanDB db{};
                db.open(path);
                for (const auto& entry : db) {
                    if (entry.frequency_a > 0)
                        parent_app.frequency_list.push_back(entry.frequency_a);
                }
                parent_app.freqman_file = path.stem().string();
                update_list_count();
            };
        }
    };

    button_clear.on_select = [this](Button&) {
        parent_app.frequency_list.clear();
        parent_app.current_freq_index = 0;
        update_list_count();
    };
}

void HunterFreqsView::focus() {
    button_load_file.focus();
}

void HunterFreqsView::update_list_count() {
    text_loaded_info.set("Loaded: " + to_string_dec_uint(parent_app.frequency_list.size()) + " freqs");
}

// --- TAB 3: Config View ---
HunterConfigView::HunterConfigView(Rect parent_rect, SignalHunterAppView& parent)
    : View(parent_rect), parent_app(parent) {
    add_children({&labels, &field_threshold, &field_hang_time, &text_info_config});

    field_threshold.set_value(parent_app.energy_threshold);
    field_threshold.on_change = [this](int32_t v) { parent_app.energy_threshold = v; };

    field_hang_time.set_value(parent_app.hangtime_ms);
    field_hang_time.on_change = [this](int32_t v) { parent_app.hangtime_ms = v; };
}

void HunterConfigView::focus() {
    field_threshold.focus();
}

// --- Main App View ---
SignalHunterAppView::SignalHunterAppView(ui::NavigationView& nav)
    : frequency_list{},
      nav_(nav) {

    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    receiver_model.set_target_frequency(433920000);
    receiver_model.set_baseband_bandwidth(1750000);
    receiver_model.set_sampling_rate(2000000);
    receiver_model.enable();

    view_main   = std::make_unique<HunterMainView>(content_rect, *this);
    view_freqs  = std::make_unique<HunterFreqsView>(content_rect, *this);
    view_config = std::make_unique<HunterConfigView>(content_rect, *this);

    tab_view = std::make_unique<TabView>(std::initializer_list<TabView::TabDef>{
        {"Target", Color::cyan(),   view_main.get()},
        {"Freqs",  Color::green(),  view_freqs.get()},
        {"Config", Color::yellow(), view_config.get()}
    });
    tab_view->set_parent_rect(view_rect);

    add_children({
        &field_lna,
        &field_vga,
        &field_rf_amp,
        &rssi,
        tab_view.get(),
        view_main.get(),
        view_freqs.get(),
        view_config.get()
    });

    view_freqs->hidden(true);
    view_config->hidden(true);

    using namespace app_settings;
    settings_ = std::make_unique<SettingsManager>(
        "ext_signal_hunter", Mode::RX,
        SettingBindings{
            {"threshold"sv, &energy_threshold},
            {"hangtime_ms"sv, &hangtime_ms},
            {"file"sv,      &freqman_file}
        }
    );
}

SignalHunterAppView::~SignalHunterAppView() {
    // Safely shutdown recording
    stop_recording();
    receiver_model.disable();
    baseband::shutdown();
}

void SignalHunterAppView::focus() {
    if (tab_view)
        tab_view->focus();
}

// Recording control logic via M0 <-> M4 IPC
void SignalHunterAppView::send_hunter_config(bool start) {
    // Send config to baseband processor via IPC mechanism
    baseband::set_hunter_config(energy_threshold, hangtime_ms, start);

    if (!start) {
        stop_recording();
    }
}

void SignalHunterAppView::on_hunter_trigger(const HunterTriggerMessage* /*message*/) {
    // M4 has detected signal above threshold - start I/Q capture
    if (!capture_thread) {
        trigger_hits++;
        view_main->update_hits(trigger_hits);
        view_main->update_status("RECORDING!", Theme::getInstance()->fg_red);
        
        start_recording();
    }
}

void SignalHunterAppView::on_hunter_stop(const HunterStopMessage* /*message*/) {
    // Hangtime expired - M4 signals stop condition
    if (capture_thread) {
        stop_recording();
        
        if (is_hunting) {
            view_main->update_status("HUNTING...", Theme::getInstance()->fg_green);
            // TODO: Future enhancement - increment current_freq_index
            // and tune to next frequency in list for multi-frequency scanning.
        }
    }
}

void SignalHunterAppView::start_recording() {
    rtc::RTC datetime{};
    rtc_time::now(datetime);
    
    // Generate timestamped filename for capture session
    current_capture_filename = "HUN_" +
        to_string_dec_uint(datetime.year(),   4, '0') +
        to_string_dec_uint(datetime.month(),  2, '0') +
        to_string_dec_uint(datetime.day(),    2, '0') + "T" +
        to_string_dec_uint(datetime.hour(),   2, '0') +
        to_string_dec_uint(datetime.minute(), 2, '0') +
        to_string_dec_uint(datetime.second(), 2, '0');

    // Use FileWriter for direct raw I/Q dump to avoid M0 CPU bottlenecks.
    // FileConvertWriter is heavier
    auto writer = std::make_unique<FileWriter>();
    auto create_error = writer->create(std::filesystem::path(u"/CAPTURES") / (current_capture_filename + ".C16"));

    if (create_error.is_valid()) {
        if (view_main) view_main->update_status("SD ERROR", Theme::getInstance()->fg_red);
        return;
    }

    capture_thread = std::make_unique<CaptureThread>(
        std::move(writer),
        4096, 
        4,
        []() {}, 
        [this](File::Error) {}
    );
    
    if (view_main) view_main->update_status("RECORDING!", Theme::getInstance()->fg_red);
}

void SignalHunterAppView::stop_recording() {
    // Stop I/Q capture and release SD card for metadata write
    capture_thread.reset();

    // Generate metadata file after capture completes
    if (!current_capture_filename.empty()) {
        File txt_file;
        auto txt_error = txt_file.create(std::filesystem::path(u"/CAPTURES") / (current_capture_filename + ".TXT"));
        
        // is_valid() returns true only if error is present (counter-intuitive)
        if (!txt_error.is_valid()) {
            std::string metadata = 
                "center_frequency=" + to_string_dec_uint(receiver_model.target_frequency()) + "\n" +
                // Divide main sample rate by decimation factor (8) to match C16 sample rate
                "sample_rate=" + to_string_dec_uint(receiver_model.sampling_rate() / 8) + "\n";
                
            txt_file.write(metadata.c_str(), metadata.length());
        }
        // Clear filename for next capture cycle
        current_capture_filename.clear();
    }
}

}  // namespace ui::external_app::signal_hunter
