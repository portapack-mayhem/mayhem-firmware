/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#include "ui_battleship.hpp"
#include "portapack_shared_memory.hpp"
#include "utility.hpp"
#include "modems.hpp"
#include "bch_code.hpp"

using namespace portapack;
using namespace modems;

namespace ui::external_app::battleship {

// POCSAG address for battleship game messages
constexpr uint32_t BATTLESHIP_BASE_ADDRESS = 1000000;
constexpr uint32_t RED_TEAM_ADDRESS = BATTLESHIP_BASE_ADDRESS + 1;
constexpr uint32_t BLUE_TEAM_ADDRESS = BATTLESHIP_BASE_ADDRESS + 2;

BattleshipView::BattleshipView(NavigationView& nav)
    : nav_{nav} {
    CELL_SIZE = screen_width / GRID_SIZE;
    baseband::run_image(portapack::spi_flash::image_tag_pocsag2);

    add_children({&text_title, &text_subtitle,
                  //&rect_radio_settings, &rect_audio_settings, &rect_team_selection,
                  &label_radio, &button_frequency,
                  &label_rf_amp, &checkbox_rf_amp,
                  &label_lna, &field_lna,
                  &label_vga, &field_vga,
                  &label_tx_gain, &field_tx_gain,
                  &label_audio,
                  &checkbox_sound, &label_volume, &field_volume,
                  &label_team,
                  &button_red_team, &button_blue_team,
                  &rssi,
                  &button_rotate, &button_place, &button_fire, &button_menu});

    // Hide in-game elements
    rssi.hidden(true);
    button_rotate.hidden(true);
    button_place.hidden(true);
    button_fire.hidden(true);
    button_menu.hidden(true);

    // Configure frequency button
    button_frequency.set_text("<" + to_string_short_freq(tx_frequency) + ">");

    button_frequency.on_select = [this, &nav](ButtonWithEncoder& button) {
        auto new_view = nav_.push<FrequencyKeypadView>(tx_frequency);
        new_view->on_changed = [this, &button](rf::Frequency f) {
            tx_frequency = f;
            rx_frequency = f;
            button_frequency.set_text("<" + to_string_short_freq(tx_frequency) + ">");
            if (!is_transmitting) {
                receiver_model.set_target_frequency(rx_frequency);
            }
        };
    };

    button_frequency.on_change = [this]() {
        int64_t def_step = 25000;
        int64_t new_freq = static_cast<int64_t>(tx_frequency) + (button_frequency.get_encoder_delta() * def_step);

        if (new_freq < 1) new_freq = 1;
        if (new_freq > 7200000000LL) new_freq = 7200000000LL;

        tx_frequency = static_cast<uint32_t>(new_freq);
        rx_frequency = tx_frequency;
        button_frequency.set_encoder_delta(0);
        button_frequency.set_text("<" + to_string_short_freq(tx_frequency) + ">");
        if (!is_transmitting) {
            receiver_model.set_target_frequency(rx_frequency);
        }
    };

    // Radio controls
    checkbox_rf_amp.set_value(rf_amp_enabled);
    checkbox_rf_amp.on_select = [this](Checkbox&, bool v) {
        rf_amp_enabled = v;
        transmitter_model.set_rf_amp(v);
        receiver_model.set_rf_amp(v);
    };

    field_lna.set_value(lna_gain);
    field_lna.on_change = [this](int32_t v) {
        lna_gain = v;
        receiver_model.set_lna(v);
    };

    field_vga.set_value(vga_gain);
    field_vga.on_change = [this](int32_t v) {
        vga_gain = v;
        receiver_model.set_vga(v);
    };

    field_tx_gain.set_value(tx_gain);
    field_tx_gain.on_change = [this](int32_t v) {
        tx_gain = v;
        transmitter_model.set_tx_gain(v);
    };

    // Audio controls
    checkbox_sound.set_value(sound_enabled);
    checkbox_sound.on_select = [this](Checkbox&, bool v) {
        sound_enabled = v;
        if (sound_enabled) {
            audio::output::unmute();
        } else {
            audio::output::mute();
        }
    };

    // Team selection
    button_red_team.on_select = [this](Button&) {
        start_team(true);
    };

    button_blue_team.on_select = [this](Button&) {
        start_team(false);
    };

    // In-game controls
    button_rotate.on_select = [this](Button&) {
        placing_horizontal = !placing_horizontal;
        set_dirty();
    };

    button_place.on_select = [this](Button&) {
        place_ship();
    };

    button_fire.on_select = [this](Button&) {
        fire_at_position();
    };

    button_menu.on_select = [this](Button&) {
        reset_game();
    };

    // Set proper rectangles for layout
    button_frequency.set_parent_rect({17, 65, 11 * 8, 20});
    checkbox_rf_amp.set_parent_rect({55, 90, 24, 24});
    field_lna.set_parent_rect({50, 118, 32, 16});
    field_vga.set_parent_rect({125, 118, 32, 16});
    field_tx_gain.set_parent_rect({185, 118, 32, 16});
    checkbox_sound.set_parent_rect({17, 187, 80, 24});
    field_volume.set_parent_rect({165, 187, 32, 16});
    // button_red_team.set_parent_rect({25, 242, 85, 45});
    // button_blue_team.set_parent_rect({130, 242, 85, 45});

    // Make menu elements focusable
    button_frequency.set_focusable(true);
    checkbox_rf_amp.set_focusable(true);
    field_lna.set_focusable(true);
    field_vga.set_focusable(true);
    field_tx_gain.set_focusable(true);
    checkbox_sound.set_focusable(true);
    field_volume.set_focusable(true);
    button_red_team.set_focusable(true);
    button_blue_team.set_focusable(true);

    set_focusable(true);
    init_game();
}

BattleshipView::~BattleshipView() {
    transmitter_model.disable();
    receiver_model.disable();
    audio::output::stop();
    baseband::shutdown();
}

void BattleshipView::focus() {
    if (game_state == GameState::MENU) {
        button_frequency.focus();
    } else {
        View::focus();
    }
}

void BattleshipView::init_game() {
    for (uint8_t y = 0; y < GRID_SIZE; y++) {
        for (uint8_t x = 0; x < GRID_SIZE; x++) {
            my_grid[y][x] = CellState::EMPTY;
            enemy_grid[y][x] = CellState::EMPTY;
        }
    }
    setup_ships();
    update_score();
}

void BattleshipView::show_hide_menu(bool menu_vis) {
    text_title.hidden(!menu_vis);
    text_subtitle.hidden(!menu_vis);
    // rect_radio_settings.hidden(!menu_vis);    rect_audio_settings.hidden(!menu_vis);    rect_team_selection.hidden(!menu_vis);
    label_radio.hidden(!menu_vis);
    button_frequency.hidden(!menu_vis);
    label_rf_amp.hidden(!menu_vis);
    checkbox_rf_amp.hidden(!menu_vis);
    label_lna.hidden(!menu_vis);
    field_lna.hidden(!menu_vis);
    label_vga.hidden(!menu_vis);
    field_vga.hidden(!menu_vis);
    label_tx_gain.hidden(!menu_vis);
    field_tx_gain.hidden(!menu_vis);
    label_audio.hidden(!menu_vis);
    checkbox_sound.hidden(!menu_vis);
    label_volume.hidden(!menu_vis);
    field_volume.hidden(!menu_vis);
    label_team.hidden(!menu_vis);
    button_red_team.hidden(!menu_vis);
    button_blue_team.hidden(!menu_vis);
    rssi.hidden(menu_vis);
    button_rotate.hidden(menu_vis);
    button_place.hidden(menu_vis);
    button_menu.hidden(menu_vis);

    // button_rotate.set_focusable(false); //no need, since can't focus on hidden
    // button_place.set_focusable(false);
    // button_menu.set_focusable(false);
    set_dirty();
}

void BattleshipView::reset_game() {
    transmitter_model.disable();
    receiver_model.disable();
    audio::output::stop();

    game_state = GameState::MENU;
    is_red_team = false;
    opponent_ready = false;
    current_ship_index = 0;
    placing_horizontal = true;
    ships_remaining = 5;
    enemy_ships_remaining = 5;
    cursor_x = 0;
    cursor_y = 0;
    target_x = 0;
    target_y = 0;
    is_transmitting = false;
    last_address = 0;

    init_game();

    current_status = "Choose your team!";
    update_score();

    // Toggle visibility
    show_hide_menu(true);

    button_frequency.focus();
    set_dirty();
}

void BattleshipView::setup_ships() {
    static const ShipType types[] = {ShipType::CARRIER, ShipType::BATTLESHIP,
                                     ShipType::CRUISER, ShipType::SUBMARINE, ShipType::DESTROYER};
    for (uint8_t i = 0; i < 5; i++) {
        my_ships[i] = {types[i], 0, 0, true, 0, false};
    }
}

void BattleshipView::start_team(bool red) {
    is_red_team = red;
    game_state = GameState::PLACING_SHIPS;

    // Toggle visibility
    show_hide_menu(false);

    current_status = "Place carrier (5)";

    focus();
    is_transmitting = true;
    configure_radio_rx();
    set_dirty();
}

void BattleshipView::configure_radio_tx() {
    if (is_transmitting) return;

    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();

    chThdSleepMilliseconds(100);

    baseband::run_image(portapack::spi_flash::image_tag_fsktx);

    chThdSleepMilliseconds(100);

    transmitter_model.set_target_frequency(tx_frequency);
    transmitter_model.set_sampling_rate(2280000);
    transmitter_model.set_baseband_bandwidth(1750000);
    transmitter_model.set_rf_amp(rf_amp_enabled);
    transmitter_model.set_tx_gain(tx_gain);

    is_transmitting = true;
}

void BattleshipView::configure_radio_rx() {
    if (is_transmitting) {
        transmitter_model.disable();
        baseband::shutdown();
        chThdSleepMilliseconds(100);
    }

    baseband::run_image(portapack::spi_flash::image_tag_pocsag2);
    chThdSleepMilliseconds(100);

    receiver_model.set_target_frequency(rx_frequency);
    receiver_model.set_sampling_rate(3072000);
    receiver_model.set_baseband_bandwidth(1750000);
    receiver_model.set_rf_amp(rf_amp_enabled);
    receiver_model.set_lna(lna_gain);
    receiver_model.set_vga(vga_gain);

    baseband::set_pocsag();
    receiver_model.enable();

    audio::set_rate(audio::Rate::Hz_24000);

    if (sound_enabled) {
        audio::output::start();
    }

    is_transmitting = false;
    current_status = "RX Ready";
    set_dirty();
}

void BattleshipView::paint(Painter& painter) {
    painter.fill_rectangle({0, 0, UI_POS_MAXWIDTH, UI_POS_MAXHEIGHT}, Color::black());

    if (game_state == GameState::MENU) {
        draw_menu_screen(painter);

        // Custom paint team buttons
        if (!button_red_team.hidden()) {
            Rect r = button_red_team.screen_rect();
            painter.fill_rectangle(r, Color::dark_red());
            painter.draw_rectangle(r, Color::red());

            if (button_red_team.has_focus()) {
                painter.draw_rectangle({r.location().x() - 1, r.location().y() - 1, r.size().width() + 2, r.size().height() + 2}, Color::yellow());
            }

            auto style_white = Style{
                .font = ui::font::fixed_8x16,
                .background = Color::dark_red(),
                .foreground = Color::white()};
            painter.draw_string(r.center() - Point(24, 16), style_white, "RED");
            painter.draw_string(r.center() - Point(24, 0), style_white, "TEAM");
        }

        if (!button_blue_team.hidden()) {
            Rect r = button_blue_team.screen_rect();
            painter.fill_rectangle(r, Color::dark_blue());
            painter.draw_rectangle(r, Color::blue());

            if (button_blue_team.has_focus()) {
                painter.draw_rectangle({r.location().x() - 1, r.location().y() - 1, r.size().width() + 2, r.size().height() + 2}, Color::yellow());
            }

            auto style_white = Style{
                .font = ui::font::fixed_8x16,
                .background = Color::dark_blue(),
                .foreground = Color::white()};
            painter.draw_string(r.center() - Point(24, 16), style_white, "BLUE");
            painter.draw_string(r.center() - Point(24, 0), style_white, "TEAM");
        }

        return;
    }

    Color team_color = is_red_team ? Color::red() : Color::blue();
    painter.fill_rectangle({0, 5, screen_width, 16}, team_color);

    auto style_white = Style{
        .font = ui::font::fixed_8x16,
        .background = team_color,
        .foreground = Color::white()};
    painter.draw_string({UI_POS_X_CENTER(9), 5}, style_white, is_red_team ? "RED TEAM" : "BLUE TEAM");

    auto style_status = Style{
        .font = ui::font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::white()};
    painter.fill_rectangle({0, 21, screen_width, 16}, Color::black());
    painter.draw_string({5, 21}, style_status, current_status);
    painter.draw_string({170, 21}, style_status, current_score);

    if (game_state == GameState::PLACING_SHIPS) {
        draw_grid(painter, GRID_OFFSET_X, GRID_OFFSET_Y + 5, my_grid, true);
        if (current_ship_index < 5) {
            draw_ship_preview(painter);
        }
    } else if (game_state == GameState::MY_TURN) {
        draw_grid(painter, GRID_OFFSET_X, GRID_OFFSET_Y + 5, enemy_grid, false, true);
        painter.draw_string({0, GRID_OFFSET_Y + GRID_SIZE * CELL_SIZE + 10}, style_status,
                            "Enemy: " + to_string_dec_uint(enemy_ships_remaining));
    } else if (game_state == GameState::OPPONENT_TURN || game_state == GameState::WAITING_FOR_OPPONENT) {
        draw_grid(painter, GRID_OFFSET_X, GRID_OFFSET_Y + 5, my_grid, true);
        painter.draw_string({0, GRID_OFFSET_Y + GRID_SIZE * CELL_SIZE + 10}, style_status,
                            "You: " + to_string_dec_uint(ships_remaining));
    } else if (game_state == GameState::GAME_OVER) {
        painter.draw_string({UI_POS_X_CENTER(11), 150}, style_status, "Game Over!");
        painter.draw_string({30, 170}, style_status, current_status);
    }
}

void BattleshipView::draw_menu_screen(Painter& painter) {
    painter.draw_hline({12, 20 + 16}, screen_width - 24, Color::dark_cyan());

    painter.fill_rectangle({13, 59, screen_width - 26, 116}, Color::dark_grey());
    painter.draw_hline({12, 58}, screen_width - 24, Color::cyan());
    painter.draw_hline({12, 157}, screen_width - 24, Color::cyan());

    painter.fill_rectangle({13, 165, screen_width - 24, 57}, Color::dark_grey());
    painter.draw_hline({12, 164}, screen_width - 24, Color::cyan());
    painter.draw_hline({12, 223}, screen_width - 24, Color::cyan());

    painter.fill_rectangle({13, 232, screen_width - 26, 67}, Color::dark_grey());
    painter.draw_hline({12, 232}, screen_width - 24, Color::cyan());
    painter.draw_hline({12, 300}, screen_width - 24, Color::cyan());

    // Radio status indicator
    Point indicator_pos{UI_POS_MAXWIDTH - 20, 59};
    if (is_transmitting) {
        painter.fill_rectangle({indicator_pos, {6, 6}}, Color::red());
        painter.draw_rectangle({indicator_pos.x() - 1, indicator_pos.y() - 1, 8, 8}, Color::light_grey());
    } else {
        painter.fill_rectangle({indicator_pos, {6, 6}}, Color::green());
        painter.draw_rectangle({indicator_pos.x() - 1, indicator_pos.y() - 1, 8, 8}, Color::light_grey());
    }
}

void BattleshipView::draw_grid(Painter& painter, uint16_t grid_x, uint16_t grid_y, const std::array<std::array<CellState, GRID_SIZE>, GRID_SIZE>& grid, bool show_ships, bool is_offense_grid) {
    painter.fill_rectangle({grid_x, grid_y, GRID_SIZE * CELL_SIZE, GRID_SIZE * CELL_SIZE},
                           Color::dark_blue());

    for (uint8_t i = 0; i <= GRID_SIZE; i++) {
        painter.draw_vline({grid_x + i * CELL_SIZE, grid_y},
                           GRID_SIZE * CELL_SIZE, Color::grey());
        painter.draw_hline({grid_x, grid_y + i * CELL_SIZE},
                           GRID_SIZE * CELL_SIZE, Color::grey());
    }

    for (uint16_t y = 0; y < GRID_SIZE; y++) {
        for (uint16_t x = 0; x < GRID_SIZE; x++) {
            draw_cell(painter, grid_x + x * CELL_SIZE + 1, grid_y + y * CELL_SIZE + 1, grid[y][x], show_ships, is_offense_grid, is_cursor_at(x, y, is_offense_grid));
        }
    }
}

void BattleshipView::draw_cell(Painter& painter, uint16_t cell_x, uint16_t cell_y, CellState state, bool show_ships, bool is_offense_grid, bool is_cursor) {
    Color cell_color = Color::dark_blue();
    bool should_fill = false;

    if (game_state == GameState::PLACING_SHIPS && !is_offense_grid && current_ship_index < 5) {
        uint8_t ship_size = my_ships[current_ship_index].size();
        for (uint8_t i = 0; i < ship_size; i++) {
            uint16_t preview_x = placing_horizontal ? cursor_x + i : cursor_x;
            uint16_t preview_y = placing_horizontal ? cursor_y : cursor_y + i;
            uint16_t grid_x = (cell_x - 1) / CELL_SIZE;
            uint16_t grid_y = (cell_y - GRID_OFFSET_Y - 6) / CELL_SIZE;
            if (grid_x == preview_x && grid_y == preview_y && preview_x < GRID_SIZE && preview_y < GRID_SIZE) {
                return;
            }
        }
    }

    switch (state) {
        case CellState::SHIP:
            if (show_ships) {
                cell_color = Color::grey();
                should_fill = true;
            }
            break;
        case CellState::HIT:
            cell_color = Color::red();
            should_fill = true;
            break;
        case CellState::MISS:
            cell_color = Color::light_grey();
            should_fill = true;
            break;
        case CellState::SUNK:
            cell_color = Color::dark_red();
            should_fill = true;
            break;
        default:
            if (is_offense_grid && state == CellState::EMPTY) {
                cell_color = Color::dark_grey();
                should_fill = true;
            }
            break;
    }

    if (should_fill) {
        painter.fill_rectangle({cell_x, cell_y, CELL_SIZE - 2, CELL_SIZE - 2}, cell_color);
    }

    if (state == CellState::HIT || state == CellState::SUNK) {
        painter.draw_hline({cell_x + 4, cell_y + 4}, CELL_SIZE - 10, Color::white());
        painter.draw_hline({cell_x + 4, cell_y + CELL_SIZE - 6}, CELL_SIZE - 10, Color::white());
        painter.draw_vline({cell_x + 4, cell_y + 4}, CELL_SIZE - 10, Color::white());
        painter.draw_vline({cell_x + CELL_SIZE - 6, cell_y + 4}, CELL_SIZE - 10, Color::white());
    } else if (state == CellState::MISS) {
        painter.draw_hline({cell_x + 8, cell_y + 4}, 8, Color::white());
        painter.draw_hline({cell_x + 8, cell_y + CELL_SIZE - 6}, 8, Color::white());
        painter.draw_vline({cell_x + 4, cell_y + 8}, 8, Color::white());
        painter.draw_vline({cell_x + CELL_SIZE - 6, cell_y + 8}, 8, Color::white());
    }

    if (is_cursor) {
        painter.draw_rectangle({cell_x - 1, cell_y - 1, CELL_SIZE, CELL_SIZE},
                               is_offense_grid && game_state == GameState::MY_TURN ? Color::yellow() : Color::cyan());
    }
}

bool BattleshipView::is_cursor_at(uint8_t x, uint8_t y, bool is_offense_grid) {
    if (game_state == GameState::PLACING_SHIPS && !is_offense_grid) {
        return x == cursor_x && y == cursor_y;
    } else if (is_offense_grid && game_state == GameState::MY_TURN) {
        return x == target_x && y == target_y;
    }
    return false;
}

void BattleshipView::draw_ship_preview(Painter& painter) {
    if (current_ship_index >= 5) return;

    const Ship& ship = my_ships[current_ship_index];
    uint8_t size = ship.size();
    bool can_place = can_place_ship(cursor_x, cursor_y, size, placing_horizontal);

    for (uint8_t i = 0; i < size; i++) {
        uint8_t x = placing_horizontal ? cursor_x + i : cursor_x;
        uint8_t y = placing_horizontal ? cursor_y : cursor_y + i;

        if (x < GRID_SIZE && y < GRID_SIZE) {
            uint16_t cell_x = GRID_OFFSET_X + x * CELL_SIZE + 1;
            uint16_t cell_y = GRID_OFFSET_Y + 5 + y * CELL_SIZE + 1;

            Color preview_color = can_place ? Color::green() : Color::red();

            painter.fill_rectangle({cell_x, cell_y, CELL_SIZE - 2, CELL_SIZE - 2}, preview_color);
            painter.draw_rectangle({cell_x, cell_y, CELL_SIZE - 2, CELL_SIZE - 2}, Color::white());
        }
    }
}

bool BattleshipView::can_place_ship(uint8_t x, uint8_t y, uint8_t size, bool horizontal) {
    if ((horizontal && x + size > GRID_SIZE) || (!horizontal && y + size > GRID_SIZE)) {
        return false;
    }

    for (uint8_t i = 0; i < size; i++) {
        uint8_t check_x = horizontal ? x + i : x;
        uint8_t check_y = horizontal ? y : y + i;

        if (my_grid[check_y][check_x] != CellState::EMPTY) {
            return false;
        }

        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int adj_x = check_x + dx;
                int adj_y = check_y + dy;

                if (adj_x >= 0 && adj_x < GRID_SIZE &&
                    adj_y >= 0 && adj_y < GRID_SIZE) {
                    if (my_grid[adj_y][adj_x] == CellState::SHIP) {
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

void BattleshipView::place_ship() {
    if (current_ship_index >= 5) return;

    Ship& ship = my_ships[current_ship_index];
    uint8_t size = ship.size();

    if (!can_place_ship(cursor_x, cursor_y, size, placing_horizontal)) {
        current_status = "Invalid placement!";
        set_dirty();
        return;
    }

    ship.x = cursor_x;
    ship.y = cursor_y;
    ship.horizontal = placing_horizontal;
    ship.placed = true;

    for (uint8_t i = 0; i < size; i++) {
        uint8_t x = placing_horizontal ? cursor_x + i : cursor_x;
        uint8_t y = placing_horizontal ? cursor_y : cursor_y + i;
        my_grid[y][x] = CellState::SHIP;
    }

    current_ship_index++;

    if (current_ship_index >= 5) {
        button_rotate.hidden(true);
        button_place.hidden(true);

        send_message({MessageType::READY, 0, 0});

        if (is_red_team) {
            game_state = GameState::MY_TURN;
            current_status = "Your turn! Fire!";
            button_fire.hidden(false);
            button_fire.set_focusable(false);
            touch_enabled = true;
        } else {
            game_state = GameState::WAITING_FOR_OPPONENT;
            current_status = "Waiting for Red...";
            touch_enabled = false;
        }

        focus();
    } else {
        static const char* ship_names[] = {"carrier (5)", "battleship (4)", "cruiser (3)",
                                           "submarine (3)", "destroyer (2)"};
        current_status = "Place ";
        current_status += ship_names[current_ship_index];
    }

    set_dirty();
}

void BattleshipView::send_message(const GameMessage& msg) {
    static const char* msg_strings[] = {"READY", "SHOT:", "HIT:", "MISS:", "SUNK:", "WIN"};

    std::string message = msg_strings[static_cast<int>(msg.type)];
    if (msg.type != MessageType::READY && msg.type != MessageType::WIN) {
        message += to_string_dec_uint(msg.x) + "," + to_string_dec_uint(msg.y);
    }

    configure_radio_tx();

    uint32_t target_address = is_red_team ? BLUE_TEAM_ADDRESS : RED_TEAM_ADDRESS;

    std::vector<uint32_t> codewords;
    BCHCode BCH_code{{1, 0, 1, 0, 0, 1}, 5, 31, 21, 2};

    pocsag::pocsag_encode(pocsag::MessageType::ALPHANUMERIC, BCH_code, 0, message, target_address, codewords);

    uint8_t* data_ptr = shared_memory.bb_data.data;
    size_t bi = 0;

    for (size_t i = 0; i < codewords.size(); i++) {
        uint32_t codeword = codewords[i];
        data_ptr[bi++] = (codeword >> 24) & 0xFF;
        data_ptr[bi++] = (codeword >> 16) & 0xFF;
        data_ptr[bi++] = (codeword >> 8) & 0xFF;
        data_ptr[bi++] = codeword & 0xFF;
    }

    baseband::set_fsk_data(
        codewords.size() * 32,
        2280000 / 1200,
        4500,
        64);

    transmitter_model.set_baseband_bandwidth(1750000);
    transmitter_model.enable();

    current_status = "TX: " + message;
    set_dirty();
}

void BattleshipView::on_pocsag_packet(const POCSAGPacketMessage* message) {
    if (message->packet.flag() != pocsag::NORMAL) {
        return;
    }

    pocsag_state.codeword_index = 0;
    pocsag_state.errors = 0;

    while (pocsag::pocsag_decode_batch(message->packet, pocsag_state)) {
        if (pocsag_state.out_type == pocsag::MESSAGE) {
            uint32_t expected_address = is_red_team ? RED_TEAM_ADDRESS : BLUE_TEAM_ADDRESS;
            if (pocsag_state.address == expected_address) {
                process_message(pocsag_state.output);
            }
        }
    }
}

void BattleshipView::on_tx_progress(const uint32_t progress, const bool done) {
    (void)progress;

    if (done) {
        transmitter_model.disable();
        chThdSleepMilliseconds(200);
        configure_radio_rx();

        if (game_state == GameState::MY_TURN) {
            current_status = "Waiting for response";
            set_dirty();
        }
    }
}

bool BattleshipView::parse_coords(const std::string& coords, uint8_t& x, uint8_t& y) {
    size_t comma_pos = coords.find(',');
    if (comma_pos == std::string::npos) return false;

    x = 0;
    y = 0;

    for (size_t i = 0; i < comma_pos; i++) {
        char c = coords[i];
        if (c >= '0' && c <= '9') {
            x = x * 10 + (c - '0');
        }
    }

    for (size_t i = comma_pos + 1; i < coords.length(); i++) {
        char c = coords[i];
        if (c >= '0' && c <= '9') {
            y = y * 10 + (c - '0');
        }
    }

    return x < GRID_SIZE && y < GRID_SIZE;
}

void BattleshipView::mark_ship_sunk(uint8_t x, uint8_t y, std::array<std::array<CellState, GRID_SIZE>, GRID_SIZE>& grid) {
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int check_x = x + dx;
            int check_y = y + dy;
            if (check_x >= 0 && check_x < GRID_SIZE &&
                check_y >= 0 && check_y < GRID_SIZE) {
                if (grid[check_y][check_x] == CellState::HIT) {
                    grid[check_y][check_x] = CellState::SUNK;
                }
            }
        }
    }
}

void BattleshipView::process_message(const std::string& message) {
    if (message.empty()) return;

    size_t colon_pos = message.find(':');
    std::string msg_type = (colon_pos != std::string::npos)
                               ? message.substr(0, colon_pos)
                               : message;

    if (msg_type == "READY") {
        opponent_ready = true;
        if (!is_red_team && game_state == GameState::WAITING_FOR_OPPONENT) {
            current_status = "Red ready! Waiting...";
            set_dirty();
        }
    } else if (msg_type == "SHOT" && colon_pos != std::string::npos) {
        if (game_state == GameState::OPPONENT_TURN ||
            (game_state == GameState::WAITING_FOR_OPPONENT && !is_red_team)) {
            uint8_t x, y;
            if (parse_coords(message.substr(colon_pos + 1), x, y)) {
                process_shot(x, y);
            }
        }
    } else if ((msg_type == "HIT" || msg_type == "MISS" || msg_type == "SUNK") && colon_pos != std::string::npos) {
        uint8_t x, y;
        if (parse_coords(message.substr(colon_pos + 1), x, y)) {
            if (msg_type == "HIT") {
                enemy_grid[y][x] = CellState::HIT;
                current_status = "Hit! Fire again!";
            } else if (msg_type == "MISS") {
                enemy_grid[y][x] = CellState::MISS;
                current_status = "Miss! Enemy turn";
                game_state = GameState::OPPONENT_TURN;
                button_fire.hidden(true);
                touch_enabled = false;
            } else if (msg_type == "SUNK") {
                enemy_grid[y][x] = CellState::SUNK;
                enemy_ships_remaining--;
                current_status = "Ship sunk! Fire!";
                mark_ship_sunk(x, y, enemy_grid);
            }

            if (game_state == GameState::MY_TURN) {
                touch_enabled = true;
            }
        }
    } else if (msg_type == "WIN") {
        game_state = GameState::GAME_OVER;
        current_status = (is_red_team ? "BLUE" : "RED") + std::string(" TEAM WINS!");
        button_fire.hidden(true);
        touch_enabled = false;
        losses++;
        update_score();
    }

    set_dirty();
}

void BattleshipView::fire_at_position() {
    if (game_state != GameState::MY_TURN) return;

    if (enemy_grid[target_y][target_x] != CellState::EMPTY) {
        current_status = "Already fired!";
        set_dirty();
        return;
    }

    send_message({MessageType::SHOT, target_x, target_y});
    current_status = "Firing...";
    touch_enabled = false;
    set_dirty();
}

void BattleshipView::process_shot(uint8_t x, uint8_t y) {
    if (my_grid[y][x] == CellState::SHIP) {
        my_grid[y][x] = CellState::HIT;

        bool sunk = false;

        for (auto& ship : my_ships) {
            if (!ship.placed) continue;

            bool hit_this_ship = false;
            for (uint8_t i = 0; i < ship.size(); i++) {
                uint8_t check_x = ship.horizontal ? ship.x + i : ship.x;
                uint8_t check_y = ship.horizontal ? ship.y : ship.y + i;

                if (check_x == x && check_y == y) {
                    hit_this_ship = true;
                    ship.hits++;
                    break;
                }
            }

            if (hit_this_ship && ship.is_sunk()) {
                sunk = true;
                ships_remaining--;

                for (uint8_t i = 0; i < ship.size(); i++) {
                    uint8_t sunk_x = ship.horizontal ? ship.x + i : ship.x;
                    uint8_t sunk_y = ship.horizontal ? ship.y : ship.y + i;
                    my_grid[sunk_y][sunk_x] = CellState::SUNK;
                }
                break;
            }
        }

        if (sunk) {
            send_message({MessageType::SUNK, x, y});

            if (ships_remaining == 0) {
                send_message({MessageType::WIN, 0, 0});
                game_state = GameState::GAME_OVER;
                current_status = (is_red_team ? "RED" : "BLUE") + std::string(" TEAM WINS!");
                wins++;
                update_score();
            }
        } else {
            send_message({MessageType::HIT, x, y});
        }
    } else {
        my_grid[y][x] = CellState::MISS;
        send_message({MessageType::MISS, x, y});

        game_state = GameState::MY_TURN;
        button_fire.hidden(false);
        touch_enabled = true;
        current_status = "Your turn! Fire!";
    }

    set_dirty();
}

void BattleshipView::update_score() {
    current_score = "W:" + to_string_dec_uint(wins) + " L:" + to_string_dec_uint(losses);
}

bool BattleshipView::on_touch(const TouchEvent event) {
    if (event.type != TouchEvent::Type::Start || !touch_enabled) {
        return false;
    }

    uint16_t x = event.point.x();
    uint16_t y = event.point.y();

    if (x >= GRID_OFFSET_X && x < GRID_OFFSET_X + GRID_SIZE * CELL_SIZE &&
        y >= GRID_OFFSET_Y + 5 && y < GRID_OFFSET_Y + 5 + GRID_SIZE * CELL_SIZE) {
        uint8_t grid_x = (x - GRID_OFFSET_X) / CELL_SIZE;
        uint8_t grid_y = (y - GRID_OFFSET_Y - 5) / CELL_SIZE;

        if (game_state == GameState::PLACING_SHIPS) {
            cursor_x = grid_x;
            cursor_y = grid_y;
        } else if (game_state == GameState::MY_TURN) {
            target_x = grid_x;
            target_y = grid_y;
        }
        set_dirty();
        return true;
    }

    return false;
}

bool BattleshipView::on_encoder(const EncoderEvent delta) {
    if (delta == 0) return false;

    if (game_state == GameState::PLACING_SHIPS) {
        placing_horizontal = !placing_horizontal;
    } else if (game_state == GameState::MY_TURN) {
        target_x = (delta > 0) ? (target_x + 1) % GRID_SIZE : (target_x == 0) ? GRID_SIZE - 1
                                                                              : target_x - 1;
    }
    set_dirty();
    return true;
}

bool BattleshipView::on_key(const KeyEvent key) {
    if (game_state == GameState::MENU) {
        if (key == KeyEvent::Up || key == KeyEvent::Down ||
            key == KeyEvent::Left || key == KeyEvent::Right) {
            return false;
        }
        if (key == KeyEvent::Select || key == KeyEvent::Back) {
            return false;
        }
        return false;
    }

    // Game state key handling
    if (key == KeyEvent::Select) {
        if (game_state == GameState::PLACING_SHIPS) {
            place_ship();
            return true;
        } else if (game_state == GameState::MY_TURN) {
            fire_at_position();
            return true;
        }
    } else if (key == KeyEvent::Back) {
        if (game_state != GameState::MENU) {
            reset_game();
            return true;
        }
    } else if (key == KeyEvent::Up || key == KeyEvent::Down) {
        uint8_t* coord_y = (game_state == GameState::PLACING_SHIPS) ? &cursor_y : &target_y;
        if (key == KeyEvent::Up) {
            *coord_y = (*coord_y == 0) ? GRID_SIZE - 1 : *coord_y - 1;
        } else {
            *coord_y = (*coord_y + 1) % GRID_SIZE;
        }
        set_dirty();
        return true;
    } else if (key == KeyEvent::Left || key == KeyEvent::Right) {
        uint8_t* coord_x = (game_state == GameState::PLACING_SHIPS) ? &cursor_x : &target_x;
        if (key == KeyEvent::Left) {
            *coord_x = (*coord_x == 0) ? GRID_SIZE - 1 : *coord_x - 1;
        } else {
            *coord_x = (*coord_x + 1) % GRID_SIZE;
        }
        set_dirty();
        return true;
    }

    return false;
}

}  // namespace ui::external_app::battleship
