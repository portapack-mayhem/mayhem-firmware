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
#include "portapack.hpp"
#include "message.hpp"

#include <string>
#include <array>
#include <cstdint>

namespace ui::external_app::battleship {

using namespace portapack;

constexpr uint8_t GRID_SIZE = 10;
constexpr uint8_t CELL_SIZE = 24;
constexpr uint8_t GRID_OFFSET_X = 0;
constexpr uint8_t GRID_OFFSET_Y = 32;
constexpr uint16_t BUTTON_Y = 280;
constexpr uint32_t DEFAULT_FREQUENCY = 433920000;
constexpr uint32_t FSK_BAUDRATE = 1200;
constexpr uint32_t FSK_DEVIATION = 3750;

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

    RxRadioState rx_radio_state_{};
    TxRadioState tx_radio_state_{};

    app_settings::SettingsManager settings_{
        "battleship",
        app_settings::Mode::RX_TX,
        {{"rx_freq"sv, &rx_frequency},
         {"tx_freq"sv, &tx_frequency},
         {"wins"sv, &wins},
         {"losses"sv, &losses}}};

    GameState game_state{GameState::MENU};
    bool is_red_team{false};
    bool opponent_ready{false};
    uint32_t wins{0};
    uint32_t losses{0};

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

    uint32_t tx_frequency{DEFAULT_FREQUENCY};
    uint32_t rx_frequency{DEFAULT_FREQUENCY};
    std::string rx_buffer{};
    bool is_transmitting{false};

    bool in_message{false};
    std::string current_message{};

    RSSI rssi{
        {21 * 8, 0, 6 * 8, 4}};

    FrequencyField field_frequency{
        {10, 50}};

    Text text_status{
        {10, 16, 220, 16},
        "Choose your team!"};

    Text text_score{
        {170, 16, 60, 16},
        "W:0 L:0"};

    Button button_red_team{
        {20, 100, 90, 40},
        "RED TEAM"};

    Button button_blue_team{
        {130, 100, 90, 40},
        "BLUE TEAM"};

    Button button_rotate{
        {10, BUTTON_Y, 60, 32},
        "Rotate"};

    Button button_place{
        {80, BUTTON_Y, 60, 32},
        "Place"};

    Button button_fire{
        {80, BUTTON_Y, 60, 32},
        "Fire!"};

    Button button_menu{
        {150, BUTTON_Y, 60, 32},
        "Menu"};

    void init_game();
    void reset_game();
    void start_team(bool red);
    void setup_ships();
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
    void parse_fsk_data(const FskPacketData& packet);
    bool parse_coords(const std::string& coords, uint8_t& x, uint8_t& y);
    void mark_ship_sunk(uint8_t x, uint8_t y, std::array<std::array<CellState, GRID_SIZE>, GRID_SIZE>& grid);

    void configure_radio(bool tx);

    MessageHandlerRegistration message_handler_packet{
        Message::ID::FskPacket,
        [this](Message* const p) {
            const auto message = static_cast<const FskPacketMessage*>(p);
            parse_fsk_data(message->packet);
        }};

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto message = static_cast<const TXProgressMessage*>(p);
            if (message->done) {
                transmitter_model.disable();
                chThdSleepMilliseconds(200);
                configure_radio(false);

                if (game_state == GameState::MY_TURN) {
                    current_status = "Waiting for response";
                    set_dirty();
                }
            }
        }};
};

}  // namespace ui::external_app::battleship

#endif
