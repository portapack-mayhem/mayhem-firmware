/*
 * Copyright (C) 2026 Alexey Verhogladov
 *
 * This file is part of PortaPack.
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Meshtastic LoRa mesh app for HackRF + PortaPack H4M.
 * Autonomous Meshtastic node (no phone/BT required).
 * Tabs: Chat, Nodes, Map, Setup.
 */

#ifndef __UI_MESHTASTIC_H__
#define __UI_MESHTASTIC_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_tabview.hpp"
#include "ui_geomap.hpp"
#include "ui_qrcode.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "log_file.hpp"
#include "message.hpp"
#include "rtc_time.hpp"

#include "apps/mesh/mesh_regions.hpp"
#include "apps/mesh/mesh_protocol.hpp"
#include "apps/mesh/mesh_crypto.hpp"
#include "apps/mesh/mesh_nodedb.hpp"
#include "apps/mesh/mesh_router.hpp"
#include "apps/mesh/mesh_pki.hpp"

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <array>

namespace ui {

// ---- Log file --------------------------------------------------------------

class MeshtasticLogger {
   public:
    Optional<File::Error> append(const std::filesystem::path& filename) {
        return log_file.append(filename);
    }
    // One row per received frame. Everything here is known at the moment a frame
    // arrives, before anything is decrypted, so a packet on a channel we cannot read
    // is logged as faithfully as one we can - which is the interesting case when you
    // are asking who is on the air.
    void log_packet(const meshtastic::PacketHeader& header, int8_t rssi, int16_t snr_tenths, uint8_t length, uint8_t colour);

   private:
    LogFile log_file{};
    bool header_written_{false};
};

// ---- Chat message ----------------------------------------------------------

struct ChatMessage {
    std::string from_name;
    uint32_t from_id{0};
    uint32_t to_id{0};  // who it was addressed to (broadcast for channel text)
    std::string text;
    std::string timestamp;
    bool outgoing{false};
    uint32_t packet_id{0};  // our TX id, for ACK matching (0 = n/a)
    // 0=none, 1=pending, 2=delivered, 3=failed (a unicast nobody acknowledged),
    // 4=sent but never confirmed - which is all a broadcast can ever tell us.
    uint8_t status{0};
    uint32_t hist_offset{0};  // where its colour byte sits in the saved history
    // ...and WHICH history. The delivery byte is patched in place long after the line
    // was written, by which time the reader may be looking at another conversation -
    // and patching by offset alone wrote that byte into whatever file happened to be
    // open, corrupting an unrelated chat.
    uint32_t hist_peer{0};    // BROADCAST_ADDR for a channel, node id for a thread
    uint8_t hist_ch{0};       // channel index when hist_peer is BROADCAST_ADDR
    uint32_t sent_tick{0};    // uptime tick at send (for the ACK timeout)
    bool expects_ack{false};  // unicast -> time out to red; broadcast -> never red
    // Reception details, kept for the message card.
    int8_t rssi{0};
    uint8_t hops{0};
    uint8_t channel{0};
};

// Board and role names, so a node's card says what it is rather than a bare number.
// Only the models people actually run are named; anything else shows as its enum value.
// Board names, generated from mesh.proto HardwareModel so the list is the real one
// rather than the handful someone remembered. Shown in a node's card, beside its
// share code, and offered when choosing what to claim to be.
struct HwModelName {
    uint8_t id;
    const char* name;
};
static constexpr HwModelName HW_MODELS[] = {
    {0, "Unset"},
    {1, "TLoRa v2"},
    {2, "TLoRa v1"},
    {3, "TLoRa v2 1 1P6"},
    {4, "T-Beam"},
    {5, "Heltec v2 0"},
    {6, "T-Beam V0P7"},
    {7, "T Echo"},
    {8, "TLoRa v1 1P3"},
    {9, "RAK4631"},
    {10, "Heltec v2 1"},
    {11, "Heltec v1"},
    {12, "Lilygo T-Beam S3 C"},
    {13, "RAK11200"},
    {14, "Nano G1"},
    {15, "TLoRa v2 1 1P8"},
    {16, "TLoRa T3 S3"},
    {17, "Nano G1 Explorer"},
    {18, "Nano G2 Ultra"},
    {19, "LoRa Type"},
    {20, "Wiphone"},
    {21, "Wio Wm1110"},
    {22, "RAK2560"},
    {23, "Heltec Hru 3601"},
    {24, "Heltec Wireless Br"},
    {25, "Station G1"},
    {26, "RAK11310"},
    {27, "Senselora Rp2040"},
    {28, "Senselora S3"},
    {29, "Canaryone"},
    {30, "Rp2040 LoRa"},
    {31, "Station G2"},
    {32, "LoRa Relay v1"},
    {33, "T Echo Plus"},
    {34, "Ppr"},
    {35, "Genieblocks"},
    {36, "nRF52 Unknown"},
    {37, "Portduino"},
    {38, "Android Sim"},
    {39, "DIY v1"},
    {40, "nRF52840 PCA10059"},
    {41, "Dr Dev"},
    {42, "M5Stack"},
    {43, "Heltec v3"},
    {44, "Heltec Wsl v3"},
    {45, "Betafpv 2400 Tx"},
    {46, "Betafpv 900 Nano T"},
    {47, "RPi Pico"},
    {48, "Heltec Wireless Tr"},
    {49, "Heltec Wireless Pa"},
    {50, "T Deck"},
    {51, "T Watch S3"},
    {52, "Picomputer S3"},
    {53, "Heltec Ht62"},
    {54, "Ebyte ESP32 S3"},
    {55, "ESP32 S3 Pico"},
    {56, "Chatter 2"},
    {57, "Heltec Wireless Pa"},
    {58, "Heltec Wireless Tr"},
    {59, "Unphone"},
    {60, "Td LoRac"},
    {61, "Cdebyte Eora S3"},
    {62, "Twc Mesh V4"},
    {63, "nRF52 Promicro DIY"},
    {64, "Radiomaster 900 Ba"},
    {65, "Heltec Capsule Sen"},
    {66, "Heltec Vision Mast"},
    {67, "Heltec Vision Mast"},
    {68, "Heltec Vision Mast"},
    {69, "Heltec Mesh Node T"},
    {70, "Sensecap Indicator"},
    {71, "Tracker T1000 E"},
    {72, "RAK3172"},
    {73, "Wio E5"},
    {74, "Radiomaster 900 Ba"},
    {75, "Me25Ls01 4Y10Td"},
    {76, "Rp2040 Feather Rfm"},
    {77, "M5Stack Corebasic"},
    {78, "M5Stack Core2"},
    {79, "RPi Pico2"},
    {80, "M5Stack Cores3"},
    {81, "Seeed Xiao S3"},
    {82, "Ms24Sf1"},
    {83, "TLoRa C6"},
    {84, "Wismesh Tap"},
    {85, "Routastic"},
    {86, "Mesh Tab"},
    {87, "Meshlink"},
    {88, "Xiao nRF52 Kit"},
    {89, "Thinknode M1"},
    {90, "Thinknode M2"},
    {91, "T Eth Elite"},
    {92, "Heltec Sensor Hub"},
    {93, "Muzi Base"},
    {94, "Heltec Mesh Pocket"},
    {95, "Seeed Solar Node"},
    {96, "Nomadstar Meteor P"},
    {97, "Crowpanel"},
    {98, "Link 32"},
    {99, "Seeed Wio Tracker "},
    {100, "Seeed Wio Tracker "},
    {101, "Muzi R1 Neo"},
    {102, "T Deck Pro"},
    {103, "T LoRa Pager"},
    {104, "M5Stack Reserved"},
    {105, "Wismesh Tag"},
    {106, "RAK3312"},
    {107, "Thinknode M5"},
    {108, "Heltec Mesh Solar"},
    {109, "T Echo Lite"},
    {110, "Heltec V4"},
    {111, "M5Stack C6L"},
    {112, "M5Stack Cardputer "},
    {113, "Heltec Wireless Tr"},
    {114, "T Watch Ultra"},
    {115, "Thinknode M3"},
    {116, "Wismesh Tap v2"},
    {117, "RAK3401"},
    {118, "RAK6421"},
    {119, "Thinknode M4"},
    {120, "Thinknode M6"},
    {121, "Meshstick 1262"},
    {122, "T-Beam 1 Watt"},
    {123, "T5 S3 Epaper Pro"},
    {124, "T-Beam Bpf"},
    {125, "Mini Epaper S3"},
    {126, "Tdisplay S3 Pro"},
    {127, "Heltec Mesh Node T"},
    {128, "Tracker T1000 E Pr"},
    {129, "Thinknode M7"},
    {130, "Thinknode M8"},
    {131, "Thinknode M9"},
    {132, "Heltec V4 R8"},
    {133, "Heltec Mesh Node T"},
    {134, "Station G3"},
    {135, "T Impulse Plus"},
    {136, "T Echo Card"},
    {137, "Seeed Wio Tracker "},
    {138, "Crowpanel P4"},
    {139, "Heltec Mesh Tower "},
    {140, "Meshnology W10"},
    {255, "Private"},
};
static constexpr size_t NUM_HW_MODELS = sizeof(HW_MODELS) / sizeof(HW_MODELS[0]);

inline const char* hw_model_name(uint8_t m) {
    for (const auto& h : HW_MODELS)
        if (h.id == m) return h.name;
    return nullptr;
}

inline const char* mesh_role_name(uint8_t r) {
    switch (r) {
        case 0:
            return "Client";
        case 1:
            return "Client Mute";
        case 2:
            return "Router";
        case 3:
            return "Router Client";
        case 4:
            return "Repeater";
        case 5:
            return "Tracker";
        case 6:
            return "Sensor";
        case 7:
            return "TAK";
        case 8:
            return "Client Hidden";
        case 9:
            return "Lost and Found";
        case 10:
            return "TAK Tracker";
        default:
            return nullptr;
    }
}

// ---- Extra telemetry metrics ------------------------------------------------
// telemetry.proto carries far more than this device can measure. Rather than a widget
// per field, the whole set lives in this table and one editor walks it: pick a field,
// give it a value, tick "send". raw * scale is what goes on the wire, so an integer
// entry box can still express 12.3 m/s.
struct ExtraMetricDef {
    const char* name;  // shown in the picker
    uint8_t variant;   // TelemetryData::Variant it belongs to
    uint8_t field;     // protobuf field number in that sub-message
    bool is_float;     // float on the wire, or a varint
    float scale;       // wire value = raw * scale
    const char* unit;
    uint16_t rnd_max;  // ceiling for Randomize, so the values stay believable
    uint8_t group;     // index into EXTRA_GROUPS: related fields share a screen
};

static constexpr ExtraMetricDef EXTRA_METRICS[] = {
    // EnvironmentMetrics: the three a weather station always reports come first
    {"Temp", 1, 1, true, 0.1f, "C", 500, 10},
    {"Humidity", 1, 2, true, 0.1f, "%", 1000, 10},
    {"Pressure", 1, 3, true, 0.1f, "hPa", 11000, 10},
    // AirQualityMetrics: the readings a phone shows for a node
    {"PM2.5", 2, 2, false, 1.0f, "ug/m3", 90, 11},
    {"PM10", 2, 3, false, 1.0f, "ug/m3", 150, 11},
    {"CO2", 2, 13, false, 1.0f, "ppm", 2000, 11},
    // EnvironmentMetrics
    {"Gas resist", 1, 4, true, 0.1f, "MOhm", 900, 4},
    {"IAQ", 1, 7, false, 1.0f, "0-500", 500, 4},
    {"Distance", 1, 8, true, 1.0f, "mm", 5000, 5},
    {"Lux", 1, 9, true, 1.0f, "lx", 20000, 3},
    {"White lux", 1, 10, true, 1.0f, "lx", 20000, 3},
    {"IR lux", 1, 11, true, 1.0f, "lx", 5000, 3},
    {"UV lux", 1, 12, true, 1.0f, "lx", 500, 3},
    {"Wind dir", 1, 13, false, 1.0f, "deg", 359, 0},
    {"Wind speed", 1, 14, true, 0.1f, "m/s", 250, 0},
    {"Weight", 1, 15, true, 0.1f, "kg", 1500, 5},
    {"Wind gust", 1, 16, true, 0.1f, "m/s", 350, 0},
    {"Wind lull", 1, 17, true, 0.1f, "m/s", 120, 0},
    {"Radiation", 1, 18, true, 0.1f, "uR/h", 400, 5},
    {"Rain 1h", 1, 19, true, 0.1f, "mm", 200, 1},
    {"Rain 24h", 1, 20, true, 0.1f, "mm", 900, 1},
    {"Soil wet", 1, 21, false, 1.0f, "%", 100, 2},
    {"Soil temp", 1, 22, true, 0.1f, "C", 350, 2},
    // AirQualityMetrics
    {"PM1.0", 2, 1, false, 1.0f, "ug/m3", 60, 6},
    {"PM2.5 env", 2, 5, false, 1.0f, "ug/m3", 90, 6},
    {"PM10 env", 2, 6, false, 1.0f, "ug/m3", 150, 6},
    {"Part 0.3um", 2, 7, false, 1.0f, "/0.1L", 3000, 7},
    {"Part 2.5um", 2, 10, false, 1.0f, "/0.1L", 800, 7},
    {"Part 10um", 2, 12, false, 1.0f, "/0.1L", 200, 7},
    {"HCHO", 2, 16, true, 0.1f, "ppb", 300, 4},
    {"VOC index", 2, 23, true, 1.0f, "", 500, 4},
    {"NOx index", 2, 24, true, 1.0f, "", 300, 4},
    // PowerMetrics (three channels: solar, battery, load)
    {"Ch1 volts", 3, 1, true, 0.01f, "V", 1500, 8},
    {"Ch1 current", 3, 2, true, 1.0f, "mA", 2000, 8},
    {"Ch2 volts", 3, 3, true, 0.01f, "V", 500, 8},
    {"Ch2 current", 3, 4, true, 1.0f, "mA", 800, 8},
    {"Ch3 volts", 3, 5, true, 0.01f, "V", 600, 8},
    {"Ch3 current", 3, 6, true, 1.0f, "mA", 500, 8},
    // HealthMetrics
    {"Pulse", 4, 1, false, 1.0f, "bpm", 180, 9},
    {"SpO2", 4, 2, false, 1.0f, "%", 100, 9},
    {"Body temp", 4, 3, true, 0.1f, "C", 420, 9},
};
static constexpr size_t NUM_EXTRA_METRICS =
    sizeof(EXTRA_METRICS) / sizeof(EXTRA_METRICS[0]);

// Screens of related fields, so setting up a weather station is not a walk through
// thirty-five entries one at a time.
static constexpr const char* EXTRA_GROUPS[] = {
    "Wind", "Rain", "Soil", "Light", "Air chem", "Misc",
    "Particles PM", "Particle count", "Power", "Health",
    "Environment", "Air quality"};
static constexpr size_t NUM_EXTRA_GROUPS =
    sizeof(EXTRA_GROUPS) / sizeof(EXTRA_GROUPS[0]);

// ---- Telemetry override ----------------------------------------------------
// Per-parameter control of what we BROADCAST (the local Map keeps showing the
// real readings). batt_mode: 0=real, 1=custom %, 2=charging(101). volt/env
// mode: 0=real, 1=custom.
struct TelemetryOverride {
    uint8_t batt_mode{0};
    uint8_t batt_pct{50};
    uint8_t volt_mode{0};
    uint8_t volt_dV{37};  // decivolts (37 = 3.7 V)

    // Airtime: what share of the last hour the channel was busy and we transmitted.
    // "Real" reports the figures the app measures; "Custom" the two below.
    // Uptime. Real is what the app has been running, which is not what a node claiming
    // to be a solar repeater would say - and it is also a fingerprint, so Off is here
    // for the same reason it is on the rest of these.
    uint8_t up_mode{0};     // 0 = real, 1 = custom, 2 = off
    uint32_t up_hours{24};  // uint32 because the settings table has no 16-bit kind

    uint8_t util_mode{0};  // 0 = real, 1 = custom, 2 = off
    uint8_t chutil_pct{25};
    uint8_t airutil_pct{3};

    // What to claim for a fixed position; the live GPS supplies both when it has a fix.
    int32_t fixed_alt_m{0};
    uint8_t fixed_sats{0};

    // Every other metric in telemetry.proto, indexed by EXTRA_METRICS: a tick box and
    // a value each, rather than a widget per field for sixty-odd fields.
    uint8_t extra_on[NUM_EXTRA_METRICS]{};
    uint16_t extra_raw[NUM_EXTRA_METRICS]{};

    // Broadcast intervals in MINUTES (0 = never send this). Meshtastic nodes
    // default to sparse position updates (~30 min); keep the mesh un-spammy.
    uint8_t pos_min{15};  // POSITION broadcast period
    uint8_t tel_min{15};  // device/environment TELEMETRY broadcast period
};

// ---- Chat display options --------------------------------------------------
// How the chat renders and how much of it survives a restart. Kept in one struct so
// the Setup page and the chat view agree without a pile of loose references.
struct ChatDisplay {
    uint8_t time_mode{1};    // 0 = no timestamp, 1 = HH:MM, 2 = HH:MM:SS
    uint8_t name_mode{0};    // 0 = long name, 1 = short name, 2 = node id, 3 = none
                             // (3 leans on the per-node colour square instead)
    bool show_db{true};      // signal of the last packet in the status row
    bool show_tech{true};    // app status lines ("* ...") in the chat
    uint32_t save_count{0};  // messages kept on the SD card (0 = do not save)
    // Load /APPS/mesh_font.fnt so the chat can show alphabets the firmware font has
    // no glyphs for. Off by default, and it has to be: the table is about two
    // kilobytes held for as long as the app runs, and the on-screen keyboard wants
    // 4848 bytes in one piece. On a device this tight, having both means having
    // neither - turning it on can leave nothing to type a name or a message with.
    bool glyph_font{false};
    uint8_t font_size{16};    // 8 = compact 5x8 face, 16 = standard 8x16
    bool sf_notify{true};     // announce Store and Forward activity in the chat
    uint32_t hist_lines{20};  // wrapped lines of chat scrollback kept in memory
    // A message carrying the bell character is meant to get attention, so it can chime
    // even when ordinary messages are silent.
    bool bell_alert{true};
    // Delivery: how many times a direct message is repeated while no receipt comes
    // back, and how long to wait between attempts (seconds).
    uint32_t tx_retries{3};
    uint32_t tx_retry_s{12};
};

// ---- Everything this app remembers -----------------------------------------
// One structure, for the same reason ChatDisplay above is one: the settings screens
// used to take their share of it as separate reference parameters, and the last of
// them had grown to thirty - a constructor call nothing but the compiler could check,
// and one that had to be edited in three places to add a single option. A page now
// takes this and the hooks below, and nothing else.
// Everything here is persisted (see MeshtasticView::settings_bindings); runtime state
// stays on the view.
struct MeshSettings {
    // Custom channels 1..NUM_CUSTOM (router slots 1..N): slot 0 is the primary
    // (preset name + default PSK) and is always on. Router MAX_CHANNELS = 1 + this.
    static constexpr int NUM_CUSTOM = 10;

    uint8_t region_idx{meshtastic::DEFAULT_REGION};
    uint8_t preset_idx{meshtastic::DEFAULT_PRESET};
    char node_long_name[41]{"PortaPack H4M"};
    char node_short_name[5]{"PP4M"};
    // Persistence mirrors (SettingsManager binds std::string, not char[]); kept in
    // sync with the char[] working copies on load / save so the node name sticks.
    std::string node_long_str{"PortaPack H4M"};
    std::string node_short_str{"PP4M"};
    uint32_t node_id{0xDEADBEEF};
    uint8_t node_role{0};
    bool gps_enabled{true};
    float fixed_lat{0.0f};
    float fixed_lon{0.0f};
    uint8_t hop_limit{meshtastic::DEFAULT_HOP_LIMIT};
    uint8_t freq_slot{0};  // LoRaConfig.channel_num (0 = auto from name hash)
    // Coding rate denominator: 0 = whatever the preset says, otherwise 5..8 for
    // 4/5..4/8. Meshtastic ties this to the preset, so changing it only makes sense
    // when the other end is set the same way - the phone app hides it behind custom
    // LoRa settings for the same reason. Ours is honest about it: "preset" is default.
    uint8_t coding_rate{0};
    // Transmit power, and whose choice it is. HackRF's gain scale is NOT calibrated in
    // dBm - 47 is a position on a control, not a figure - so the app does not pretend to
    // enforce a legal limit. What "Region" does is honest and stated: it steps the gain
    // down by as many dB as the chosen region's limit sits below the most permissive one
    // in the table. "Max" asks for everything the radio has. Either way the firmware's
    // own global cap (config_tx_gain_max_db) still applies on top - this setting cannot
    // exceed what the operator has already allowed system-wide.
    uint8_t tx_pwr_mode{0};     // 0 = region, 1 = max, 2 = custom
    uint8_t tx_pwr_db{47};      // used when custom
    uint32_t nodeinfo_min{15};  // NodeInfo broadcast interval, minutes (Meshtastic default 15)
    // NeighborInfo broadcast interval, minutes. Meshtastic's default is six hours and
    // it refuses anything under four - the packet is heavy and the picture it draws
    // changes slowly. 0 = never.
    uint32_t nbr_min{360};
    bool ignore_mqtt{false};  // drop LoRa packets that came via MQTT
    bool sf_enabled{false};   // Store and Forward server
    // Both of these buy comfort with memory, and the memory is genuinely short: the
    // app leaves about two kilobytes of contiguous heap, and the on-screen keyboard
    // wants 4848 bytes of it in one piece. Raise them if your traffic needs it and you
    // do not mind the app running closer to the edge; the defaults are chosen to leave
    // room rather than to look generous.
    uint32_t sf_max{8};        // texts kept in the Store and Forward buffer
    uint32_t sf_ttl_min{120};  // and for how many minutes (0 = until the buffer fills)
    // Holding a key repeats it about eleven times a second after a quarter second, which
    // is far too eager for picking a menu entry or nudging a number. The repeat timing
    // is a firmware-wide constant, but whether a key repeats at all can be set per app.
    bool key_repeat{false};
    // 0 = send and receive, 1 = transmit only, 2 = receive only. Tapped on the chat's
    // RX/TX indicator; kept across restarts so a silent watch stays silent.
    uint8_t radio_mode{0};
    bool echo_enabled{false};  // range test: echo heard texts back with the RSSI
    // What the echo reply carries, and what triggers it. The reply is a range test:
    // the far end reads back how its own transmission arrived here. An empty keyword
    // answers every text; otherwise only the ones containing it - handy when several
    // people are testing range on the same channel at once.
    enum EchoBits : uint8_t { ECHO_TEXT = 1,
                              ECHO_SIG = 2,
                              ECHO_MEM = 4,
                              ECHO_UP = 8,
                              ECHO_BUILD = 16,
                              ECHO_BELL = 32 };
    uint8_t echo_mask{ECHO_TEXT | ECHO_SIG};
    std::string echo_key{};
    bool mqtt_ok{false};
    // HardwareModel advertised in NodeInfo. 255 = PRIVATE_HW; pick a common node
    // (Heltec V3, T-Beam, ...) so peers show us as that device.
    uint8_t hw_model{255};
    bool logging{false};
    // When false ("stealth"), we still receive/read packets but never send a
    // ROUTING ACK back - the peer's delivery indicator never turns green.
    bool send_read_receipts{true};
    // Listen only: no NodeInfo, no position, no telemetry, no neighbour list. Reading a
    // channel without announcing yourself is a legitimate thing to want, and switching
    // it back off restores whatever intervals were set rather than losing them.
    bool listen_only{false};
    // Whether a node asking for our LocalStats gets an answer. There is nothing
    // automatic to switch off: Meshtastic never puts these counters on the air unasked
    // (DeviceTelemetryModule hands them to an attached phone and otherwise replies only
    // to a request), so this is the only control over them that exists.
    bool answer_stats{true};
    // Name of the PRIMARY channel. Empty means "follow the preset", which is what a
    // fresh Meshtastic node does - but only while ITS name field is empty too. A node
    // that has ever been configured stores an explicit name, and that name does NOT
    // change when its preset does: the peer here still said "LongFast" on SHORT_FAST.
    // The name is half the channel hash, so a mismatch is total - every broadcast is
    // dropped at the filter in one direction and undecryptable in the other, while PKC
    // unicast (channel hash 0) keeps working and hides it.
    std::string primary_name{};
    // Receiver gain. It lives in receiver_model, which is reset on every app launch, so
    // a range test that took an afternoon to tune came back at 32/32/0 after the next
    // flash. Defaults are the model's own.
    uint8_t rx_lna{32};
    uint8_t rx_vga{32};
    uint8_t rx_amp{0};
    uint32_t freq_override_hz{0};  // 0 = Auto: region/preset model (RU + SHORT_TURBO -> 868.950 MHz)
    // Identity randomisation: interval in minutes (0 = off) and what it touches.
    enum RandBits : uint8_t { RAND_ID = 1,
                              RAND_NAME = 2,
                              RAND_ROLE = 4,
                              RAND_DEV = 8,
                              RAND_POS = 16,
                              RAND_TEL = 32 };
    uint32_t rand_min{0};
    uint8_t rand_mask{RAND_ID | RAND_NAME | RAND_DEV};
    bool beep_on_rx{false};  // play a chime when a new text message arrives
    // PKC (public-key crypto) for direct messages. When on, we advertise our
    // Curve25519 public key in NodeInfo (peers show a green padlock) and encrypt
    // DMs to known-key nodes with AES-256-CCM. Private key persists as 64 hex chars.
    bool pki_enabled{false};
    std::string pki_priv_hex{};

    ChatDisplay chat{};             // Setup -> Chat options
    TelemetryOverride telemetry{};  // user overrides for what we broadcast
    // The custom telemetry fields ride to the card in one packed string.
    std::string extra_pack{};

    std::string ch_name[NUM_CUSTOM]{};
    std::string ch_key[NUM_CUSTOM]{};
    bool ch_enabled[NUM_CUSTOM]{};
    uint8_t active_channel{0};  // 0 = primary
};

// ---- What a settings page can ask the app to do ----------------------------
// The pages themselves only write to MeshSettings; anything that has to happen in the
// world - restarting the receiver, announcing ourselves, clearing the history - is the
// app's business, and it batches the request rather than acting on every encoder click.
struct MeshSettingsHooks {
    std::function<void()> apply{};          // a setting changed
    std::function<void()> apply_radio{};    // ...and the receiver has to be restarted
    std::function<void()> resend{};         // announce NodeInfo now
    std::function<void()> clear_history{};  // the app owns every conversation file
    std::function<void()> pick_on_map{};    // take a position off the map
    // Show our own contact code. The owner does it because our public key lives with
    // it, not in the settings.
    std::function<void()> share_me{};
    // Put the sensor readings on the air now, rather than waiting for the interval.
    std::function<void()> send_telemetry{};
};

// ---- MeshConsole: UTF-8 chat console ---------------------------------------
// The shared ui::Console renders byte-by-byte with the Latin-1 firmware font, so
// UTF-8 peer text (Cyrillic, emoji) came out as mojibake (each Cyrillic letter = 2
// garbage glyphs, each emoji = 4). MeshConsole decodes UTF-8 and draws: ASCII/Latin-1
// from the built-in fixed_8x16 font, Cyrillic from an 8x16 glyph table loaded from the
// SD card into RAM (the SPI flash is full, so the glyphs can't be baked in), and known
// emoji as short ASCII tokens (->"+1"); anything else -> a "." placeholder. Entirely
// self-contained to this app - no shared font/UI/baseband changes.
// ---- Text entry ------------------------------------------------------------
// The firmware's own keyboard builds a widget object per key: 29 Buttons and their
// strings and callbacks, 5704 bytes measured on the way in. That is a fair price for
// an app that holds two kilobytes, and an impossible one for this app, which holds
// sixteen: entering a name, a passphrase or a message ended in an out-of-memory panic
// once a few screens had been visited. This one draws its keys in paint() from a
// static table, so a keyboard costs a view object and a cursor rather than five
// kilobytes of widgets.
//
// Deliberately not a general replacement: it has one layout, no raw-code entry and no
// per-app customisation, which is exactly what lets it be cheap. The shared
// AlphanumView stays where it is for every other app in the firmware.
class MeshTextEntry : public View {
   public:
    MeshTextEntry(NavigationView& nav,
                  std::string& str,
                  size_t max_len,
                  bool start_numeric,
                  std::function<void(std::string&)> on_done);

    ~MeshTextEntry();

    // Called by the app's frame tick; see mesh_text_entry_tick().
    void on_frame();

    void focus() override;
    void paint(Painter& painter) override;
    bool on_key(const KeyEvent key) override;
    bool on_encoder(const EncoderEvent delta) override;
    bool on_touch(const TouchEvent event) override;
    // Characters injected over the USB console arrive here, which is also how the
    // on-air tests type without a finger.
    bool on_keyboard(const KeyboardEvent event) override;
    std::string title() const override { return "Text entry"; }

   private:
    // QWERTY, ten to a row, because that is the arrangement everyone's fingers
    // already know. Alphabetical order looks tidier and is slower to use.
    static constexpr int COLS = 10;
    static constexpr int ROWS = 4;
    static constexpr int KEYS = COLS * ROWS;  // 40 per set
    // Two rows to type into, and the keys pushed down to make room: one row meant a
    // message scrolled out of sight while it was still being written.
    static constexpr int TEXT_ROWS = 2;
    static constexpr int TEXT_H = 8 + TEXT_ROWS * 16 + 2;  // 42
    static constexpr int KEY_W = 240 / COLS;               // 24
    static constexpr int KEY_H = 50;
    static constexpr int GRID_Y = TEXT_H;
    static constexpr int CTRL_Y = GRID_Y + ROWS * KEY_H + 2;
    static constexpr int CTRL_H = 44;
    // No Esc: the back arrow in the title bar already leaves without saving, and two
    // ways out of one screen is one too many. The width it freed went to the space bar,
    // which is the key a thumb reaches for most.
    static constexpr int CTRLS = 5;  // shift, script, space, delete, ok
    static const uint8_t CTRL_W[CTRLS];
    // Ticks of quiet demanded between one release and the next press: long enough to
    // swallow a bouncing contact, short enough not to slow real typing.
    static constexpr uint32_t TOUCH_GUARD = 60;
    // The text area is a place the selection can be, not just somewhere to look: with
    // it selected, Left and Right walk the caret through what has been typed and the
    // encoder does the same, so a typo in the middle is reachable without a touch.
    static constexpr int8_t FIELD = -2;

    // Four sets of exactly KEYS codepoints, so the grid never has a hole in it: two
    // Latin, two Cyrillic. Codepoints rather than characters because Cyrillic does not
    // fit in a byte, and because a key has to be drawn from the card's glyph table.
    static const uint16_t SETS[4][KEYS];
    static const char* const SET_NAMES[5];
    // The Cyrillic pair is offered only when the glyph table is loaded. Typing letters
    // that appear neither on the keys nor in the chat is not a feature.
    //
    // And a fifth page, built at startup from the table itself: every codepoint it
    // carries that none of the four fixed layouts already offers. Ukrainian needs four
    // letters Russian does not have, Kazakh sixteen, Romanian two, and naming them
    // language by language in here is the enumeration the glyph ranges exist to avoid.
    // Whoever holds the card decides which alphabet the keys speak.
    uint16_t extras_[KEYS]{};
    uint8_t extras_n_{0};
    void build_extras();

    NavigationView& nav_;
    std::string& str_;
    size_t max_len_;
    std::function<void(std::string&)> on_done_;
    // Case and script are two different questions, and one key answering both meant
    // four presses to get from "a" to "A" once Cyrillic was on the card.
    uint8_t script_{0};  // 0 latin, 1 cyrillic, 2 the card's extras
    uint8_t shift_{0};   // 0 lower, 1 for the next character only, 2 locked
    int8_t sel_{0};      // 0..KEYS-1 in the grid, KEYS..KEYS+CTRLS-1 on the control row
    // A key twenty-four pixels wide and a fingertip do not agree on where a press
    // landed. The key under the finger lights up while it is down and only fires when
    // it lifts, so a wrong one can be corrected by sliding rather than by deleting.
    bool pressing_{false};
    // Repainting forty-five keys takes long enough that doing it on every touch-move
    // buried the release event: the key lit up, and fired seconds later when the queue
    // caught up. Only what changed is drawn now.
    bool redraw_all_{true};
    int8_t drawn_sel_{-1};
    size_t drawn_len_{0};
    // One touch, one character. The key is decided where the finger lands and does not
    // follow it afterwards: a resistive panel reports a dragged finger as a stream of
    // presses, and letting those through meant sliding across the keys typed all of
    // them - and sent the result. Contact bounces too, so a new press is ignored for a
    // moment after the last one lifted.
    uint32_t last_release_{0};
    // A caret that blinks says "typing happens here" in a way a static block does not.
    // Driven by the same 60 Hz frame sync the rest of the app runs on.
    // Held DEL wipes the line. Counted in frames, because a finger that is still down
    // sends nothing at all: the panel reports the press and then the release, and
    // silence in between is what "held" looks like.
    uint16_t hold_{0};
    bool repeated_{false};
    static constexpr uint16_t HOLD_START = 30;  // half a second before it runs on
    static constexpr uint16_t HOLD_EVERY = 5;   // then a character every 80 ms
    // Held keys repeat: letters, space and delete. Shift, script and OK do not - a
    // finger resting on OK should send once, not once every eighty milliseconds.
    bool repeatable(int8_t s) const;
    int ctrl_x(int idx) const;  // controls are no longer all the same width
    // Where the next character goes, as a byte index into the string. Tapping the text
    // area puts it where the finger landed, so a typo in the middle is fixed in place
    // instead of by deleting everything after it.
    size_t caret_{0};
    size_t view_start_{0};  // first character shown, in characters not bytes
    bool caret_on_{true};
    uint8_t caret_tick_{0};
    bool caret_drawn_{true};
    // No handler of its own: the framework allows one registration per message and the
    // app already owns DisplayFrameSync. Registering a second panicked with MsgDblReg
    // the moment the keyboard opened. The app forwards the tick instead - see
    // mesh_text_entry_tick().
    void draw_text_line(Painter& painter);
    void draw_caret(Painter& painter);
    bool del_one();  // one whole character, not one byte of one
    // The string is UTF-8, so a column on screen is not a byte in the buffer.
    size_t char_count() const;
    size_t byte_at(size_t chars) const;  // byte index of the nth character
    size_t chars_before(size_t bytes) const;
    void place_caret(Coord x, Coord y);  // from a touch inside the text area
    void follow_caret();                 // scroll the window so the caret shows
    uint8_t scripts_available() const;   // 1 without a glyph table, up to 3 with one

    void put(uint32_t cp);
    void activate();
    void draw_key(Painter& painter, int idx);
    void draw_ctrl(Painter& painter, int idx);
    int hit(Coord x, Coord y) const;  // -1 when the touch is on nothing
};

// Opens the entry above. Same shape as the firmware's text_prompt so the call sites
// read the same, minus the keyboard-mode enum: this one starts on letters or on
// digits and the user switches from there.
// Ticked by whoever owns DisplayFrameSync, for the blinking caret and for a held
// DEL. Does nothing when no entry is open.
void mesh_text_entry_tick();

void mesh_prompt(NavigationView& nav,
                 std::string& str,
                 size_t max_len,
                 std::function<void(std::string&)> on_done,
                 bool numeric = false);

// The RX/TX direction lock, drawn as text in the status row. A Text that says which
// way the radio is held and takes a press to change it - so it has to show focus, and
// the plain widget has no notion of it. Inverted while focused, plain otherwise; the
// mode itself is in the square brackets, which are part of the string and cannot be
// painted over by anything.
class RadioModeText : public Text {
   public:
    using Text::Text;
    void on_focus() override;
    void on_blur() override;
    // Text answers a finger and nothing else: its on_select is wired to on_touch alone,
    // so a control reachable by the navigation cross could not be pressed by it.
    bool on_key(const KeyEvent key) override;

   private:
    void restyle(bool focused);
};

class MeshConsole : public Widget {
   public:
    explicit MeshConsole(Rect parent_rect);

    void write(const std::string& utf8);
    // A status line the app produced (node joined, relayed, Store and Forward, ...)
    // rather than mesh text, so the chat can be filtered down to real conversation.
    void write_tech(const std::string& utf8);
    // Same, drawn entirely in one of the node palette colours - used to announce a
    // node in its own colour, so the chat and the node list agree at a glance.
    void write_tech_col(const std::string& utf8, uint8_t colour);
    // 0 = everything, 1 = messages only, 2 = technical only.
    void set_filter(uint8_t mode) {
        filter_ = mode;
        scroll_back_ = 0;
        set_dirty();
    }
    uint8_t filter() const { return filter_; }
    // Which chat message a screen row belongs to (SIZE_MAX = none). Used to open a
    // message's details by tapping it.
    size_t message_at(Coord y) const;
    // Tag the lines written next with a chat-message index, so they can be tapped.
    void set_message_index(size_t idx) { pending_msg_ = static_cast<uint32_t>(idx) + 1; }
    void clear(bool clear_buffer);
    void paint(Painter& painter) override;
    // Scrollback: the console keeps a larger history than fits on screen; when it
    // holds focus, the encoder / Up-Down page through it (releasing focus at the
    // ends so it never traps navigation). A new message jumps back to the bottom.
    bool on_key(const KeyEvent key) override;
    bool on_encoder(const EncoderEvent delta) override;

    // Write a line with a leading coloured square, tagged by packet_id so it can be
    // recoloured later. marker: 0=none, 1=pending (amber), 2=delivered (green),
    // 3=failed (red); 8+n = the sender node's own colour from the palette.
    void write_tagged(const std::string& utf8, uint8_t marker, uint32_t tag);
    // Recolour the dot of every still-visible line carrying this tag.
    void set_marker(uint32_t tag, uint8_t marker);
    // Attempts still to come for one message, drawn beside its line. An empty string
    // takes it away again - the message is either delivered or out of tries.
    uint32_t retry_tag() const { return retry_tag_; }
    void set_retry(uint32_t tag, const std::string& s) {
        retry_tag_ = s.empty() ? 0 : tag;
        retry_text_ = s;
        set_dirty();
    }

    // An optional 8x16 glyph table read from the SD card, for writing systems the
    // firmware font does not carry. Ranges of codepoints rather than one fixed script:
    // the cost of a script here is its glyph count, so the alphabetic ones - Cyrillic,
    // Greek, Hebrew, the Latin letters with diacritics Latin-1 leaves out - all fit the
    // same way, and none of them is built in at the expense of the others. Scripts that
    // need shaping (Arabic's contextual forms, the Indic conjuncts) or thousands of
    // glyphs (Han) are out of reach on this device whatever the file says.
    struct GlyphRange {
        uint16_t first{0};  // first codepoint of the run
        uint16_t index{0};  // where its glyph sits in the table
        uint8_t count{0};   // how many codepoints the run covers
    };
    static constexpr uint8_t MAX_RANGES = 16;
    void set_ext_font(const uint8_t* glyphs, const GlyphRange* ranges, uint8_t n);
    // Whether a table was loaded at all, so the chat can tell "this device has no
    // font for that alphabet" from "the file it has does not cover it".
    bool has_ext_font() const { return ext_glyphs_ != nullptr; }

    // Chat text size: 8 = the compact 5x8 face, 16 = the standard 8x16 one. Those are
    // the only two the firmware has, and neither scales cleanly.
    void set_font_size(uint8_t px);
    // How many wrapped lines of scrollback to keep. Not free: each line is a record with
    // a string, and this buffer is one of the larger things the app holds. See the note
    // on MeshSettings::hist_lines.
    void set_max_lines(size_t n);
    // True when the text just written contained characters the display has no glyph
    // for, or a bell. Both are read and cleared by the chat after each message.
    bool take_unicode_flag() {
        const bool v = had_unicode_;
        had_unicode_ = false;
        return v;
    }
    bool take_bell_flag() {
        const bool v = had_bell_;
        had_bell_ = false;
        return v;
    }

   private:
    int LINE_H{16};
    int CHAR_W{8};
    uint8_t font_size_{16};  // 8 or 16, the two faces the firmware carries
    bool small_font_{false};

    size_t cols_{30};
    size_t max_lines_{28};   // history buffer (not the on-screen count)
    size_t visible_{14};     // rows that fit on screen
    size_t scroll_back_{0};  // lines scrolled up from the newest (0 = bottom)
    // One record per wrapped display line. These used to be seven parallel vectors, and
    // clear() emptied only three of them: every index after a chat wipe then read the
    // colour, filter flag and message link of some other line. One struct cannot fall
    // out of step with itself.
    struct Line {
        std::string text{};
        uint32_t tag{0};    // packet id, so a delivery result can find its line
        uint32_t msg{0};    // chat message index + 1 (0 = not a message)
        uint8_t marker{0};  // delivery dot, or 8+n for a node's colour stripe
        uint8_t colour{0};  // palette index for the text (0 = theme colour)
        bool tech{false};   // app status line rather than mesh text
        bool mine{false};   // ours: drawn against the right edge
    };
    std::vector<Line> lines_{};
    uint8_t filter_{0};
    uint32_t retry_tag_{0};  // the message whose attempts are on show
    std::string retry_text_{};
    bool writing_tech_{false};
    uint32_t pending_msg_{0};
    uint8_t writing_col_{0};
    bool writing_mine_{false};  // set while one of our own messages is written
    // Newest-first list of the lines the current filter shows.
    // Indices of the lines the current filter lets through, newest first. Written into
    // a member buffer rather than a fresh vector: this runs on every repaint, sixty
    // times a second, and a heap allocation each time is exactly the kind of churn that
    // fragments the little memory this device has.
    void visible_lines(std::vector<size_t>& out) const;
    mutable std::vector<size_t> vis_buf_{};
    size_t cur_cols_{0};  // display columns used by the current line
    bool had_unicode_{false};
    bool had_bell_{false};

    const uint8_t* ext_glyphs_{nullptr};  // owned by the chat view, not by us
    GlyphRange ext_ranges_[MAX_RANGES]{};
    uint8_t ext_nranges_{0};
    // Glyph index for a codepoint in the loaded table, or -1 when it has none.
    int ext_index(uint32_t cp) const;

    void new_line();
    void append_out(uint32_t out_cp);  // append one display glyph, wrapping at cols_
    void emit_cp(uint32_t cp);         // sanitize one input codepoint -> 0+ display glyphs
    void draw_line(Coord y, const std::string& utf8, Coord indent, uint8_t colour);
    // How many character cells a line occupies once decoded, for right alignment.
    size_t glyph_columns(const std::string& utf8) const;
    void draw_marker(Coord x, Coord y, uint8_t marker);
};

// ============================================================================
// Layout: NavigationView gives apps {0,0,240,304}.
// TabView self-positions to {0,0,240,24} (3x8=24px tab bar).
// 4 tabs -> each tab button = 60px wide.
// Sub-views: {0, 24, 240, 280}  (screen_height - 40 = 280)
// ============================================================================

// ---- Tab: Chat  (index 0) --------------------------------------------------
// y=0..15   Status: L:XX V:XX A:X  [PKT]  RSSI
// y=16..247 Console (h=232)
// y=248..279 [Send(60)] [Ch:N(40)] [->Dest(80)] [Clear(60)]

class MeshtasticChatView : public View {
   public:
    // Built out of line on purpose. All four tab views are members of
    // MeshtasticView, so the compiler happily inlines their constructors into
    // its own - four widget trees' worth of temporaries in one stack frame,
    // 1536 bytes of the M0's 4 KB process stack. Launching the app by touch
    // runs that constructor several frames deeper than launching it with the
    // keys (the touch dispatcher recurses down the widget tree first), and the
    // difference was the whole margin: touch crashed, keys did not.
    __attribute__((noinline)) MeshtasticChatView(NavigationView& nav,
                                                 meshtastic::MeshRouter& router,
                                                 meshtastic::NodeDB& node_db,
                                                 Rect parent_rect);

    // Hands back the glyph table. It comes from chHeapAlloc rather than new, so
    // nothing else will: without this, up to 4 kB of the M0's 43 went missing on
    // every launch, and three launches in a session are enough to bring back the
    // out-of-memory panic this whole app fights.
    ~MeshtasticChatView();

    void focus() override;
    void paint(Painter& painter) override;
    bool on_touch(const TouchEvent event) override;
    void on_packet(const meshtastic::MeshPacket& pkt);
    // An incoming ROUTING ACK/NAK for one of our sent messages -> flip its dot
    // green (delivered) or red (failed).
    void on_ack(uint32_t request_id, bool is_ack);
    // Attempts ran out. Only a message that named a recipient can be called failed.
    void on_retries_done(uint32_t request_id);
    // Called ~60 Hz with the app uptime; ages pending messages -> red on timeout.
    void tick(uint32_t now_ticks);
    void set_pkt_indicator(bool tx);
    // A status line drawn in a node's colour (announcements).
    void write_colour(const std::string& s, uint8_t colour) {
        console_.write_tech_col(s, colour);
    }
    // Delivery attempts for the message in flight. It is drawn beside that message
    // rather than in the status row: the row also carries the message count, and once
    // that reaches double figures there is no room left for anything else.
    void set_retry(const std::string& s) { console_.set_retry(retry_tag_, s); }
    void set_retry_tag(uint32_t tag) { retry_tag_ = tag; }
    // Signal of the last decoded packet: level and signal-to-noise together, because
    // when comparing receiver settings the ratio is what actually matters.
    // Signal of the last decoded packet: level in dBm over signal-to-noise in dB.
    // Short, because it shares the row with the message counter.
    void set_last_signal(int8_t rssi, int16_t snr_tenths) {
        if (!rssi && !snr_tenths) return;
        text_rssi_.set(to_string_dec_int(rssi) + "/" +
                       to_string_dec_int(snr_tenths / 10));
    }
    // App status lines start with '*' by convention; route them to the console's
    // technical channel so the filter button can hide or isolate them.
    void write_console(const std::string& s) {
        if (!s.empty() && s[0] == '*')
            console_.write_tech(s);
        else
            console_.write(s);
    }
    // Parent wires this to MeshtasticView::queue_tx so a composed message is
    // actually transmitted (the Send button builds the packet but the chat view
    // has no radio - it must hand the bytes up to the owning view to key TX).
    void set_on_tx_request(std::function<void(const uint8_t*, size_t)> cb) {
        on_tx_request_ = std::move(cb);
    }
    // Owner restores the saved gain and is told when the user turns a knob.
    void set_gains(uint8_t lna, uint8_t vga, uint8_t amp);
    void set_on_gain(std::function<void(uint8_t, uint8_t, uint8_t)> cb) {
        on_gain_ = std::move(cb);
    }
    void report_gain();
    // The "Ch" button opens the Channels screen (owner holds the channel table).
    void set_on_channels(std::function<void()> cb) { on_channels_ = std::move(cb); }
    // Owner points this at its "beep on RX" flag; a chime plays when a text arrives.
    void set_beep_flag(const bool* p) { beep_flag_ = p; }
    // Radio direction lock, cycled by tapping the RX/TX indicator: 0 = both ways,
    // 1 = transmit only, 2 = receive only (nothing at all leaves the antenna, which is
    // the point when you would rather sit on a channel unnoticed).
    void set_on_radio_mode(std::function<void(uint8_t)> cb) { on_radio_mode_ = std::move(cb); }
    void set_radio_mode(uint8_t m);
    // Forget every saved conversation: the files on the card and what is on screen.
    // Also the way out when a history file has been left damaged by a crash.
    void clear_all_history();
    void init_radio_mode_switch();

    // Owner reports whether the active channel is encrypted; a padlock at the far
    // right of the status row shows locked (green) vs open (grey).
    void set_encrypted(bool e) {
        channel_private_ = e;
        update_padlock();
    }
    // Owner reports the active channel index. Each channel keeps its own conversation,
    // so switching channel switches the history shown (and the file it is saved to).
    void set_channel_index(uint8_t idx);
    // Update the "Ch" button caption to the active channel name.
    void set_channel_label(const std::string& s) { button_chan_.set_text(s); }
    // Apply the Setup -> Chat options (timestamps, name style, signal, status lines).
    void set_display(const ChatDisplay& d);
    // Console filter and its button caption, kept in step.
    void set_filter(uint8_t mode);
    // Read the Cyrillic font and the saved conversation. Called once from the app
    // timer rather than from the constructor: both open files, and the constructor is
    // the deepest point of an app launch.
    void load_deferred() {
        load_ext_font();
        load_history();
    }
    // Open a direct-message thread with a node (node detail "Message" button, or a
    // thread picked from the Channels list): outgoing text becomes unicast to it and
    // is PKC-encrypted when its public key is known. BROADCAST_ADDR = back to "All".
    // open_thread=false only re-points the destination (the ">" cycle button) without
    // adding the node to the conversations list.
    void set_dest(uint32_t node_id, bool open_thread = true);
    // The node whose private thread is open, or 0 when the chat is on a channel.
    uint32_t open_dm() const {
        return (dest_id_ == meshtastic::BROADCAST_ADDR) ? 0 : dest_id_;
    }

   private:
    // A glyph table read from the card at startup, for alphabets the firmware font does
    // not carry. Held here because the console only borrows it. Capped at 256 glyphs:
    // 4 kB is already a fifth of what the whole app is allowed, and a file claiming
    // more is a mistake, not a wish.
    static constexpr uint32_t MAX_EXT_GLYPHS = 256;
    // What AlphanumView asks for the first time a text prompt opens in a session. The
    // glyph table must never eat into it: a device that cannot be typed on is worse
    // than one that cannot show every alphabet.
    static constexpr uint32_t KEYBOARD_BYTES = 4848;
    uint8_t* ext_font_{nullptr};
    bool ext_font_short_{false};  // a table was there, but loading it would have hurt
    void load_ext_font();

    static constexpr int SV_H = 280;
    static constexpr int STATUS_H = 16;
    static constexpr int BOT_H = 24;  // slim button row -> the chat gets more height
    static constexpr int CON_Y = STATUS_H;
    static constexpr int CON_H = SV_H - STATUS_H - BOT_H;  // 240
    static constexpr int BOT_Y = SV_H - BOT_H;             // 256

    NavigationView& nav_;
    meshtastic::MeshRouter& router_;
    meshtastic::NodeDB& node_db_;

    std::vector<ChatMessage> messages_{};
    static constexpr size_t MAX_MESSAGES = 16;
    uint32_t total_msgs_{0};  // running count of all messages (sent + received)
    std::string compose_str_{};

    uint32_t now_ticks_{0};
    // No ACK within this many ~60 Hz ticks -> mark the message failed (red).
    static constexpr uint32_t ACK_TIMEOUT_TICKS = 30 * 60;  // ~30 s

    // Default to broadcast - the dest button starts labelled ">All", so dest_id_
    // must match (was 0, which sent dest=0x00000000 that real Meshtastic drops).
    uint32_t dest_id_{meshtastic::BROADCAST_ADDR};
    uint8_t dest_node_idx_{0};

    // Caption the destination button for the current dest_id_ (">All" or ">peer").
    void apply_dest_label();

    std::function<void(const uint8_t*, size_t)> on_tx_request_{};
    std::function<void(uint8_t, uint8_t, uint8_t)> on_gain_{};
    bool loading_gain_{false};  // set_gains must not write the settings straight back
    std::function<void()> on_channels_{};
    const bool* beep_flag_{nullptr};  // -> owner's beep_on_rx_ setting (null = no beep)
    ChatDisplay display_{};           // copy of the Setup -> Chat options
    // Appends one line per message so a restart does not lose the conversation.
    // Returns where the line's colour byte landed in the file, so a later delivery
    // result can be written back over it.
    uint32_t save_line(const std::string& line, uint8_t marker = 0);
    void patch_saved_marker(const ChatMessage& m, uint8_t marker);
    uint32_t tag_at_offset(uint32_t off) const;
    // Append to a conversation that is not the one on screen (a direct message that
    // arrived while a channel is open, and the other way round).
    uint32_t save_line_to(const std::u16string& path, const std::string& line, uint8_t marker = 0);
    // Timestamp for a new message, per the display options.
    std::string stamp() const;
    // Same time, written after the text: used for our own right-aligned messages.
    std::string stamp_after() const;
    // How a peer is labelled, per the display options.
    std::string peer_label(const meshtastic::NodeEntry* e, uint32_t id) const;
    uint8_t radio_mode_{0};
    std::function<void(uint8_t)> on_radio_mode_{};
    void apply_radio_mode();

    bool active_encrypted_{true};  // padlock as drawn right now
    bool channel_private_{false};  // active channel has a passphrase of its own
    uint8_t channel_idx_{0};       // which channel the console is showing

    // The padlock follows the conversation, not just the channel: a direct message to
    // a node whose public key we hold is end-to-end encrypted and shows locked even on
    // the public channel, which is exactly where a private chat is least expected.
    void update_padlock();
    // Where this conversation is kept on the card: one file per channel and one per
    // direct-message peer, so histories never mix.
    std::u16string history_path() const { return conversation_path(dest_id_, channel_idx_); }
    // Path of an arbitrary conversation: peer for a direct message, channel otherwise.
    static std::u16string conversation_path(uint32_t peer, uint8_t channel);
    // Show the conversation for the current channel / peer: the console is cleared and
    // refilled from that conversation's file (empty when it was never saved).
    void switch_conversation();

    Labels labels_rf_{
        {{0, 0}, "L:", Theme::getInstance()->fg_light->foreground},
        {{4 * 8, 0}, "V:", Theme::getInstance()->fg_light->foreground},
        {{8 * 8, 0}, "A:", Theme::getInstance()->fg_light->foreground}};
    NumberField field_lna_{{2 * 8, 0}, 2, {0, 40}, 8, ' '};
    NumberField field_vga_{{6 * 8, 0}, 2, {0, 62}, 2, ' '};
    NumberField field_amp_{{10 * 8, 0}, 1, {0, 1}, 1, ' '};
    RadioModeText text_pkt_{{11 * 8 + 4, 0, 4 * 8, 16}, "    "};
    // Total messages (sent + received) shown to the right of RX/TX. The app has no
    // meaningful per-packet signal bar, so this reuses that otherwise-unused space.
    // Width leaves ~12 px at the far right for the encryption padlock.
    // Five characters, not four: "m:100" ran out of box and the last digit landed on
    // the signal reading beside it.
    Text text_count_{{16 * 8, 0, 5 * 8, 16}, ""};
    // Delivery attempts of the message still waiting for a receipt ("2/3"), shown
    // only while it is being retried and cleared the moment it lands or gives up.
    uint32_t retry_tag_{0};  // packet id of the message being retried
    // Signal of the last packet we decoded. The live RSSI widget cannot come back:
    // it repaints on every baseband update and those framebuffer writes starve the
    // LoRa receiver. This costs one repaint per received packet instead.
    // Ends at 228, where the padlock begins: a Text paints its whole rect, so the eight
    // pixels I saved here were painted black straight over the padlock's left third.
    Text text_rssi_{{21 * 8, 0, 240 - 21 * 8 - 12, 16}, ""};

    MeshConsole console_{{0, CON_Y, screen_width, CON_H}};

    void load_history();

    // Chat is the main window, so the button row is compact: a >> send glyph, the
    // channel button, a WIDE node-filter (long peer names fit), and an X clear.
    Button button_send_{{0, BOT_Y, 26, BOT_H}, "\xBB"};  // >> = send
    Button button_chan_{{26, BOT_Y, 42, BOT_H}, "Ch:0"};
    Button button_dest_{{68, BOT_Y, 108, BOT_H}, ">All"};
    // Cycles the console filter: everything / messages only / status lines only.
    Button button_filter_{{176, BOT_Y, 38, BOT_H}, "All"};
    Button button_clear_{{214, BOT_Y, 26, BOT_H}, "X"};
};

// ---- Message detail (full-screen, pushed by tapping a chat line) -----------
// Everything known about one message plus a resend, which is the quickest way to
// retry a direct message that timed out without retyping it.

// A share code the size of the screen: a phone reads it from across the room, which
// is the whole point of handing someone a channel key at a meet-up.
class MeshtasticQRView : public View {
   public:
    // caption lines describe what the code carries (node name, board, key state), so a
    // single screen is enough to decide whether to add it.
    MeshtasticQRView(NavigationView& nav, const std::string& text, std::string line1 = {}, std::string line2 = {});
    std::string title() const override { return "Share"; }
    void focus() override { button_close_.focus(); }
    void paint(Painter& painter) override;

   private:
    std::string text_;
    std::string line1_, line2_;
    QRCodeImage image_{{16, 62, screen_width - 32, screen_width - 32}};
    Button button_close_{{0, 272, screen_width, 32}, "Close"};
};

class MeshtasticMessageView : public View {
   public:
    MeshtasticMessageView(NavigationView& nav, const ChatMessage& msg, std::function<void(std::string, uint32_t)> on_resend);

    void focus() override;
    void paint(Painter& painter) override;
    std::string title() const override { return "Message"; }

   private:
    NavigationView& nav_;
    ChatMessage msg_;
    std::function<void(std::string, uint32_t)> on_resend_;

    Button button_resend_{{0, 304 - 34, screen_width, 32}, "Resend"};
};

// ---- Node detail (full-screen, pushed from the Nodes tab) ------------------
// Shows everything a node has advertised, and can ask it three questions.

// What the node card can ask a node for. One callback carries all three rather than
// three callbacks: a std::function costs more here than a parameter does.
enum NodeRequest : uint8_t {
    REQ_METRICS = 0,  // Telemetry, device variant: battery, voltage, airtime
    REQ_STATS = 1,    // Telemetry, LocalStats variant: the router's own counters
    REQ_TRACE = 2,    // RouteDiscovery: which path reaches this node
};

class MeshtasticNodeDetailView : public View {
   public:
    MeshtasticNodeDetailView(NavigationView& nav,
                             meshtastic::NodeDB& node_db,
                             uint32_t node_id,
                             std::function<void(uint32_t)> on_message = nullptr,
                             uint32_t now_ticks = 0,
                             std::function<void(uint32_t, uint8_t)> on_request_stats = nullptr);

    void focus() override;
    void paint(Painter& painter) override;
    std::string title() const override { return "Node"; }

   private:
    NavigationView& nav_;
    meshtastic::NodeDB& node_db_;
    uint32_t node_id_;
    uint32_t now_ticks_{0};  // app uptime, for "first heard N ago"
    // Owner opens a direct-message thread with this node (chat destination + Chat tab).
    std::function<void(uint32_t)> on_message_{};

    void show_on_map();

    // QRCodeView keeps the pointer it is given, so the string has to outlive it.
    std::string qr_url_{};
    void show_qr();
    // Owner sends a LocalStats request to this node ("Request local stats").
    // The argument says which question to ask: true = the router's own statistics,
    // false = the device metrics a phone shows (battery, voltage, airtime).
    // What the card can ask a node for: REQ_METRICS, REQ_STATS or REQ_TRACE below.
    std::function<void(uint32_t, uint8_t)> on_request_stats_{};
    // Announce ourselves and ask to be answered. See button_exchange_.
    std::function<void()> on_exchange_{};

   public:
    void set_on_exchange(std::function<void()> cb) { on_exchange_ = std::move(cb); }

   private:
    // Everything a Meshtastic node can report does not fit on one screen, so the page
    // selector swaps between the groups the phone app also separates.
    OptionsField opt_page_{
        {0, 4},
        13,
        // Stats and Route sit ahead of the sensor pages: whether the link works, and
        // by what path, is asked far more often than a humidity reading. The values
        // stay bound to their pages; only the order and the numbering move.
        {{"Identity 1/9", 0}, {"Radio 2/9", 1}, {"Stats 3/9", 7}, {"Route 4/9", 8}, {"Environ 5/9", 2}, {"Weather 6/9", 3}, {"Air qual 7/9", 4}, {"Power 8/9", 5}, {"Health 9/9", 6}}};
    uint8_t page_{0};

    // Identity page only: the colour this node's messages are marked with in the chat.
    // It sits down among the rows rather than beside the page selector: an OptionsField
    // swallows Left and Right to change its own value, so a field parked next to the
    // selector could never be reached - the selector ate every press that would have
    // moved the focus onto it.
    OptionsField options_colour_{{11 * 8, 174}, 8, {{"none", 0}, {"red", 1}, {"green", 2}, {"blue", 3}, {"yellow", 4}, {"cyan", 5}, {"magenta", 6}, {"orange", 7}}};

    // Four rows of buttons, paired where a pair fits: everything the card can do is
    // reachable without the list moving under your thumb, and the identity page keeps
    // the two lines its public key needs. "Exchange info" is what the phone app calls
    // exchange user info: the peer replies with its own, and that reply is the only
    // place its public key travels, so it is the button that starts a private
    // conversation. It is not page-specific - it asks the node, not the page.
    Button button_msg_{{0, 304 - 108, screen_width, 24}, "Message"};
    Button button_exchange_{{0, 304 - 82, 118, 24}, "Exchange info"};
    Button button_qr_{{122, 304 - 82, 118, 24}, "Share QR"};
    // "Map" needs a position and says so when there is none; the other three need
    // nothing, which is the point - they are what you can ask a node that has never
    // said where it is or what its battery is doing.
    Button button_map_{{0, 304 - 56, 118, 24}, "Map"};
    Button button_trace_{{122, 304 - 56, 118, 24}, "Trace"};
    Button button_stats_{{0, 304 - 30, 118, 24}, "Stats"};
    Button button_metrics_{{122, 304 - 30, 118, 24}, "Metrics"};
};

// ---- Nodes settings (pushed from the Nodes tab gear button) ----------------

class MeshtasticNodesConfigView : public View {
   public:
    MeshtasticNodesConfigView(NavigationView& nav, uint32_t& offline_after_s, uint32_t& forget_after_min);
    void focus() override;
    std::string title() const override { return "Node list"; }

   private:
    NavigationView& nav_;
    uint32_t& offline_after_s_;
    uint32_t& forget_after_min_;

    Labels label_off_{{{0, 16}, "Offline after (min):", Theme::getInstance()->fg_light->foreground}};
    NumberField field_off_{{21 * 8, 16}, 3, {1, 240}, 1, ' '};
    Labels label_forget_{{{0, 48}, "Forget after (min):", Theme::getInstance()->fg_light->foreground}};
    NumberField field_forget_{{21 * 8, 48}, 3, {0, 240}, 5, ' '};
    Labels label_hint_{
        {{0, 80}, "0 = keep nodes for ever.", Theme::getInstance()->fg_dark->foreground}};
};

// ---- Tab: Nodes  (index 1) -------------------------------------------------

class MeshtasticNodesView : public View {
   public:
    // Out of line: see MeshtasticChatView's constructor.
    __attribute__((noinline)) MeshtasticNodesView(NavigationView& nav,
                                                  meshtastic::NodeDB& node_db,
                                                  Rect parent_rect);

    void focus() override;
    void paint(Painter& painter) override;
    bool on_encoder(const EncoderEvent delta) override;
    bool on_key(const KeyEvent key) override;
    bool on_touch(const TouchEvent event) override;
    void refresh();
    // Wired by the owner: the node detail's "Message" button opens a DM thread.
    void set_on_message(std::function<void(uint32_t)> cb) { on_message_ = std::move(cb); }
    // Wired by the owner: ask a node for its LocalStats.
    void set_on_request_stats(std::function<void(uint32_t, uint8_t)> cb) { on_stats_ = std::move(cb); }
    // Wired by the owner: announce ourselves and ask the mesh to answer, so peers'
    // public keys arrive. Handed on to the node card, which is where the button is.
    void set_on_exchange(std::function<void()> cb) { on_exchange_ = std::move(cb); }
    // Current uptime (ticks) for "last heard" ages; repaints ~1/s while shown.
    void set_now(uint32_t ticks) {
        if (!hidden() && ticks >= now_ticks_ + 60) set_dirty();
        now_ticks_ = ticks;
    }

   private:
    static constexpr int SV_H = 280;
    static constexpr int BOT_H = 24;   // button row, like the chat
    static constexpr int FOOT_H = 16;  // "Nodes: n  Online: m"
    static constexpr int BOT_Y = SV_H - BOT_H;
    static constexpr int FOOT_Y = BOT_Y - FOOT_H;
    static constexpr int HDR_H = 16;
    static constexpr int ROW_H = 16;
    static constexpr int VISIBLE = (FOOT_Y - HDR_H) / ROW_H;

    // Sort keys, in the order the header columns appear.
    enum class Sort : uint8_t { Id = 0,
                                Name = 1,
                                Signal = 2,
                                Age = 3 };

    NavigationView& nav_;
    meshtastic::NodeDB& node_db_;

    size_t selected_{0};
    size_t scroll_offset_{0};
    uint32_t now_ticks_{0};
    uint32_t last_refresh_tick_{0};  // throttles RX-driven repaints
    std::function<void(uint32_t)> on_message_{};
    std::function<void(uint32_t, uint8_t)> on_stats_{};
    std::function<void()> on_exchange_{};
    // Newest first by default: a node that just appeared is the interesting one.
    Sort sort_{Sort::Age};
    bool sort_desc_{true};
    // Nodes quiet for longer than this many seconds count as offline.
    uint32_t offline_after_s_{900};
    // Drop a node from the list once it has been quiet this long (0 = never).
    uint32_t forget_after_min_{0};
    // Order (indices into the node DB) rebuilt whenever the list is drawn.
    std::vector<uint8_t> order_{};

    void rebuild_order();
    void toggle_sort(Sort key);
    void cycle_sort();  // the button: next direction, then the next column
    void update_sort_caption();
    void move_selection(int delta);
    void open_selected();

   public:
    // Also the "nodes online" field of LocalStats, which the owner fills in when a peer
    // asks for our counters - the same definition the list footer shows.
    size_t online_count() const;

   private:
    // Full width, evenly split, with a hair of a gap so they read as three keys.
    Button button_clear_{{0, BOT_Y, 78, BOT_H}, "Clear"};
    // Was "Refresh", which did nothing the list was not already doing for itself.
    // Sorting was only reachable by tapping a header title - discoverable by accident
    // at best - so the button now carries it, and says which way round it is.
    Button button_sort_{{81, BOT_Y, 78, BOT_H}, "Age v"};
    Button button_setup_{{162, BOT_Y, 78, BOT_H}, "Setup"};
};

// ---- Tab: Map  (index 2) ---------------------------------------------------

class MeshtasticMapView : public View {
   public:
    // Out of line: see MeshtasticChatView's constructor.
    __attribute__((noinline)) MeshtasticMapView(NavigationView& nav,
                                                meshtastic::NodeDB& node_db,
                                                Rect parent_rect);

    void focus() override;
    void paint(Painter& painter) override;
    void update_position(float lat, float lon, float alt, uint8_t sats = 0);
    // Show a manually-entered (Setup tab) position when the live GPS is off.
    void set_fixed_position(float lat, float lon, int32_t alt_m = 0, uint8_t sats = 0);
    void clear_fix() {
        has_fix_ = false;
        manual_ = false;
        set_dirty();
    }
    void set_battery(uint8_t pct, float volts, bool charging);
    void set_environment(float t, float h, float p) {
        temp_ = t;
        hum_ = h;
        press_ = p;
        has_env_ = true;
        set_dirty();
    }
    // Airtime use over the last hour: what share of it the radio heard traffic on the
    // channel, and what share of it we spent transmitting. Meshtastic reports both in
    // its device metrics, and the second one is what a duty-cycle limit applies to.
    void set_utilisation(float ch_util, float air_util);
    void set_uptime(uint32_t seconds);
    void refresh_nodes();
    bool on_encoder(const EncoderEvent delta) override;
    // Owner wires this to push the telemetry-config screen (it holds tel_cfg_).
    void set_on_telemetry(std::function<void()> cb) { on_telemetry_ = std::move(cb); }
    // "Resend Me" button: re-announce our NodeInfo now (same as Setup->Privacy).
    void set_on_resend(std::function<void()> cb) { on_resend_ = std::move(cb); }
    // "Send Pos": broadcast our position right now instead of waiting for the timer.
    void set_on_resend_pos(std::function<void()> cb) { on_resend_pos_ = std::move(cb); }

   private:
    static constexpr int SV_H = 280;
    // 24, like the chat and the node list. This row was 32 and stood out as taller than
    // the same row on every other tab; the eight pixels go to the list above it.
    static constexpr int BOT_H = 24;
    static constexpr int BOT_Y = SV_H - BOT_H;

    NavigationView& nav_;
    meshtastic::NodeDB& node_db_;

    float lat_{0}, lon_{0}, alt_{0};
    // First row of the "Nodes with GPS" list. It used to stop wherever the screen ran
    // out, so anything past the sixth node with a position was simply not there.
    // Scrolled with the encoder: the buttons own the D-pad on this tab.
    uint8_t gps_scroll_{0};
    uint8_t sats_{0};
    bool has_fix_{false};
    bool manual_{false};  // position came from the Setup tab, not the GPS

    uint8_t batt_pct_{0};
    bool batt_charging_{false};
    float temp_{0}, hum_{0}, press_{0};
    bool has_env_{false};
    std::string batt_text_{}, util_text_{}, up_text_{};  // last drawn, so identical rows are skipped

    std::function<void()> on_telemetry_{};
    std::function<void()> on_resend_{};

    std::function<void()> on_resend_pos_{};

    // The two readings that change every second live in their own widgets: repainting
    // the whole tab for them made the screen blink once a second.
    static constexpr int ROW_BATT = 64;
    static constexpr int ROW_UTIL = 80;
    static constexpr int ROW_UP = 96;
    Text text_batt_{{0, ROW_BATT, screen_width, 16}, ""};
    Text text_util_{{0, ROW_UTIL, screen_width, 16}, ""};
    // Our own uptime - the same figure we put in every telemetry packet, so there is
    // somewhere to read back what the mesh is being told about us.
    Text text_up_{{0, ROW_UP, screen_width, 16}, ""};

    Button button_open_map_{{0, BOT_Y, 57, BOT_H}, "Map"};
    Button button_telemetry_{{61, BOT_Y, 57, BOT_H}, "Telem"};
    Button button_resend_{{122, BOT_Y, 57, BOT_H}, "Info"};
    Button button_resend_pos_{{183, BOT_Y, 57, BOT_H}, "Pos"};
};

// ---- Setup pages -----------------------------------------------------------
// One small window per page, pushed from the Setup menu like any other screen.
// These were five sets of widgets sharing a single screen, built and torn down as the
// category selector moved - the only place in the firmware that created and destroyed
// widgets while its own window stayed open. It cost a use-after-free in the focus
// manager and, when the pages were flipped quickly, the process stack. Navigation
// frees a page for us, and only the page being looked at costs any memory - which was
// the point of building them on demand in the first place.

class MeshSettingsPage : public View {
   public:
    MeshSettingsPage(NavigationView& nav, MeshSettings& cfg, MeshSettingsHooks& hooks)
        : nav_{nav}, cfg_{cfg}, hooks_{hooks} {}

   protected:
    // The eight rows every page lays its controls out on. They start just under the
    // title bar: the category selector that used to occupy the first row is gone, so
    // nothing needs reserving above them any more.
    static constexpr int S0 = 10, S1 = 32, S2 = 54, S3 = 76,
                         S4 = 98, S5 = 120, S6 = 142, S7 = 164;

    // One apply for the whole visit, from the destructor: every encoder click used to
    // restart the receiver and re-announce the node, which is what made scrolling
    // through the options stutter. The hooks themselves only arm a timer in the app,
    // so calling them while this page is being torn down is safe.
    ~MeshSettingsPage() override {
        // Both, and in this order. A radio change is also a settings change: the preset
        // names the primary channel ("LongFast", "ShortFast", ...), and that name's hash
        // rides in every packet we send and gates every one we accept. The "else if"
        // that used to be here meant a preset changed at runtime retuned the radio but
        // left the hash on the previous preset - so the peer received our frames and
        // could not decrypt them, and we dropped all of its at the channel filter. One
        // silent bug, both directions, and only when the preset is changed without
        // restarting: at startup the hash is derived from whatever was persisted, which
        // is why switching presets appeared to work whenever it followed a flash.
        if (dirty_ && hooks_.apply) hooks_.apply();
        if (dirty_radio_ && hooks_.apply_radio) hooks_.apply_radio();
    }

    void changed() { dirty_ = true; }
    // ...and this one needs the receiver restarted: region, preset, frequency, slot.
    void changed_radio() { dirty_ = dirty_radio_ = true; }

    NavigationView& nav_;
    MeshSettings& cfg_;
    MeshSettingsHooks& hooks_;
    std::string entry_{};      // scratch for whatever the keyboard is editing
    bool dirty_{false};        // something changed and is waiting to be applied
    bool dirty_radio_{false};  // ...and it needs the receiver restarted
};

// Who we are on the mesh: name, id, role, and the board we claim to be.
class MeshProfilePageView : public MeshSettingsPage {
   public:
    MeshProfilePageView(NavigationView& nav, MeshSettings& cfg, MeshSettingsHooks& hooks);
    void focus() override { button_name_.focus(); }
    std::string title() const override { return "Profile"; }

   private:
    Labels labels_{
        {{0, S0}, "Name:", Theme::getInstance()->fg_light->foreground},
        {{0, S1}, "Short:", Theme::getInstance()->fg_light->foreground},
        {{0, S2}, "Node ID:", Theme::getInstance()->fg_light->foreground},
        {{0, S3}, "Role:", Theme::getInstance()->fg_light->foreground},
        {{0, S4}, "Device:", Theme::getInstance()->fg_light->foreground},
        {{0, S5}, "or number:", Theme::getInstance()->fg_light->foreground}};

    Button button_name_{{5 * 8, S0, 24 * 8, 16}, ""};
    Button button_short_{{6 * 8, S1, 8 * 8, 16}, ""};
    Button button_id_{{8 * 8, S2, 10 * 8, 16}, ""};
    // Tapping the id used to overwrite it with a random one, which is a trap on
    // the control that shows it: a mis-tap changed the address every peer knows
    // us by, with no way back. Tapping now edits it; rolling a new one is its own
    // button, and says so.
    Button button_rnd_{{19 * 8, S2, 5 * 8, 16}, "rnd"};
    OptionsField field_role_{
        {6 * 8, S3},
        13,
        {{"Client", 0}, {"Client Mute", 1}, {"Router", 2}, {"Router Client", 3}, {"Repeater", 4}, {"Tracker", 5}, {"Sensor", 6}, {"TAK", 7}, {"Client Hidden", 8}, {"Lost & Found", 9}, {"TAK Tracker", 10}, {"Router Late", 11}, {"Client Base", 12}}};
    OptionsField field_device_{{8 * 8, S4}, 11, {}};
    NumberField field_hw_num_{{11 * 8, S5}, 3, {0, 255}, 1, ' '};
    Button button_hw_type_{{15 * 8, S5, 6 * 8, 16}, "type"};
    // Our own contact code, the one a phone scans to add us - the node cards have had
    // this for peers all along, and there was no way to offer our own.
    Button button_share_{{0, S6 + 4, screen_width, 24}, "Share my contact QR"};

    // The infrastructure roles are not a "more powerful node" setting, which is how they
    // read to someone meeting them for the first time. A Router tells every other node to
    // prefer routing through it, and Router/Router Late never cancel a rebroadcast even
    // after hearing someone else make it (FloodingRouter::roleAllowsCancelingDupe). On a
    // node that is not high up with clear coverage that means duplicated traffic and a
    // routing preference pointing into a dead end - one badly set radio degrades the mesh
    // for everyone in range. Shown only for the roles that carry the risk, so it stays a
    // warning rather than wallpaper.
    Text text_warn0_{{0, S6 + 40, screen_width, 16}, ""};
    Text text_warn1_{{0, S6 + 56, screen_width, 16}, ""};
    Text text_warn2_{{0, S6 + 72, screen_width, 16}, ""};
    void show_role_warning();
};

// Where and how we transmit.
class MeshRadioPageView : public MeshSettingsPage {
   public:
    MeshRadioPageView(NavigationView& nav, MeshSettings& cfg, MeshSettingsHooks& hooks);
    void focus() override { field_region_.focus(); }
    std::string title() const override { return "Radio"; }

   private:
    Labels labels_{
        {{0, S0}, "Region:", Theme::getInstance()->fg_light->foreground},
        {{0, S1}, "Preset:", Theme::getInstance()->fg_light->foreground},
        {{0, S2}, "Hop limit:", Theme::getInstance()->fg_light->foreground},
        {{0, S3}, "Freq:", Theme::getInstance()->fg_light->foreground},
        {{0, S4}, "Freq slot:", Theme::getInstance()->fg_light->foreground},
        {{0, S5}, "NodeInfo min:", Theme::getInstance()->fg_light->foreground},
        {{18 * 8, S2}, "CR:", Theme::getInstance()->fg_light->foreground},
        {{0, S7 + 22}, "TX pwr:", Theme::getInstance()->fg_light->foreground}};

    OptionsField field_region_{{7 * 8, S0}, 14, {}};
    OptionsField field_preset_{{7 * 8, S1}, 14, {}};
    NumberField field_hops_{{11 * 8, S2}, 1, {1, 7}, 1, ' '};
    OptionsField field_cr_{
        {21 * 8, S2},
        6,
        {{"preset", 0}, {"4/5", 5}, {"4/6", 6}, {"4/7", 7}, {"4/8", 8}}};
    Button button_freq_{{5 * 8, S3, 15 * 8, 16}, ""};
    NumberField field_slot_{{11 * 8, S4}, 2, {0, 60}, 1, ' '};
    NumberField field_nodeinfo_{{14 * 8, S5}, 3, {0, 240}, 5, ' '};
    Checkbox check_mqtt_{{0, S6}, 4, "OK to MQTT", true};
    Checkbox check_ignore_mqtt_{{0, S7}, 4, "Ignore MQTT pkts", true};
    // Whose choice the transmit power is - see MeshSettings::tx_pwr_mode for why this
    // informs rather than enforces.
    OptionsField field_txpwr_{{8 * 8, S7 + 22}, 6, {{"Region", 0}, {"Max", 1}, {"Custom", 2}}};
    NumberField field_txdb_{{15 * 8, S7 + 22}, 2, {0, 47}, 1, ' '};
    // How far to pull a telescopic whip out for the frequency actually in use. The
    // number people want in the field is the quarter wave, in centimetres.
    Text text_whip_{{0, S7 + 44, screen_width, 16}, ""};

    void update_whip();
};

// Receipts, keys, and how much of our identity is real.
class MeshPrivacyPageView : public MeshSettingsPage {
   public:
    MeshPrivacyPageView(NavigationView& nav, MeshSettings& cfg, MeshSettingsHooks& hooks);
    void focus() override { check_receipts_.focus(); }
    std::string title() const override { return "Privacy"; }

   private:
    static constexpr size_t NBITS = 6;
    // Which field each tick box randomises, in the order they are drawn.
    static const uint8_t RAND_BIT[NBITS];

    Labels labels_{
        {{0, S2}, "Randomize:", Theme::getInstance()->fg_light->foreground}};

    Checkbox check_receipts_{{0, S0}, 4, "Send read receipts", true};
    Checkbox check_pki_{{0, S1}, 4, "Encrypt DMs (PKC)", true};
    // Was "Listen only, no beacons", which promised a silence it does not deliver: it
    // stops what this node says about itself, not what it repeats for others. Relaying
    // is the role's business (Client Mute) and total silence is the RX/TX lock in the
    // chat's status row. Reported as a bug because a peer still showed messages
    // delivered - it was hearing its own packet relayed back, which Meshtastic counts
    // as an implicit acknowledgement.
    Checkbox check_listen_{{0, S6 + 26}, 4, "No beacons or replies", true};
    // Meshtastic answers a LocalStats request from anyone; this refuses. Off also means
    // no device metrics go out on request, since both travel the same reply.
    Checkbox check_stats_{{0, S6 + 48}, 4, "Answer stats requests", true};
    Button button_rand_{{11 * 8, S2, 9 * 8, 16}, ""};
    // Named rather than an array: ui::Checkbox has no move constructor, so an array
    // member cannot be brace-initialised element by element.
    Checkbox check_r_id_{{0, S3}, 4, "node id", true};
    Checkbox check_r_name_{{15 * 8, S3}, 4, "name", true};
    Checkbox check_r_role_{{0, S4}, 4, "role", true};
    Checkbox check_r_dev_{{15 * 8, S4}, 4, "device", true};
    Checkbox check_r_pos_{{0, S5}, 4, "position", true};
    Checkbox check_r_tel_{{15 * 8, S5}, 4, "telemetry", true};
    Button button_resend_{{0, S6, screen_width, 24}, "Announce NodeInfo now"};
};

// Logging, store and forward, key repeat, the echo range test.
class MeshSystemPageView : public MeshSettingsPage {
   public:
    MeshSystemPageView(NavigationView& nav, MeshSettings& cfg, MeshSettingsHooks& hooks);
    void focus() override { check_log_.focus(); }
    std::string title() const override { return "System"; }

   private:
    static constexpr size_t NBITS = 5;
    // What the echo reply carries, in the order the boxes are drawn.
    static const uint8_t ECHO_BIT[NBITS];
    // What the echo answers, read back from the settings it is made of. The separate
    // "Echo test replies" and "bell" tick boxes are gone: one field says the whole
    // thing, and "off" is simply one of its positions.
    enum EchoMode : uint8_t { ECHO_OFF = 0,
                              ECHO_ANY = 1,
                              ECHO_ONLY_BELL = 2,
                              ECHO_CUSTOM = 3 };
    uint8_t echo_mode() const;
    void apply_echo_mode(uint8_t mode);

    Labels labels_{
        {{0, S2}, "  buffer:", Theme::getInstance()->fg_light->foreground},
        {{12 * 8, S2}, "hold:", Theme::getInstance()->fg_light->foreground},
        {{0, S7}, "Neighbors min:", Theme::getInstance()->fg_light->foreground}};

    Checkbox check_log_{{0, S0}, 4, "Log to SD card", true};
    Checkbox check_sf_{{0, S1}, 4, "Store & Forward", true};
    NumberField field_sf_max_{{9 * 8, S2}, 2, {1, 30}, 1, ' '};
    NumberField field_sf_ttl_{{17 * 8, S2}, 3, {0, 720}, 15, ' '};
    Checkbox check_key_repeat_{{0, S3}, 4, "Hold key repeats", true};
    // Hard against the left edge, and it says "echo" itself instead of leaning on a
    // label. A tick box is only 48 pixels wide however long its text reads, so a field
    // starting where that ends shares no column with the rows above and below it - and
    // the D-pad, which picks the next widget by geometry, stepped straight over it.
    OptionsField field_echo_{
        {0, S4},
        10,
        {{"echo off", ECHO_OFF}, {"echo any", ECHO_ANY}, {"echo bell", ECHO_ONLY_BELL}, {"echo text:", ECHO_CUSTOM}}};
    // Only on show while the field says "echo text:": it is where that text is typed.
    Button button_echo_key_{{11 * 8, S4, 19 * 8, 16}, ""};
    // Named rather than an array: see MeshPrivacyPageView.
    Checkbox check_e_text_{{0, S5}, 4, "text", true};
    Checkbox check_e_sig_{{12 * 8, S5}, 4, "signal", true};
    Checkbox check_e_mem_{{0, S6}, 4, "mem", true};
    Checkbox check_e_up_{{9 * 8, S6}, 4, "uptime", true};
    Checkbox check_e_build_{{20 * 8, S6}, 4, "build", true};
    NumberField field_nbr_{{14 * 8, S7}, 4, {0, 1440}, 60, ' '};
};

// How the conversation reads and how much of it is kept.
class MeshChatPageView : public MeshSettingsPage {
   public:
    MeshChatPageView(NavigationView& nav, MeshSettings& cfg, MeshSettingsHooks& hooks);
    void focus() override { field_time_.focus(); }
    std::string title() const override { return "Chat"; }
    // The "Aa" sample beside the font selector, in the very face the chat will use:
    // the choice is shown rather than described.
    void paint(Painter& painter) override;

   private:
    Labels labels_{
        {{0, S0}, "Time:", Theme::getInstance()->fg_light->foreground},
        {{0, S1}, "Names:", Theme::getInstance()->fg_light->foreground},
        {{0, S5}, "Save msgs:", Theme::getInstance()->fg_light->foreground},
        {{14 * 8, S5}, "Resend:", Theme::getInstance()->fg_light->foreground},
        {{22 * 8, S5}, "x", Theme::getInstance()->fg_light->foreground},
        {{25 * 8, S5}, "s", Theme::getInstance()->fg_light->foreground},
        {{0, S6}, "Font:", Theme::getInstance()->fg_light->foreground},
        {{16 * 8, S4}, "Lines:", Theme::getInstance()->fg_light->foreground}};

    OptionsField field_time_{
        {6 * 8, S0},
        9,
        {{"none", 0}, {"HH:MM", 1}, {"HH:MM:SS", 2}}};
    OptionsField field_names_{
        {7 * 8, S1},
        8,
        {{"long", 0}, {"short", 1}, {"node id", 2}, {"colour", 3}}};
    Checkbox check_db_{{0, S2}, 4, "Signal of last pkt", true};
    // "Status lines" is gone: the chat's own All / Msg / Tec button already decides
    // whether the app's "* ..." notices are shown, and two controls for one thing only
    // disagree with each other.
    Checkbox check_beep_{{0, S3}, 4, "Beep on new msg", true};
    // Far enough right to clear the label beside it - at sixteen columns the tick box
    // landed on the "g" of "msg".
    Checkbox check_bell_{{18 * 8, S3}, 4, "Bell", true};
    Checkbox check_sf_notify_{{0, S4}, 4, "Notify on S&F", true};
    NumberField field_lines_{{23 * 8, S4}, 2, {10, 40}, 2, ' '};
    NumberField field_save_{{10 * 8, S5}, 3, {0, 200}, 10, ' '};
    NumberField field_retries_{{21 * 8, S5}, 1, {0, 9}, 1, ' '};
    NumberField field_retry_s_{{23 * 8, S5}, 2, {3, 60}, 1, ' '};
    // Along the bottom, clear of everything: it used to sit beside the font selector
    // and cover the sample the selector exists to show.
    Button button_clear_{{0, S6 + 26, screen_width, 24}, "Clear history"};
    OptionsField field_font_{{6 * 8, S6}, 6, {{"small", 8}, {"normal", 16}}};
    // Reads "SD glyphs" rather than naming an alphabet: the file decides which one.
    // Not on the Font row, however much it belongs there by meaning: paint() draws the
    // "Aa" sample across x 104..200 of that row and repainting it wiped the box out.
    Checkbox check_glyphs_{{16 * 8, S1}, 9, "SD glyphs", true};
};

// ---- Tab: Setup (index 3) --------------------------------------------------
// Only a menu; each entry pushes the page above. The tab itself is alive for the whole
// session, and the device has ~27 KB of core memory for everything, so the settings
// themselves cost nothing while you are chatting.

class MeshtasticSetupMenuView : public View {
   public:
    // Out of line: see MeshtasticChatView's constructor.
    __attribute__((noinline)) explicit MeshtasticSetupMenuView(Rect parent_rect);
    void focus() override;
    void paint(Painter& painter) override;
    // Owner pushes the page the tapped entry names.
    void set_on_open(std::function<void(uint8_t)> cb) { on_open_ = std::move(cb); }

   private:
    std::function<void(uint8_t)> on_open_{};
    static constexpr int BH = 30;
    Button button_profile_{{0, 30, screen_width, BH}, "Profile"};
    Button button_radio_{{0, 66, screen_width, BH}, "Radio"};
    Button button_privacy_{{0, 102, screen_width, BH}, "Privacy"};
    Button button_system_{{0, 138, screen_width, BH}, "System"};
    Button button_chat_{{0, 174, screen_width, BH}, "Chat"};

    // Said once, here, and nowhere else in the app: this speaks the protocol, it is not
    // the project. The name belongs to them and the app must not imply otherwise.
    //
    // Sat at 250/266/282 and only the first line was on screen - the two that actually
    // disclaim anything fell off the bottom. A disclaimer nobody can read is not one.
    Labels disclaimer_{
        {{0, 214}, "Uses the Meshtastic protocol.", Theme::getInstance()->fg_dark->foreground},
        {{0, 230}, "Not affiliated with Meshtastic", Theme::getInstance()->fg_dark->foreground},
        {{0, 246}, "LLC, and not endorsed by them.", Theme::getInstance()->fg_dark->foreground}};
};

// ---- Telemetry pages -------------------------------------------------------
// What we broadcast about ourselves, a group at a time. Same story as the Setup pages
// above: one window per group, freed by navigation, instead of one screen rebuilding
// its widgets whenever the group selector moved.

class MeshTelemetryPage : public MeshSettingsPage {
   public:
    MeshTelemetryPage(NavigationView& nav, MeshSettings& cfg, MeshSettingsHooks& hooks)
        : MeshSettingsPage(nav, cfg, hooks) {}

   protected:
    static constexpr int LX = 0;      // label x
    static constexpr int FX = 9 * 8;  // field x
    // As on the settings pages: the group selector is gone, so the rows start at the
    // top - at the same 10 those pages use (S0), not the 16 left over from when there
    // was a row above them.
    static constexpr int G0 = 10, G1 = 28, G2 = 46, G3 = 64, G4 = 82;
};

// The menu the Data tab's "Telem" button opens. It carries nav/cfg/hooks itself rather
// than capturing its constructor's parameters: those are references that die with the
// constructor, and the buttons are pressed long afterwards.
class MeshTelemetryMenuView : public MeshSettingsPage {
   public:
    MeshTelemetryMenuView(NavigationView& nav, MeshSettings& cfg, MeshSettingsHooks& hooks);
    void focus() override { button_device_.focus(); }
    std::string title() const override { return "Telemetry"; }

   private:
    // Straight under the title bar. These used to start at y=30, which was the right
    // number back when this was laid out like the Setup TAB - that one keeps a heading
    // above its buttons. A pushed page has the navigation bar instead, so the 30 was
    // simply an empty band.
    static constexpr int BH = 30;
    Button button_device_{{0, 4, screen_width, BH}, "Device"};
    Button button_position_{{0, 40, screen_width, BH}, "Position"};
    Button button_sensors_{{0, 76, screen_width, BH}, "Sensors"};
    Button button_timing_{{0, 112, screen_width, BH}, "Timing"};
};

// What the node says about its own power and airtime.
class MeshTelemetryDevicePageView : public MeshTelemetryPage {
   public:
    MeshTelemetryDevicePageView(NavigationView& nav, MeshSettings& cfg, MeshSettingsHooks& hooks);
    void focus() override { field_batt_.focus(); }
    std::string title() const override { return "Device"; }

   private:
    Labels labels_{
        {{LX, G0}, "Battery:", Theme::getInstance()->fg_light->foreground},
        {{LX, G1}, "  Pct %:", Theme::getInstance()->fg_light->foreground},
        {{LX, G2}, "Voltage:", Theme::getInstance()->fg_light->foreground},
        {{LX, G3}, " 0.1V:", Theme::getInstance()->fg_light->foreground},
        {{LX, G4}, "Airtime:", Theme::getInstance()->fg_light->foreground},
        {{LX, G4 + 18}, " Ch/Air:", Theme::getInstance()->fg_light->foreground},
        {{LX, G4 + 36}, "Uptime:", Theme::getInstance()->fg_light->foreground},
        {{LX, G4 + 54}, "  Hours:", Theme::getInstance()->fg_light->foreground}};

    OptionsField field_batt_{
        {FX, G0},
        8,
        {{"Real", 0}, {"Custom", 1}, {"Charging", 2}, {"Off", 3}}};
    NumberField field_pct_{{FX, G1}, 3, {0, 100}, 1, ' '};
    OptionsField field_volt_{{FX, G2}, 8, {{"Real", 0}, {"Custom", 1}, {"Off", 2}}};
    NumberField field_dv_{{FX, G3}, 2, {25, 45}, 1, ' '};
    OptionsField field_util_{{FX, G4}, 8, {{"Real", 0}, {"Custom", 1}, {"Off", 2}}};
    NumberField field_chutil_{{FX, G4 + 18}, 3, {0, 100}, 1, ' '};
    NumberField field_airutil_{{FX + 5 * 8, G4 + 18}, 3, {0, 100}, 1, ' '};
    OptionsField field_up_{{FX, G4 + 36}, 8, {{"Real", 0}, {"Custom", 1}, {"Off", 2}}};
    NumberField field_uphours_{{FX, G4 + 54}, 4, {0, 8760}, 1, ' '};
};

// The fix we claim when there is no live GPS.
class MeshTelemetryPositionPageView : public MeshTelemetryPage {
   public:
    MeshTelemetryPositionPageView(NavigationView& nav, MeshSettings& cfg, MeshSettingsHooks& hooks);
    void focus() override { check_gps_.focus(); }
    std::string title() const override { return "Position"; }

   private:
    Labels labels_{
        {{LX, G1}, "Lat:", Theme::getInstance()->fg_light->foreground},
        {{LX, G2}, "Lon:", Theme::getInstance()->fg_light->foreground},
        {{LX, G3}, "Alt m:", Theme::getInstance()->fg_light->foreground},
        {{LX + 21 * 8, G3}, "Sats:", Theme::getInstance()->fg_light->foreground}};

    Checkbox check_gps_{{LX, G0}, 4, "Live GPS (USB)", true};
    Button button_lat_{{4 * 8, G1, 26 * 8, 16}, "Set Latitude"};
    Button button_lon_{{4 * 8, G2, 26 * 8, 16}, "Set Longitude"};
    NumberField field_alt_{{FX, G3}, 5, {-1000, 20000}, 1, ' '};
    // The "type" button used to start at the same pixel as the "Sats:" label and drew
    // straight over it, so the satellite count showed up as a bare 0 with nothing to
    // say what it was. Row now reads: Alt m: [field] [type]  Sats: [field].
    Button button_alt_{{FX + 5 * 8, G3, 6 * 8, 16}, "type"};
    NumberField field_sats_{{LX + 26 * 8, G3}, 2, {0, 32}, 1, ' '};
    Button button_map_{{LX, G4, screen_width, 16}, "Pick location on map >"};
};

// The whole of telemetry.proto, a family at a time.
class MeshTelemetrySensorsPageView : public MeshTelemetryPage {
   public:
    MeshTelemetrySensorsPageView(NavigationView& nav, MeshSettings& cfg, MeshSettingsHooks& hooks);
    void focus() override { field_family_.focus(); }
    std::string title() const override { return "Sensors"; }

   private:
    static constexpr size_t XROWS = 6;  // rows in the sensor editor (power needs six)
    // Point the six rows at another family of metrics. Never touches the selector
    // itself - see the note where on_change is wired.
    void show_family(size_t group);
    size_t family_{0};
    size_t row_idx_[XROWS]{};

    Labels labels_{{{LX, G0}, "Group:", Theme::getInstance()->fg_light->foreground}};
    OptionsField field_family_{{FX - 16, G0}, 14, {}};

    // One set of widgets per row, named rather than an array: ui::Checkbox and friends
    // have no move constructor, so an array member cannot be brace-initialised.
    Checkbox row_check_0_{{LX, G1 + 0 * 18}, 11, "", true};
    NumberField row_value_0_{{16 * 8, G1 + 0 * 18}, 5, {0, 65535}, 1, ' '};
    Text row_unit_0_{{22 * 8, G1 + 0 * 18, 8 * 8, 16}, ""};
    Checkbox row_check_1_{{LX, G1 + 1 * 18}, 11, "", true};
    NumberField row_value_1_{{16 * 8, G1 + 1 * 18}, 5, {0, 65535}, 1, ' '};
    Text row_unit_1_{{22 * 8, G1 + 1 * 18, 8 * 8, 16}, ""};
    Checkbox row_check_2_{{LX, G1 + 2 * 18}, 11, "", true};
    NumberField row_value_2_{{16 * 8, G1 + 2 * 18}, 5, {0, 65535}, 1, ' '};
    Text row_unit_2_{{22 * 8, G1 + 2 * 18, 8 * 8, 16}, ""};
    Checkbox row_check_3_{{LX, G1 + 3 * 18}, 11, "", true};
    NumberField row_value_3_{{16 * 8, G1 + 3 * 18}, 5, {0, 65535}, 1, ' '};
    Text row_unit_3_{{22 * 8, G1 + 3 * 18, 8 * 8, 16}, ""};
    Checkbox row_check_4_{{LX, G1 + 4 * 18}, 11, "", true};
    NumberField row_value_4_{{16 * 8, G1 + 4 * 18}, 5, {0, 65535}, 1, ' '};
    Text row_unit_4_{{22 * 8, G1 + 4 * 18, 8 * 8, 16}, ""};
    Checkbox row_check_5_{{LX, G1 + 5 * 18}, 11, "", true};
    NumberField row_value_5_{{16 * 8, G1 + 5 * 18}, 5, {0, 65535}, 1, ' '};
    Text row_unit_5_{{22 * 8, G1 + 5 * 18, 8 * 8, 16}, ""};

    // ...and the same rows again as pointers, which is how show_family walks them.
    Checkbox* row_check_[XROWS]{};
    NumberField* row_value_[XROWS]{};
    Text* row_unit_[XROWS]{};

    Button button_all_on_{{LX, G1 + 112, 76, 24}, "All on"};
    Button button_random_{{LX + 80, G1 + 112, 76, 24}, "Random"};
    Button button_all_off_{{LX + 160, G1 + 112, 76, 24}, "All off"};
    // The Data tab has "Info" for who we are and "Pos" for where we are; this is the
    // third of the trio, and it belongs with the values it sends.
    Button button_send_{{LX, G1 + 140, screen_width, 24}, "Send telemetry now"};
};

// How often we speak unprompted.
class MeshTelemetryTimingPageView : public MeshTelemetryPage {
   public:
    MeshTelemetryTimingPageView(NavigationView& nav, MeshSettings& cfg, MeshSettingsHooks& hooks);
    void focus() override { field_pos_min_.focus(); }
    std::string title() const override { return "Timing"; }

   private:
    Labels labels_{
        {{LX, G0}, "Minutes, 0 = off", Theme::getInstance()->fg_light->foreground},
        {{LX, G1}, "Position:", Theme::getInstance()->fg_light->foreground},
        {{LX, G2}, "Telemetry:", Theme::getInstance()->fg_light->foreground}};

    NumberField field_pos_min_{{FX + 8, G1}, 3, {0, 120}, 5, ' '};
    NumberField field_tel_min_{{FX + 16, G2}, 3, {0, 120}, 5, ' '};
};

class MeshtasticChannelsView : public View {
   public:
    // open_dm = the node whose private thread the chat is showing, or 0 when the chat
    // is on a channel. The list marks whichever row is actually open, not just the
    // active channel: with a private thread up, the marker used to sit on the public
    // channel and the QR button offered that channel instead of the thread.
    MeshtasticChannelsView(NavigationView& nav, uint8_t preset_idx, uint8_t region_idx, uint8_t freq_slot, uint8_t cr, std::string* names, std::string* keys, bool* enabled, uint8_t* active, std::string* primary_name, meshtastic::NodeDB& node_db, uint32_t open_dm, std::function<void()> on_change, std::function<void(uint32_t)> on_dm, std::function<std::string()> make_key);
    void focus() override;
    void paint(Painter& painter) override;
    bool on_encoder(const EncoderEvent delta) override;
    bool on_key(const KeyEvent key) override;
    bool on_touch(const TouchEvent event) override;
    std::string title() const override { return "Chats"; }
    // "Bell": ring whatever the highlighted row is - a channel or a private thread.
    // Meshtastic clients alert on a message containing the bell character, which is how
    // you get someone's attention without writing anything. Channel 0xFF = the node id.
    void set_on_bell(std::function<void(uint8_t, uint32_t)> cb) { on_bell_ = std::move(cb); }

   private:
    NavigationView& nav_;
    uint8_t preset_idx_;
    uint8_t region_idx_;
    uint8_t freq_slot_;  // Setup -> Radio override, 0 = derived from the preset
    uint8_t cr_;         // the coding rate actually in use, for the shared URL
    std::string* names_;
    std::string* keys_;
    bool* enabled_;
    uint8_t* active_;
    meshtastic::NodeDB& node_db_;
    uint32_t open_dm_{0};  // 0 = the chat is on a channel, not a thread
    std::function<void()> on_change_;
    std::function<void(uint32_t)> on_dm_;
    std::function<void(uint8_t, uint32_t)> on_bell_{};
    std::function<std::string()> make_key_;  // fresh random channel key (32 hex chars)
    // QRCodeView keeps the pointer it is given, so the string has to outlive it.
    std::string qr_url_{};

    void use_primary();
    void edit_primary_name();  // tap the already-active primary row -> rename it
    std::string primary_label() const;
    void select_or_edit(int i);  // tap: switch to a configured slot, else edit
    void edit_custom(int i);     // i = 0..NUM_CUSTOM-1 (custom slots -> channels 1..)
    void delete_channel();       // clear the targeted custom channel
    void randomize_key();        // give the targeted channel a fresh random key
    void show_qr();              // share the targeted channel as a Meshtastic QR
    void send_bell();            // ring the highlighted channel or thread
    // Which custom slot the action rows work on: the highlighted one when it is a
    // custom channel, otherwise the active one. -1 = none (primary).
    int target_slot() const;
    void open_row(size_t row);  // act on a list row (channel / action / thread)
    void move_selection(int delta);
    // Rows in the order a conversation list should read: the public channel, then the
    // channels that are actually set up, then the private threads, and only then the
    // empty slots. A new thread used to land behind five empty rows.
    struct Row {
        enum Kind : uint8_t { Primary,
                              Channel,
                              Dm,
                              Empty } kind{Primary};
        int slot{0};       // custom-channel index for Channel and Empty
        uint32_t node{0};  // node id for Dm
    };
    Row row_at(size_t row) const;
    // Which row is the conversation the chat currently has open.
    size_t row_of_open() const;
    size_t dm_count() const;
    uint32_t dm_at(size_t n) const;
    size_t row_count() const { return NUM_CUSTOM + 1 + dm_count(); }

    std::string* primary_name_{nullptr};
    static constexpr int NUM_CUSTOM = MeshSettings::NUM_CUSTOM;
    static constexpr int ROW_H = 24;
    static constexpr int LIST_H = 240;
    static constexpr int VISIBLE = LIST_H / ROW_H;  // 10 rows
    // Hard against the bottom of the screen: the old gap above the buttons was 22
    // pixels of nothing between the hint line and them.
    static constexpr int BTN_Y = 304 - 26;

    size_t selected_{0};
    size_t scroll_offset_{0};

    // Actions on the highlighted row. Buttons rather than list rows: pick the channel
    // with the D-pad, then tap the action - no walking back through the list.
    Button button_del_{{0, BTN_Y, 58, 26}, "Delete"};
    Button button_rnd_{{61, BTN_Y, 58, 26}, "Rnd key"};
    Button button_qr_{{122, BTN_Y, 58, 26}, "QR"};
    Button button_bell_{{183, BTN_Y, 57, 26}, "Bell"};
};

// ============================================================================
// Main Meshtastic app view
// ============================================================================

class MeshtasticView : public View {
   public:
    explicit MeshtasticView(NavigationView& nav);
    // Passphrase (or 32 hex characters, taken as a raw AES-128 key) -> channel key.
    // Public because the conversations view builds a channel QR code from it.
    static void derive_channel_key(const std::string& text, uint8_t out[16]);
    ~MeshtasticView();

    void focus() override;
    std::string title() const override { return "Mesh"; }
    void set_parent_rect(const Rect new_parent_rect) override;
    void paint(Painter&) override {}

   private:
    void on_lora_packet(const LoRaPacketMessage* msg);
    // M4 reports that a packet is being demodulated (or has finished): hold TX.
    void on_lora_rx_status(const LoRaRxStatusMessage* msg);
    void on_gps_update(const GPSPosDataMessage* msg);
    void on_environment_update(const EnvironmentDataMessage* msg);
    void read_battery();  // read the fuel gauge -> telemetry_ + Map
    void apply_telemetry_overrides(meshtastic::TelemetryData& t) const;
    void broadcast_telemetry();
    void apply_channels();  // (re)push the channel table (hashes + keys) to the router
    void persist();         // save node/channel/telemetry settings to SD right now
    // 32 hex characters of RF-seeded randomness: a fresh private channel key.
    std::string random_channel_key();
    // What the channel in use is called, for the notices that say where a packet went.
    std::string active_channel_name() const;
    void init_pki();            // load a saved key (or generate one if PKC is on); push it to the router
    void generate_pki_key();    // derive a fresh Curve25519 private key from accumulated entropy
    void randomize_identity();  // pick a fresh random node id / name / hw-model + re-announce
    uint32_t rand_next();       // xorshift32 PRNG step

    void on_timer();
    // Console lines built out of line, so their string temporaries do not sit in the
    // receive handler's frame. See the note on the definitions.
    void announce_node(const meshtastic::NodeEntry& e, uint32_t from, bool named);
    void announce_relay(uint32_t from, const uint8_t* relay);
    // Everything the constructor used to do that reaches the SD card or the baseband:
    // keys, channel table, log file, receiver. Run once, from the first timer tick,
    // where the process stack is empty. See the note in the constructor.
    // The coding rate actually in use: the Radio override, or the preset's own.
    uint8_t active_tx_gain() const;
    uint8_t active_coding_rate() const;
    void start_deferred();
    void start_rx();
    void trigger_tx();
    void on_tx_done(uint8_t nsym = 0);
    // from_user = composed by the user (chat): such a packet is never dropped to make
    // room for a relay.
    // Returns false when the packet could not be accepted (queue full, relay).
    bool queue_tx(const uint8_t* data, size_t len, bool from_user = false);
    void broadcast_nodeinfo();
    void broadcast_position();
    // Say in the chat that our coordinates went on the air, and which kind they were.
    void announce_position();
    // Ring a channel or a node: channel 0xFF means the node id is the destination.
    void send_bell(uint8_t channel, uint32_t node);
    // Push our own position (fixed one when GPS is off) to the Map tab.
    void refresh_own_position();
    // Open the map in point-pick mode and take the confirmed point as our position.
    void pick_position_on_map();
    void update_radio_config();

    NavigationView& nav_;

    meshtastic::NodeDB node_db_{};
    meshtastic::MeshRouter router_{node_db_};

    // Everything that is remembered between runs, and everything the settings pages
    // edit. Runtime state stays below.
    MeshSettings cfg_{};
    // Wired in the constructor and handed to every settings page by reference.
    MeshSettingsHooks hooks_{};
    std::string share_url_{};  // kept alive while the QR view shows it

    uint8_t channel_hash_{meshtastic::DEFAULT_CHANNEL_HASH};
    SwitchesState saved_repeat_{};
    uint32_t rand_timer_{0};           // ticks since the last identity randomisation
    uint32_t rand_state_{0x12345678};  // xorshift PRNG state for identity randomisation
    uint8_t pki_priv_[32]{};           // decoded private key (working copy)
    uint8_t pki_pub_[32]{};            // derived public key
    // Entropy pool for key generation. Stirred with the RSSI (thermal RF noise = real
    // entropy) and arrival timing of every received packet, so the private key is not
    // derived from a cold-boot clock alone. See generate_pki_key().
    uint32_t entropy_pool_{0};

    meshtastic::PositionData gps_position_{};
    meshtastic::TelemetryData telemetry_{};  // latest device + environment metrics
    bool has_gps_fix_{false};
    uint32_t uptime_ticks_{0};
    // DisplayFrameSync ticks ~60 Hz -> ~3600 ticks per minute.
    static constexpr uint16_t POSITION_PERIOD = 18000;
    static constexpr uint16_t TELEMETRY_PERIOD = 18000;
    // NodeInfo interval is user-configurable (Setup->Radio, minutes). uint32 so it can
    // exceed a uint16's ~18 min ceiling; clamp to >=1 min if the setting is 0.
    uint32_t nodeinfo_period() const {
        return static_cast<uint32_t>(cfg_.nodeinfo_min ? cfg_.nodeinfo_min : 15) * 3600u;
    }
    uint32_t nodeinfo_timer_{0};
    uint32_t nbr_timer_{0};  // ticks until the next NeighborInfo broadcast
    void broadcast_neighborinfo();
    // uint32 so configurable intervals can exceed a uint16's ~18 min ceiling.
    uint32_t position_timer_{0};
    uint32_t telemetry_timer_{0};
    uint8_t telemetry_turn_{0};  // rotates device -> environment -> air quality

    uint8_t tx_buf_[meshtastic::PKT_MAX_SIZE]{};
    size_t tx_len_{0};
    bool tx_pending_{false};
    bool is_transmitting_{false};
    // When the transmission started. The baseband reports completion with a TXProgress
    // message, and if that message is ever missed the app would sit in transmit mode
    // for ever: receiver down, nothing sent, radio dead until the app is restarted.
    // The watchdog below puts it back to listening.
    uint32_t tx_start_tick_{0};
    // After returning from transmit the demodulator sometimes stayed silent: the
    // receiver was tuned correctly and the baseband image was running, but no packet
    // ever arrived again. Re-sending its configuration a moment after the image starts
    // revives it, and costs nothing when it was fine - configure() just re-initialises
    // the demodulator state.
    uint16_t reconfig_delay_{0};
    void resend_lora_config();
    // One direct message is repeated at a time - the most recent one still waiting for
    // a receipt. Meshtastic repeats the SAME packet id, so the far side recognises the
    // repeat as a duplicate and its acknowledgement still matches our message.
    uint8_t retry_buf_[meshtastic::PKT_MAX_SIZE]{};
    size_t retry_len_{0};
    uint32_t retry_pid_{0};
    uint8_t retry_left_{0};
    uint32_t retry_at_{0};
    void arm_retry(const uint8_t* data, size_t len);
    static constexpr uint32_t TX_TIMEOUT_TICKS = 900;  // far longer than any frame
    uint16_t tx_delay_{0};
    // Packets waiting for the radio's single TX slot (relays, telemetry, chat).
    static constexpr size_t TX_QUEUE_MAX = 4;
    std::vector<std::vector<uint8_t>> tx_queue_{};
    // Listen before talk: the M4 reports when it is inside a packet, and TX waits.
    // The cap (~3 s at ~60 Hz) exceeds the longest LongFast frame, so a missed
    // Airtime accounting for the two utilisation figures. A rolling hour in twelve
    // five-minute buckets, ticked from the 60 Hz timer: one counts the ticks the
    // receiver reported a packet in the air, the other the ticks we were transmitting.
    static constexpr uint8_t UTIL_BUCKETS = 12;
    static constexpr uint32_t UTIL_BUCKET_TK = 5 * 60 * 60;  // 5 min at ~60 Hz
    uint16_t util_rx_[UTIL_BUCKETS]{};
    uint16_t util_tx_[UTIL_BUCKETS]{};
    uint8_t util_slot_{0};
    uint32_t util_slot_ticks_{0};
    uint32_t util_span_ticks_{0};  // how much of the window has actually elapsed
    void update_utilisation();

    // Direct-message threads outlive a restart: the conversation list is rebuilt from
    // this file, public keys included, so a private chat does not have to be
    // re-established (and re-keyed) every time the app is opened.
    void apply_key_repeat();
    void save_known_nodes();
    void load_known_nodes();
    // The extra-metrics table as one settings string: five hex characters per field
    // (one for the send flag, four for the value). The settings store binds scalars and
    // strings, and a hundred separate keys for one editor would be absurd.
    void stop_retry(uint32_t pid);  // an acknowledged message stops being repeated
    void pack_extras();
    void unpack_extras();
    uint32_t threads_sig_{0};  // change detector, so the file is written rarely
    // Last good fuel-gauge reading: it does not report every field on every read.
    uint8_t batt_pct_{0};
    uint16_t batt_mv_{0};
    bool batt_charging_{false};
    bool batt_have_current_{false};

    // "reception ended" message delays one transmission instead of blocking TX.
    bool deferred_load_{false};  // keys, channels, radio, font + chat history: first tick
    bool rx_busy_{false};
    uint32_t rx_busy_tick_{0};
    // Settings are applied a moment after the last change, not on every encoder click:
    // scrolling through a list used to restart the receiver and write the SD card at
    // each step, which is what made the menus crawl.
    uint16_t apply_delay_{0};
    // Replies are built from the timer, not from the packet callback: that callback
    // already runs several hundred bytes deep (relay buffer, decrypt buffers, AES
    // state), and assembling another packet there overflowed the 4 KB main stack.
    uint32_t echo_dest_{0};
    // Peer we still owe a key request: a node that publishes a public key refuses
    // direct messages that carry only channel encryption, so a unicast to a peer
    // whose key we lack asks for its NodeInfo before anything else is attempted.
    uint32_t keyreq_dest_{0};
    // Who asked us for telemetry; the reply is assembled from the timer, not from the
    // packet callback where the stack is deep and an i2c read is out of place.
    // A traceroute request waiting to be answered from on_timer, held exactly like the
    // metrics one below: assembling and encrypting a packet inside the RX callback is
    // far more stack than that callback has.
    uint32_t bad_hash_seen_{0};  // last reported channel-filter rejection count
    uint32_t trace_dest_{0};
    uint32_t trace_req_id_{0};
    float trace_snr_{0.0f};
    uint8_t trace_hops_{0};
    uint8_t trace_ch_{0};  // answer on the channel the request came in on
    uint8_t trace_len_{0};
    uint8_t trace_buf_[160]{};  // the RouteDiscovery as it arrived; a full route is 8 nodes

    // Who asked us to introduce ourselves, and whether they wanted the position or the
    // NodeInfo. Noted in the packet callback, answered from the timer: building and
    // encrypting a packet is far too much work to do inside packet handling.
    uint32_t intro_dest_{0};
    bool intro_pos_{false};
    uint32_t metrics_dest_{0};
    bool metrics_stats_{false};   // the request asked for LocalStats, not device metrics
    uint32_t metrics_req_id_{0};  // the request our metrics answer belongs to
    std::string echo_text_{};
    // The echo waits its turn: the read receipt goes first, and the reply follows a
    // couple of seconds later. Two packets back to back would have us transmitting
    // while the sender is still finishing its own retransmission.
    uint16_t echo_delay_{0};
    // Long enough that the echo never shares the air with our own relay of the very
    // packet we are answering, nor with the sender's own retransmissions.
    static constexpr uint16_t ECHO_AFTER_RX = 240;   // ~4 s after hearing the text
    static constexpr uint16_t ECHO_AFTER_ACK = 300;  // ~5 s after the receipt
    uint32_t ack_dest_{0};
    uint32_t ack_request_id_{0};
    uint8_t ack_channel_{0};
    bool apply_radio_{false};
    uint32_t identity_sig_{0};                         // last announced identity, to avoid re-announcing
    static constexpr uint16_t APPLY_DELAY_TICKS = 72;  // ~1.2 s at ~60 Hz
    void apply_settings();
    uint32_t identity_signature() const;
    static constexpr uint32_t RX_HOLD_TICKS = 180;

    // Store and Forward (lightweight, RAM-only): keep the last few mesh texts and, on a
    // "SF" request, replay them to the asker (drained one at a time so the single-slot TX
    // is respected). A real Meshtastic server uses PSRAM for thousands of records; we hold
    // a handful, which covers "catch me up on what I missed" on a small mesh.
    // Who it was addressed to and which channel it came in on travel with the text,
    // because that is what decides who may be given it back. Meshtastic's own module
    // keeps the same three fields and filters replay on them (StoreForwardModule,
    // getNumAvailablePackets): a client is offered broadcasts and messages addressed
    // to itself, nothing else.
    struct SFMsg {
        uint32_t from;
        uint32_t to;
        std::string text;
        uint32_t tick;
        uint8_t channel;
    };
    std::vector<SFMsg> sf_store_{};
    std::vector<std::string> sf_replay_{};  // pending replay lines (drained in on_timer)
    uint32_t sf_replay_dest_{0};
    void store_and_forward(const meshtastic::MeshPacket& pkt);

    std::unique_ptr<MeshtasticLogger> logger_{};

    // Bound one at a time, in settings_bindings(), rather than written out here as a
    // braced list. A braced list of eighty-four entries is materialised as an array in
    // the frame of whatever constructs it - 1344 bytes of the M0's 4 KB process stack,
    // inside this view's constructor, which the launcher may be calling from several
    // frames down its touch dispatch. Built element by element it needs no array at all.
    SettingBindings settings_bindings();
    app_settings::SettingsManager settings_{
        "meshtastic",
        app_settings::Mode::RX,
        settings_bindings()};

    RxRadioState radio_state_{
        868825000,
        500000,
        2500000,
        ReceiverModel::Mode::WidebandFMAudio};

    // Sub-views MUST be declared before tab_view_
    MeshtasticChatView chat_view_;
    MeshtasticNodesView nodes_view_;
    MeshtasticMapView map_view_;
    MeshtasticSetupMenuView setup_menu_;

    TabView tab_view_{
        {{"Chat", Color::green(), &chat_view_},
         {"Nodes", Color::yellow(), &nodes_view_},
         {"Data", Color::blue(), &map_view_},
         {"Setup", Color::cyan(), &setup_menu_}}};

    MessageHandlerRegistration handler_lora_{
        Message::ID::LoRaPacket,
        [this](const Message* const p) {
            on_lora_packet(static_cast<const LoRaPacketMessage*>(p));
        }};

    MessageHandlerRegistration handler_lora_rx_{
        Message::ID::LoRaRxStatus,
        [this](const Message* const p) {
            on_lora_rx_status(static_cast<const LoRaRxStatusMessage*>(p));
        }};

    MessageHandlerRegistration handler_gps_{
        Message::ID::GPSPosData,
        [this](const Message* const p) {
            on_gps_update(static_cast<const GPSPosDataMessage*>(p));
        }};

    // NB: no BatteryStateData handler - SystemStatusView already registers one
    // and the handler map allows only one per ID (double reg -> M0 "MsgDblReg"
    // panic). Battery is read on demand via BatteryManagement instead.

    MessageHandlerRegistration handler_env_{
        Message::ID::EnvironmentData,
        [this](const Message* const p) {
            on_environment_update(static_cast<const EnvironmentDataMessage*>(p));
        }};

    MessageHandlerRegistration handler_txprog_{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto& msg = *reinterpret_cast<const TXProgressMessage*>(p);
            if (msg.done) on_tx_done(msg.progress);
        }};

    MessageHandlerRegistration handler_frame_{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) { on_timer(); }};
};

}  // namespace ui

#endif /* __UI_MESHTASTIC_H__ */
