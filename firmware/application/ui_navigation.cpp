/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "ui_navigation.hpp"

// #include "modules.h"

#include "portapack.hpp"
#include "event_m0.hpp"
#include "bmp_splash.hpp"
#include "bmp_modal_warning.hpp"
#include "portapack_persistent_memory.hpp"
#include "portapack_shared_memory.hpp"

#include "ui_about_simple.hpp"
#include "ui_adsb_rx.hpp"
#include "ui_adsb_tx.hpp"
#include "ui_afsk_rx.hpp"
#include "ui_aprs_rx.hpp"
#include "ui_btle_rx.hpp"
#include "ui_nrf_rx.hpp"
#include "ui_aprs_tx.hpp"
#include "ui_bht_tx.hpp"
#include "ui_coasterp.hpp"
#include "ui_debug.hpp"
#include "ui_encoders.hpp"
#include "ui_fileman.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "ui_freqman.hpp"
#include "ui_jammer.hpp"
// #include "ui_keyfob.hpp"
#include "ui_lcr.hpp"
#include "ui_mictx.hpp"
#include "ui_morse.hpp"
// #include "ui_numbers.hpp"
// #include "ui_nuoptix.hpp"
// #include "ui_playdead.hpp"
#include "ui_pocsag_tx.hpp"
#include "ui_rds.hpp"
#include "ui_remote.hpp"
#include "ui_scanner.hpp"
#include "ui_search.hpp"
#include "ui_recon.hpp"
#include "ui_level.hpp"
#include "ui_sd_wipe.hpp"
#include "ui_settings.hpp"
#include "ui_siggen.hpp"
#include "ui_sonde.hpp"
#include "ui_sstvtx.hpp"
#include "ui_styles.hpp"
// #include "ui_test.hpp"
#include "ui_text_editor.hpp"
#include "ui_tone_search.hpp"
#include "ui_touchtunes.hpp"
#include "ui_playlist.hpp"
#include "ui_view_wav.hpp"
#include "ui_whipcalc.hpp"
#include "ui_flash_utility.hpp"
#include "ui_sd_over_usb.hpp"
#include "ui_spectrum_painter.hpp"
#include "ui_ss_viewer.hpp"

// #include "acars_app.hpp"
#include "ais_app.hpp"
#include "analog_audio_app.hpp"
#include "analog_tv_app.hpp"
#include "capture_app.hpp"
#include "ert_app.hpp"
#include "lge_app.hpp"
#include "pocsag_app.hpp"
#include "replay_app.hpp"
#include "gps_sim_app.hpp"
#include "soundboard_app.hpp"
#include "tpms_app.hpp"

#include "core_control.hpp"
#include "ui_looking_glass_app.hpp"
#include "file.hpp"
#include "png_writer.hpp"

using portapack::receiver_model;
using portapack::transmitter_model;
namespace pmem = portapack::persistent_memory;

namespace ui {

/* StatusTray ************************************************************/

StatusTray::StatusTray(Point pos)
    : View{{pos, {0, height}}},
      pos_(pos) {
    set_focusable(false);
}

void StatusTray::add(Widget* child) {
    width_ += child->parent_rect().width();
    add_child(child);
}

void StatusTray::update_layout() {
    // Widen the tray's parent rect.
    auto rect = parent_rect();
    set_parent_rect({{rect.left() - width_, rect.top()}, {rect.right() + width_, height}});

    // Update the children.
    auto x = 0;
    for (auto child : children()) {
        auto size = child->parent_rect().size();
        child->set_parent_rect({{x, 0}, size});
        x += size.width();
    }
    set_dirty();
}

void StatusTray::clear() {
    // More efficient than 'remove_children'.
    for (auto child : children())
        child->set_parent(nullptr);
    children_.clear();
    width_ = 0;
    set_parent_rect({pos_, {width_, height}});
    set_dirty();
}

void StatusTray::paint(Painter&) {
}

/* SystemStatusView ******************************************************/

SystemStatusView::SystemStatusView(
    NavigationView& nav)
    : nav_(nav) {
    add_children({
        &backdrop,
        &button_back,
        &title,
        &button_title,
        &status_icons,
    });

    if (pmem::should_use_sdcard_for_pmem()) {
        pmem::load_persistent_settings_from_file();
    }

    button_back.id = -1;  // Special ID used by FocusManager
    title.set_style(&Styles::bg_dark_grey);

    button_back.on_select = [this](ImageButton&) {
        if (pmem::should_use_sdcard_for_pmem()) {
            pmem::save_persistent_settings_to_file();
        }
        if (this->on_back)
            this->on_back();
    };

    button_title.on_select = [this](ImageButton&) {
        this->on_title();
    };

    button_converter.on_select = [this](ImageButton&) {
        this->on_converter();
    };

    toggle_speaker.on_change = [this](bool v) {
        pmem::set_config_speaker_disable(v);
        audio::output::update_audio_mute();
        refresh();
    };

    toggle_mute.on_change = [this](bool v) {
        pmem::set_config_audio_mute(v);
        audio::output::update_audio_mute();
        refresh();
    };

    toggle_stealth.on_change = [this](bool v) {
        pmem::set_stealth_mode(v);
        refresh();
    };

    button_bias_tee.on_select = [this](ImageButton&) {
        this->on_bias_tee();
    };

    button_camera.on_select = [this](ImageButton&) {
        this->on_camera();
    };

    button_sleep.on_select = [this](ImageButton&) {
        DisplaySleepMessage message;
        EventDispatcher::send_message(message);
    };

    button_clock_status.on_select = [this](ImageButton&) {
        this->on_clk();
    };

    // Initialize toggle buttons
    toggle_speaker.set_value(pmem::config_speaker_disable());
    toggle_mute.set_value(pmem::config_audio_mute());
    toggle_stealth.set_value(pmem::stealth_mode());

    audio::output::stop();
    audio::output::update_audio_mute();
    refresh();
}

void SystemStatusView::refresh() {
    // NB: Order of insertion is the display order Left->Right.
    // TODO: Might be better to support hide and only add once.
    status_icons.clear();
    if (!pmem::ui_hide_camera()) status_icons.add(&button_camera);
    if (!pmem::ui_hide_sleep()) status_icons.add(&button_sleep);
    if (!pmem::ui_hide_stealth()) status_icons.add(&toggle_stealth);
    if (!pmem::ui_hide_converter()) status_icons.add(&button_converter);
    if (!pmem::ui_hide_bias_tee()) status_icons.add(&button_bias_tee);
    if (!pmem::ui_hide_clock()) status_icons.add(&button_clock_status);
    if (!pmem::ui_hide_mute()) status_icons.add(&toggle_mute);

    // Display "Disable speaker" icon only if AK4951 Codec which has separate speaker/headphone control
    if (audio::speaker_disable_supported() && !pmem::ui_hide_speaker()) status_icons.add(&toggle_speaker);

    if (!pmem::ui_hide_sd_card()) status_icons.add(&sd_card_status_view);
    status_icons.update_layout();

    // Clock status
    bool external_clk = portapack::clock_manager.get_reference().source == ClockManager::ReferenceSource::External;
    button_clock_status.set_bitmap(external_clk ? &bitmap_icon_clk_ext : &bitmap_icon_clk_int);
    button_clock_status.set_foreground(pmem::clkout_enabled() ? Color::green() : Color::light_grey());

    // Antenna DC Bias
    if (portapack::get_antenna_bias()) {
        button_bias_tee.set_bitmap(&bitmap_icon_biast_on);
        button_bias_tee.set_foreground(Color::yellow());
    } else {
        button_bias_tee.set_bitmap(&bitmap_icon_biast_off);
        button_bias_tee.set_foreground(Color::light_grey());
    }

    // Converter
    button_converter.set_bitmap(pmem::config_updown_converter() ? &bitmap_icon_downconvert : &bitmap_icon_upconvert);
    button_converter.set_foreground(pmem::config_converter() ? Color::red() : Color::light_grey());

    set_dirty();
}

void SystemStatusView::set_back_enabled(bool new_value) {
    if (new_value) {
        add_child(&button_back);
    } else {
        remove_child(&button_back);
    }
}

void SystemStatusView::set_title_image_enabled(bool new_value) {
    if (new_value) {
        add_child(&button_title);
    } else {
        remove_child(&button_title);
    }
}

void SystemStatusView::set_title(const std::string new_value) {
    if (new_value.empty()) {
        title.set(default_title);
    } else {
        // Limit length of title string to prevent partial characters if too many StatusView icons
        size_t max_len = (status_icons.parent_rect().left() - title.parent_rect().left()) / 8;
        title.set(truncate(new_value, max_len));
    }
}

void SystemStatusView::on_converter() {
    pmem::set_config_converter(!pmem::config_converter());

    // Poke to update tuning
    // NOTE: Code assumes here that a TX app isn't active, since RX & TX have diff tuning offsets
    // (and there's only one tuner in the radio so can't update tuner for both).
    // TODO: Maybe expose the 'enabled_' flag on models.
    receiver_model.set_target_frequency(receiver_model.target_frequency());
    refresh();
}

void SystemStatusView::on_bias_tee() {
    if (!portapack::get_antenna_bias()) {
        nav_.display_modal("Bias voltage",
                           "Enable DC voltage on\nantenna connector?",
                           YESNO,
                           [this](bool v) {
                               if (v) {
                                   portapack::set_antenna_bias(true);
                                   receiver_model.set_antenna_bias();
                                   transmitter_model.set_antenna_bias();
                                   refresh();
                               }
                           });
    } else {
        portapack::set_antenna_bias(false);
        receiver_model.set_antenna_bias();
        transmitter_model.set_antenna_bias();

        // Ensure this is disabled. The models don't actually
        // update the radio unless they are 'enabled_'.
        radio::set_antenna_bias(false);
        refresh();
    }
}

void SystemStatusView::on_camera() {
    ensure_directory("SCREENSHOTS");
    auto path = next_filename_matching_pattern(u"SCREENSHOTS/SCR_????.PNG");

    if (path.empty())
        return;

    PNGWriter png;
    auto error = png.create(path);
    if (error)
        return;

    for (int i = 0; i < screen_height; i++) {
        std::array<ColorRGB888, screen_width> row;
        portapack::display.read_pixels({0, i, screen_width, 1}, row);
        png.write_scanline(row);
    }
}

void SystemStatusView::on_clk() {
    pmem::set_clkout_enabled(!pmem::clkout_enabled());
    portapack::clock_manager.enable_clock_output(pmem::clkout_enabled());
    refresh();
}

void SystemStatusView::on_title() {
    if (nav_.is_top())
        nav_.push<AboutView>();
    else
        nav_.pop();
}

/* Information View *****************************************************/

InformationView::InformationView(
    NavigationView& nav)
    : nav_(nav) {
    static constexpr Style style_infobar{
        .font = font::fixed_8x16,
        .background = {33, 33, 33},
        .foreground = Color::white(),
    };

    add_children({&backdrop,
                  &version,
                  &ltime});

    version.set_style(&style_infobar);

    ltime.set_hide_clock(pmem::hide_clock());
    ltime.set_style(&style_infobar);
    ltime.set_seconds_enabled(true);
    ltime.set_date_enabled(pmem::clock_with_date());
    set_dirty();
}

void InformationView::refresh() {
    ltime.set_hide_clock(pmem::hide_clock());
    ltime.set_seconds_enabled(true);
    ltime.set_date_enabled(pmem::clock_with_date());
}

/* Navigation ************************************************************/

bool NavigationView::is_top() const {
    return view_stack.size() == 1;
}

View* NavigationView::push_view(std::unique_ptr<View> new_view) {
    free_view();

    const auto p = new_view.get();
    view_stack.emplace_back(ViewState{std::move(new_view), {}});

    update_view();

    return p;
}

void NavigationView::pop() {
    pop(true);
}

void NavigationView::pop_modal() {
    // Pop modal view + underlying app view.
    // TODO: this shouldn't be necessary.
    pop(false);
    pop(true);
}

void NavigationView::display_modal(
    const std::string& title,
    const std::string& message) {
    display_modal(title, message, INFO, nullptr);
}

void NavigationView::display_modal(
    const std::string& title,
    const std::string& message,
    const modal_t type,
    const std::function<void(bool)> on_choice) {
    /* If a modal view is already visible, don't display another */
    if (!modal_view) {
        modal_view = push<ModalMessageView>(title, message, type, on_choice);
    }
}

void NavigationView::pop(bool update) {
    if (view() == modal_view) {
        modal_view = nullptr;
    }

    // Can't pop last item from stack.
    if (view_stack.size() > 1) {
        auto on_pop = view_stack.back().on_pop;

        free_view();
        view_stack.pop_back();

        if (update)
            update_view();

        if (on_pop) on_pop();
    }
}

void NavigationView::free_view() {
    remove_child(view());
}

void NavigationView::update_view() {
    const auto new_view = view_stack.back().view.get();

    add_child(new_view);
    new_view->set_parent_rect({{0, 0}, size()});

    focus();
    set_dirty();

    if (on_view_changed) {
        on_view_changed(*new_view);
    }
}

Widget* NavigationView::view() const {
    return children_.empty() ? nullptr : children_[0];
}

void NavigationView::focus() {
    if (view()) {
        view()->focus();
    }
}

bool NavigationView::set_on_pop(std::function<void()> on_pop) {
    if (view_stack.size() <= 1)
        return false;

    auto& top = view_stack.back();
    if (top.on_pop)
        return false;

    top.on_pop = on_pop;
    return true;
}

/* ReceiversMenuView *****************************************************/

ReceiversMenuView::ReceiversMenuView(NavigationView& nav) {
    if (pmem::show_gui_return_icon()) {
        add_items({{"..", Color::light_grey(), &bitmap_icon_previous, [&nav]() { nav.pop(); }}});
    }
    add_items({
        {"ADS-B", Color::green(), &bitmap_icon_adsb, [&nav]() { nav.push<ADSBRxView>(); }},
        //{ "ACARS",	Color::yellow(),	&bitmap_icon_adsb,			[&nav](){ nav.push<ACARSAppView>(); }},
        {"AIS Boats", Color::green(), &bitmap_icon_ais, [&nav]() { nav.push<AISAppView>(); }},
        {"AFSK", Color::yellow(), &bitmap_icon_modem, [&nav]() { nav.push<AFSKRxView>(); }},
        {"BTLE", Color::yellow(), &bitmap_icon_btle, [&nav]() { nav.push<BTLERxView>(); }},
        {"NRF", Color::yellow(), &bitmap_icon_nrf, [&nav]() { nav.push<NRFRxView>(); }},
        {"Audio", Color::green(), &bitmap_icon_speaker, [&nav]() { nav.push<AnalogAudioView>(); }},
        {"Analog TV", Color::yellow(), &bitmap_icon_sstv, [&nav]() { nav.push<AnalogTvView>(); }},
        {"ERT Meter", Color::green(), &bitmap_icon_ert, [&nav]() { nav.push<ERTAppView>(); }},
        {"POCSAG", Color::green(), &bitmap_icon_pocsag, [&nav]() { nav.push<POCSAGAppView>(); }},
        {"Radiosnde", Color::green(), &bitmap_icon_sonde, [&nav]() { nav.push<SondeView>(); }},
        {"TPMS Cars", Color::green(), &bitmap_icon_tpms, [&nav]() { nav.push<TPMSAppView>(); }},
        {"Recon", Color::green(), &bitmap_icon_scanner, [&nav]() { nav.push<ReconView>(); }},
        {"Level", Color::green(), &bitmap_icon_options_radio, [&nav]() { nav.push<LevelView>(); }},
        {"APRS", Color::green(), &bitmap_icon_aprs, [&nav]() { nav.push<APRSRXView>(); }}
        /*
                { "DMR", 		Color::dark_grey(),	&bitmap_icon_dmr,		[&nav](){ nav.push<NotImplementedView>(); } },
                { "SIGFOX", 	Color::dark_grey(),	&bitmap_icon_fox,		[&nav](){ nav.push<NotImplementedView>(); } }, // SIGFRXView
                { "LoRa", 		Color::dark_grey(),	&bitmap_icon_lora,		[&nav](){ nav.push<NotImplementedView>(); } },
                { "SSTV", 		Color::dark_grey(), &bitmap_icon_sstv,		[&nav](){ nav.push<NotImplementedView>(); } },
                { "TETRA", 		Color::dark_grey(),	&bitmap_icon_tetra,		[&nav](){ nav.push<NotImplementedView>(); } },*/
    });

    // set_highlighted(0);		// Default selection is "Audio"
}

/* TransmittersMenuView **************************************************/

TransmittersMenuView::TransmittersMenuView(NavigationView& nav) {
    if (pmem::show_gui_return_icon()) {
        add_items({{"..", Color::light_grey(), &bitmap_icon_previous, [&nav]() { nav.pop(); }}});
    }
    add_items({
        {"ADS-B [S]", ui::Color::green(), &bitmap_icon_adsb, [&nav]() { nav.push<ADSBTxView>(); }},
        {"APRS", ui::Color::green(), &bitmap_icon_aprs, [&nav]() { nav.push<APRSTXView>(); }},
        {"BHT Xy/EP", ui::Color::green(), &bitmap_icon_bht, [&nav]() { nav.push<BHTView>(); }},
        {"GPS Sim", ui::Color::green(), &bitmap_icon_gps_sim, [&nav]() { nav.push<GpsSimAppView>(); }},
        {"Jammer", ui::Color::green(), &bitmap_icon_jammer, [&nav]() { nav.push<JammerView>(); }},
        //{ "Key fob",		ui::Color::orange(),	&bitmap_icon_keyfob,	[&nav](){ nav.push<KeyfobView>(); } },
        {"LGE tool", ui::Color::yellow(), &bitmap_icon_lge, [&nav]() { nav.push<LGEView>(); }},
        {"Morse", ui::Color::green(), &bitmap_icon_morse, [&nav]() { nav.push<MorseView>(); }},
        {"BurgerPgr", ui::Color::yellow(), &bitmap_icon_burger, [&nav]() { nav.push<CoasterPagerView>(); }},
        //{ "Nuoptix DTMF", 	ui::Color::green(),		&bitmap_icon_nuoptix,	[&nav](){ nav.push<NuoptixView>(); } },
        {"OOK", ui::Color::yellow(), &bitmap_icon_remote, [&nav]() { nav.push<EncodersView>(); }},
        {"POCSAG", ui::Color::green(), &bitmap_icon_pocsag, [&nav]() { nav.push<POCSAGTXView>(); }},
        {"RDS", ui::Color::green(), &bitmap_icon_rds, [&nav]() { nav.push<RDSView>(); }},
        {"Soundbrd", ui::Color::green(), &bitmap_icon_soundboard, [&nav]() { nav.push<SoundBoardView>(); }},
        {"SSTV", ui::Color::green(), &bitmap_icon_sstv, [&nav]() { nav.push<SSTVTXView>(); }},
        {"TEDI/LCR", ui::Color::yellow(), &bitmap_icon_lcr, [&nav]() { nav.push<LCRView>(); }},
        {"TouchTune", ui::Color::green(), &bitmap_icon_touchtunes, [&nav]() { nav.push<TouchTunesView>(); }},
        //{"Playlist", ui::Color::green(), &bitmap_icon_scanner, [&nav]() { nav.push<PlaylistView>(); }},
        {"S.Painter", ui::Color::orange(), &bitmap_icon_paint, [&nav]() { nav.push<SpectrumPainterView>(); }},
        //{ "Remote",			ui::Color::dark_grey(),	&bitmap_icon_remote,	[&nav](){ nav.push<RemoteView>(); } },
    });
}

/* UtilitiesMenuView *****************************************************/

UtilitiesMenuView::UtilitiesMenuView(NavigationView& nav) {
    if (pmem::show_gui_return_icon()) {
        add_items({{"..", Color::light_grey(), &bitmap_icon_previous, [&nav]() { nav.pop(); }}});
    }
    add_items({
        //{ "Test app", 		Color::dark_grey(),	nullptr,				[&nav](){ nav.push<TestView>(); } },
        {"Freq. manager", Color::green(), &bitmap_icon_freqman, [&nav]() { nav.push<FrequencyManagerView>(); }},
        {"File manager", Color::green(), &bitmap_icon_dir, [&nav]() { nav.push<FileManagerView>(); }},
        {"Notepad", Color::dark_cyan(), &bitmap_icon_notepad, [&nav]() { nav.push<TextEditorView>(); }},
        {"Signal gen", Color::green(), &bitmap_icon_cwgen, [&nav]() { nav.push<SigGenView>(); }},
        //{ "Tone search",	Color::dark_grey(), nullptr,					[&nav](){ nav.push<ToneSearchView>(); } },
        {"Wav viewer", Color::yellow(), &bitmap_icon_soundboard, [&nav]() { nav.push<ViewWavView>(); }},
        {"Antenna length", Color::green(), &bitmap_icon_tools_antenna, [&nav]() { nav.push<WhipCalcView>(); }},

        {"Wipe SD card", Color::red(), &bitmap_icon_tools_wipesd, [&nav]() { nav.push<WipeSDView>(); }},
        {"Flash Utility", Color::red(), &bitmap_icon_temperature, [&nav]() { nav.push<FlashUtilityView>(); }},
        {"SD over USB", Color::yellow(), &bitmap_icon_hackrf, [&nav]() { nav.push<SdOverUsbView>(); }},
    });
    set_max_rows(2);  // allow wider buttons
}

/* SystemMenuView ********************************************************/

void SystemMenuView::hackrf_mode(NavigationView& nav) {
    nav.push<ModalMessageView>("HackRF mode", " This mode enables HackRF\n functionality. To return,\n  press the reset button.\n\n  Switch to HackRF mode?", YESNO,
                               [this](bool choice) {
                                   if (choice) {
                                       EventDispatcher::request_stop();
                                   }
                               });
}

SystemMenuView::SystemMenuView(NavigationView& nav) {
    add_items({
        //{ "Play dead",				Color::red(),		&bitmap_icon_playdead,	[&nav](){ nav.push<PlayDeadView>(); } },
        {"Receive", Color::cyan(), &bitmap_icon_receivers, [&nav]() { nav.push<ReceiversMenuView>(); }},
        {"Transmit", Color::cyan(), &bitmap_icon_transmit, [&nav]() { nav.push<TransmittersMenuView>(); }},
        {"Capture", Color::red(), &bitmap_icon_capture, [&nav]() { nav.push<CaptureAppView>(); }},
        {"Replay", Color::green(), &bitmap_icon_replay, [&nav]() { nav.push<PlaylistView>(); }},
        {"Search", Color::yellow(), &bitmap_icon_search, [&nav]() { nav.push<SearchView>(); }},
        {"Scanner", Color::green(), &bitmap_icon_scanner, [&nav]() { nav.push<ScannerView>(); }},
        {"Microphone", Color::green(), &bitmap_icon_microphone, [&nav]() { nav.push<MicTXView>(); }},
        {"Looking Glass", Color::green(), &bitmap_icon_looking, [&nav]() { nav.push<GlassView>(); }},
        {"Utilities", Color::cyan(), &bitmap_icon_utilities, [&nav]() { nav.push<UtilitiesMenuView>(); }},
        {"Settings", Color::cyan(), &bitmap_icon_setup, [&nav]() { nav.push<SettingsMenuView>(); }},
        {"Debug", Color::light_grey(), &bitmap_icon_debug, [&nav]() { nav.push<DebugMenuView>(); }},
        {"HackRF", Color::cyan(), &bitmap_icon_hackrf, [this, &nav]() { hackrf_mode(nav); }},
        //{ "About", 		Color::cyan(),			nullptr,				[&nav](){ nav.push<AboutView>(); } }
    });
    set_max_rows(2);  // allow wider buttons
    set_arrow_enabled(false);
    // set_highlighted(1);		// Startup selection
}

/* SystemView ************************************************************/

SystemView::SystemView(
    Context& context,
    const Rect parent_rect)
    : View{parent_rect},
      context_(context) {
    set_style(&Styles::white);

    constexpr Dim status_view_height = 16;
    constexpr Dim info_view_height = 16;

    add_child(&status_view);
    status_view.set_parent_rect({{0, 0},
                                 {parent_rect.width(), status_view_height}});
    status_view.on_back = [this]() {
        this->navigation_view.pop();
    };

    add_child(&navigation_view);
    navigation_view.set_parent_rect({{0, status_view_height},
                                     {parent_rect.width(), static_cast<Dim>(parent_rect.height() - status_view_height)}});

    add_child(&info_view);
    info_view.set_parent_rect({{0, 19 * 16},
                               {parent_rect.width(), info_view_height}});

    navigation_view.on_view_changed = [this](const View& new_view) {
        if (!this->navigation_view.is_top()) {
            remove_child(&info_view);
        } else {
            add_child(&info_view);
            info_view.refresh();
        }

        this->status_view.set_back_enabled(!this->navigation_view.is_top());
        this->status_view.set_title_image_enabled(this->navigation_view.is_top());
        this->status_view.set_title(new_view.title());
        this->status_view.set_dirty();
    };

    // pmem::set_playdead_sequence(0x8D1);

    // Initial view
    /*if ((pmem::playing_dead() == 0x5920C1DF) ||		// Enable code
                (pmem::ui_config() & 16)) {					// Login option
                navigation_view.push<PlayDeadView>();
        } else {*/

    navigation_view.push<SystemMenuView>();

    if (pmem::config_splash()) {
        navigation_view.push<BMPView>();
    }
    status_view.set_back_enabled(false);
    status_view.set_title_image_enabled(true);
    status_view.set_dirty();
    // else
    //	navigation_view.push<SystemMenuView>();

    //}
}

Context& SystemView::context() const {
    return context_;
}

void SystemView::toggle_overlay() {
    switch (++overlay_active) {
        case 1:
            this->add_child(&this->overlay);
            this->set_dirty();
            shared_memory.request_m4_performance_counter = 1;
            shared_memory.m4_cpu_usage = 0;
            shared_memory.m4_heap_usage = 0;
            shared_memory.m4_stack_usage = 0;
            break;
        case 2:
            this->remove_child(&this->overlay);
            this->add_child(&this->overlay2);
            this->set_dirty();
            shared_memory.request_m4_performance_counter = 0;
            break;
        case 3:
            this->remove_child(&this->overlay2);
            this->set_dirty();
            overlay_active = 0;
            break;
    }
}

void SystemView::paint_overlay() {
    static bool last_paint_state = false;
    if (overlay_active) {
        // paint background only every other second
        if ((((chTimeNow() >> 10) & 0x01) == 0x01) == last_paint_state)
            return;

        last_paint_state = !last_paint_state;
        if (overlay_active == 1)
            this->overlay.set_dirty();
        else
            this->overlay2.set_dirty();
    }
}

/* ***********************************************************************/

void BMPView::focus() {
    button_done.focus();
}

BMPView::BMPView(NavigationView& nav) {
    add_children({&button_done});

    button_done.on_select = [this, &nav](Button&) {
        nav.pop();
    };
}

void BMPView::paint(Painter&) {
    if (!portapack::display.drawBMP2({0, 0}, splash_dot_bmp))
        portapack::display.drawBMP({(240 - 230) / 2, (320 - 50) / 2 - 10}, splash_bmp, false);
}

/* NotImplementedView ****************************************************/

/*NotImplementedView::NotImplementedView(NavigationView& nav) {
        button_done.on_select = [&nav](Button&){
                nav.pop();
        };

        add_children({
                &text_title,
                &button_done,
        });
}

void NotImplementedView::focus() {
        button_done.focus();
}*/

/* ModalMessageView ******************************************************/

ModalMessageView::ModalMessageView(
    NavigationView& nav,
    const std::string& title,
    const std::string& message,
    const modal_t type,
    const std::function<void(bool)> on_choice)
    : title_{title},
      message_{message},
      type_{type},
      on_choice_{on_choice} {
    if (type == INFO) {
        add_child(&button_ok);

        button_ok.on_select = [this, &nav](Button&) {
            if (on_choice_)
                on_choice_(true);  // Assumes handler will pop.
            else
                nav.pop();
        };
    } else if (type == YESNO) {
        add_children({&button_yes,
                      &button_no});

        button_yes.on_select = [this, &nav](Button&) {
            if (on_choice_) on_choice_(true);
            nav.pop();
        };
        button_no.on_select = [this, &nav](Button&) {
            if (on_choice_) on_choice_(false);
            nav.pop();
        };
    } else if (type == YESCANCEL) {
        add_children({&button_yes,
                      &button_no});

        button_yes.on_select = [this, &nav](Button&) {
            if (on_choice_) on_choice_(true);
            nav.pop();
        };
        button_no.on_select = [this, &nav](Button&) {
            // if (on_choice_) on_choice_(false);
            nav.pop_modal();
        };
    } else {  // ABORT
        add_child(&button_ok);

        button_ok.on_select = [this, &nav](Button&) {
            if (on_choice_) on_choice_(true);
            nav.pop_modal();
        };
    }
}

void ModalMessageView::paint(Painter& painter) {
    size_t pos, i = 0, start = 0;

    portapack::display.drawBMP({100, 48}, modal_warning_bmp, false);

    // Terrible...
    while ((pos = message_.find("\n", start)) != std::string::npos) {
        painter.draw_string(
            {1 * 8, (Coord)(120 + (i * 16))},
            style(),
            message_.substr(start, pos - start));
        i++;
        start = pos + 1;
    }
    painter.draw_string(
        {1 * 8, (Coord)(120 + (i * 16))},
        style(),
        message_.substr(start, pos));
}

void ModalMessageView::focus() {
    if ((type_ == YESNO) || (type_ == YESCANCEL)) {
        button_yes.focus();
    } else {
        button_ok.focus();
    }
}

} /* namespace ui */
