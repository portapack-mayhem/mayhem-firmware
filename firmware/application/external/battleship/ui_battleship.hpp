/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#ifndef __UI_BATTLESHIP_H__
#define __UI_BATTLESHIP_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_transmitter.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "audio.hpp"
#include "message.hpp"
#include "pocsag.hpp"

#include <array>

namespace ui::external_app::battleship {

constexpr uint8_t GRID_SIZE = 10;
constexpr uint8_t CELL_SIZE = 24;
constexpr uint8_t GRID_OFFSET_X = 0;
constexpr uint8_t GRID_OFFSET_Y = 32;

enum class ShipType : uint8_t {
    CARRIER = 5,
    BATTLESHIP = 4,
    CRUISER = 3,
    SUBMARINE = 3,
    DESTROYER = 2
};

enum class GameState : uint8_t {
    MENU,
    PLACING_SHIPS,
    WAITING_FOR_OPPONENT,
    MY_TURN,
    OPPONENT_TURN,
    GAME_OVER
};

enum class CellState : uint8_t {
    EMPTY,
    SHIP,
    HIT,
    MISS,
    SUNK
};

enum class MessageType : uint8_t {
    READY,
    SHOT,
    HIT,
    MISS,
    SUNK,
    WIN
};

struct Ship {
    ShipType type;
    uint8_t x;
    uint8_t y;
    bool horizontal;
    uint8_t hits;
    bool placed;

    uint8_t size() const {
        return static_cast<uint8_t>(type);
    }

    bool is_sunk() const {
        return hits >= size();
    }
};

struct GameMessage {
    MessageType type;
    uint8_t x;
    uint8_t y;
};

class BattleshipView : public View {
   public:
    BattleshipView(NavigationView& nav);
    ~BattleshipView();

    void focus() override;
    void paint(Painter& painter) override;
    bool on_touch(const TouchEvent event) override;
    bool on_encoder(const EncoderEvent delta) override;
    bool on_key(const KeyEvent key) override;

    std::string title() const override { return "Battleship"; }

   private:
    NavigationView& nav_;

    RxRadioState rx_radio_state_{
        433920000 /* frequency */,
        1750000 /* bandwidth */,
        2280000 /* sampling rate */
    };

    TxRadioState tx_radio_state_{
        433920000 /* frequency */,
        1750000 /* bandwidth */,
        2280000 /* sampling rate */
    };

    // Settings
    bool sound_enabled{true};
    bool rf_amp_enabled{false};
    uint8_t lna_gain{24};
    uint8_t vga_gain{24};
    uint8_t tx_gain{35};

    app_settings::SettingsManager settings_{
        "battleship",
        app_settings::Mode::RX_TX,
        {{"rx_freq"sv, &rx_frequency},
         {"tx_freq"sv, &tx_frequency},
         {"rf_amp"sv, &rf_amp_enabled}}};

    GameState game_state{GameState::MENU};
    bool is_red_team{false};
    bool opponent_ready{false};
    uint8_t wins{0};
    uint8_t losses{0};

    std::array<std::array<CellState, GRID_SIZE>, GRID_SIZE> my_grid{};
    std::array<std::array<CellState, GRID_SIZE>, GRID_SIZE> enemy_grid{};

    std::string current_status{"Choose your team!"};
    std::string current_score{"W:0 L:0"};

    std::array<Ship, 5> my_ships{};
    uint8_t current_ship_index{0};
    bool placing_horizontal{true};
    uint8_t ships_remaining{5};
    uint8_t enemy_ships_remaining{5};

    uint8_t cursor_x{0};
    uint8_t cursor_y{0};
    uint8_t target_x{0};
    uint8_t target_y{0};
    bool touch_enabled{true};

    uint32_t tx_frequency{433920000};
    uint32_t rx_frequency{433920000};
    bool is_transmitting{false};

    // POCSAG decoding state
    pocsag::EccContainer ecc{};
    pocsag::POCSAGState pocsag_state{&ecc};
    uint32_t last_address{0};

    // UI Elements - Menu/Settings Screen
    Text text_title{{60, 2, 120, 24}, "BATTLESHIP"};
    Text text_subtitle{{40, 20, 160, 16}, "Naval Combat Game"};

    Rectangle rect_radio_settings{{12, 40, 216, 118}, Color::dark_grey()};
    Text label_radio{{17, 45, 100, 16}, "RADIO SETUP"};
    ButtonWithEncoder button_frequency{{17, 65, 11 * 8, 20}, ""};

    // Radio controls
    Text label_rf_amp{{17, 90, 35, 16}, "AMP:"};
    Checkbox checkbox_rf_amp{{55, 90}, 3, "", false};

    Text label_lna{{17, 118, 30, 16}, "LNA:"};
    NumberField field_lna{{50, 118}, 2, {0, 40}, 8, ' '};

    Text label_vga{{90, 118, 30, 16}, "VGA:"};
    NumberField field_vga{{125, 118}, 2, {0, 62}, 8, ' '};

    Text label_tx_gain{{155, 118, 25, 16}, "TX:"};
    NumberField field_tx_gain{{185, 118}, 2, {0, 47}, 8, ' '};

    Rectangle rect_audio_settings{{12, 164, 216, 45}, Color::dark_grey()};
    Text label_audio{{17, 169, 80, 16}, "AUDIO"};
    Checkbox checkbox_sound{{17, 187}, 8, "Sound On", true};
    Text label_volume{{110, 187, 50, 16}, "Volume:"};
    AudioVolumeField field_volume{{165, 187}};

    Rectangle rect_team_selection{{12, 217, 216, 75}, Color::dark_grey()};
    Text label_team{{17, 222, 110, 16}, "SELECT TEAM"};
    Button button_red_team{{25, 242, 85, 45}, "RED\nTEAM"};
    Button button_blue_team{{130, 242, 85, 45}, "BLUE\nTEAM"};

    // In-game UI elements
    RSSI rssi{{21 * 8, 0, 6 * 8, 4}};
    Text text_status{{10, 16, 220, 16}, ""};
    Text text_score{{170, 16, 60, 16}, ""};
    Button button_rotate{{10, 265, 65, 32}, "Rotate"};
    Button button_place{{82, 265, 65, 32}, "Place"};
    Button button_fire{{82, 265, 65, 32}, "Fire!"};
    Button button_menu{{155, 265, 65, 32}, "Menu"};

    // Methods
    void init_game();
    void reset_game();
    void start_team(bool red);
    void setup_ships();
    void draw_menu_screen(Painter& painter);
    void draw_grid(Painter& painter, uint8_t grid_x, uint8_t grid_y, const std::array<std::array<CellState, GRID_SIZE>, GRID_SIZE>& grid, bool show_ships, bool is_offense_grid = false);
    void draw_cell(Painter& painter, uint8_t cell_x, uint8_t cell_y, CellState state, bool show_ships, bool is_offense_grid, bool is_cursor);
    void draw_ship_preview(Painter& painter);
    void place_ship();
    bool can_place_ship(uint8_t x, uint8_t y, uint8_t size, bool horizontal);
    void fire_at_position();
    void process_shot(uint8_t x, uint8_t y);
    void update_score();
    bool is_cursor_at(uint8_t x, uint8_t y, bool is_offense_grid);

    void send_message(const GameMessage& msg);
    void process_message(const std::string& message);
    bool parse_coords(const std::string& coords, uint8_t& x, uint8_t& y);
    void mark_ship_sunk(uint8_t x, uint8_t y, std::array<std::array<CellState, GRID_SIZE>, GRID_SIZE>& grid);

    void configure_radio_tx();
    void configure_radio_rx();

    MessageHandlerRegistration message_handler_packet{
        Message::ID::POCSAGPacket,
        [this](Message* const p) {
            const auto message = static_cast<const POCSAGPacketMessage*>(p);
            on_pocsag_packet(message);
        }};

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto message = static_cast<const TXProgressMessage*>(p);
            on_tx_progress(message->progress, message->done);
        }};

    void on_pocsag_packet(const POCSAGPacketMessage* message);
    void on_tx_progress(const uint32_t progress, const bool done);
};

}  // namespace ui::external_app::battleship

#endif
