/*
 * LoRa RX - MeshCore receiver (EU narrow: 869.618 MHz, BW 62.5 kHz, SF8, CR 4/8).
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

#ifndef __UI_LORA_RX_H__
#define __UI_LORA_RX_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_spectrum.hpp"
#include "ui_freq_field.hpp"
#include "ui_widget.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "receiver_model.hpp"
#include "message.hpp"

#include "meshcore.hpp"

namespace ui::external_app::lora_rx {

// Received messages, newest first: a metadata line in the small font, then the wrapped text.
class MessageList : public Widget {
   public:
    explicit MessageList(Rect parent_rect)
        : Widget{parent_rect} {}
    enum class Kind : uint8_t { Info,
                                Channel,
                                Advert,
                                Other,
                                BadCrc };
    // time and snr may be empty (info lines); text may be empty
    void add(const char* time, const char* snr, int snr_db, Kind kind, const char* title, const char* text);
    void paint(Painter& painter) override;
    void getAccessibilityText(std::string& result) override;
    void getWidgetName(std::string& result) override;

   private:
    static constexpr int MAX_ENTRIES = 8;
    struct Entry {
        char time[10];
        char snr[8];
        int8_t snr_db;
        Kind kind;
        char title[48];
        char text[160];
    };
    Entry entries_[MAX_ENTRIES]{};
    int count_{0};
    int newest_{-1};
};

class LoraRxView : public View {
   public:
    LoraRxView(NavigationView& nav);
    ~LoraRxView();
    LoraRxView(const LoraRxView&) = delete;
    LoraRxView& operator=(const LoraRxView&) = delete;

    void focus() override;
    void set_parent_rect(const Rect new_parent_rect) override;
    std::string title() const override { return "MeshCore RX"; };

   private:
    NavigationView& nav_;
    RxRadioState radio_state_{};
    app_settings::SettingsManager settings_{"rx_lora", app_settings::Mode::RX};

    static constexpr int MAX_CHANNELS = 16;  // Public + 15 from CHANNELS.TXT
    Channel channels[MAX_CHANNELS]{};
    int nchannels{0};
    int channels_ignored{0};  // lines beyond MAX_CHANNELS
    uint32_t count_all{0}, count_ok{0};
    // last packet, decoded (members: kept off the small M0 stack)
    char title_[48]{};
    char text_[160]{};
    char snr_[8]{};
    int8_t snr_db_{0};
    MessageList::Kind kind_{MessageList::Kind::Info};

    void load_channels();
    void on_packet(const LoraPacketMessage& m);
    __attribute__((noinline)) void show_packet();
    __attribute__((noinline)) void log_packet(const LoraPacketMessage& m);
    void update_status();
    void on_level(const LoraLevelMessage& m);
    void apply_gain();

    // AGC: gain_idx 0 = maximum gain (LNA 32 + VGA 30 dB), each step -4 dB
    static constexpr int GAIN_STEPS = 16;
    bool agc_on{true};
    int gain_idx{0};
    int quiet_reports{0};
    // gain before a packet made the ADC clip: restored as soon as that packet is over
    int gain_before_packet{-1};
    bool packet_seen{false};  // the decoder became busy since that gain drop
    uint8_t preambles_at_drop{0};
    int reports_since_drop{0};
    void restore_gain();
    uint8_t overruns{0};
    int gain_db() const { return 62 - 4 * gain_idx; }

    Labels labels{
        {{UI_POS_X(0), UI_POS_Y(0)}, "LNA:   VGA:   AMP:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(16), UI_POS_Y(1)}, "SF8 62.5k 4/8", Theme::getInstance()->fg_light->foreground},
    };
    LNAGainField field_lna{{UI_POS_X(4), UI_POS_Y(0)}};
    VGAGainField field_vga{{UI_POS_X(11), UI_POS_Y(0)}};
    RFAmpField field_rf_amp{{UI_POS_X(18), UI_POS_Y(0)}};
    RSSI rssi{{UI_POS_X(21), 0, UI_POS_WIDTH_REMAINING(21), 4}};

    RxFrequencyField field_frequency{{UI_POS_X(0), UI_POS_Y(1)}, nav_};
    OptionsField field_agc{{UI_POS_X(12), UI_POS_Y(1)}, 3, {{"AGC", 1}, {"Man", 0}}};
    Text text_status{{UI_POS_X(0), UI_POS_Y(2), UI_POS_MAXWIDTH, 16}, ""};
    MessageList messages{{0, UI_POS_Y(3), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(4)}};
    // channel waterfall in the bottom quarter (heap: created once the baseband runs)
    spectrum::WaterfallView* waterfall{nullptr};
    void layout(Rect r);

    MessageHandlerRegistration message_handler_packet{
        Message::ID::LoraPacket,
        [this](const Message* const p) {
            this->on_packet(*static_cast<const LoraPacketMessage*>(p));
        }};
    MessageHandlerRegistration message_handler_level{
        Message::ID::LoraLevel,
        [this](const Message* const p) {
            this->on_level(*static_cast<const LoraLevelMessage*>(p));
        }};
};

}  // namespace ui::external_app::lora_rx

#endif
