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
 
 using namespace portapack;
 using namespace modems;
 
 namespace ui::external_app::battleship {
 
 BattleshipView::BattleshipView(NavigationView& nav)
     : nav_{nav} {
     
     baseband::run_prepared_image(portapack::memory::map::m4_code.base());
     
     add_children({
         &rssi,
         &field_frequency,
         &text_status,
         &text_score,
         &button_red_team,
         &button_blue_team,
         &button_rotate,
         &button_place,
         &button_fire,
         &button_menu
     });
     
     text_score.hidden(true);
     button_rotate.hidden(true);
     button_place.hidden(true);
     button_fire.hidden(true);
     button_menu.hidden(true);
     
     field_frequency.set_value(DEFAULT_FREQUENCY);
     field_frequency.on_change = [this](rf::Frequency freq) {
         tx_frequency = freq;
         rx_frequency = freq;
         if (!is_transmitting) {
             receiver_model.set_target_frequency(rx_frequency);
         }
     };
     
     button_red_team.on_select = [this](Button&) {
         start_team(true);
     };
     
     button_blue_team.on_select = [this](Button&) {
         start_team(false);
     };
     
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
         button_red_team.focus();
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
     rx_buffer.clear();
     cursor_x = 0;
     cursor_y = 0;
     target_x = 0;
     target_y = 0;
     is_transmitting = false;
     in_message = false;
     current_message.clear();
     
     init_game();
     
     current_status = "Choose your team!";
     update_score();
     text_status.hidden(false);
     text_score.hidden(true);
     field_frequency.hidden(false);
     button_red_team.hidden(false);
     button_blue_team.hidden(false);
     button_rotate.hidden(true);
     button_place.hidden(true);
     button_fire.hidden(true);
     button_menu.hidden(true);
     
     button_red_team.set_focusable(true);
     button_blue_team.set_focusable(true);
     button_red_team.focus();
     
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
     
     field_frequency.hidden(true);
     button_red_team.hidden(true);
     button_blue_team.hidden(true);
     button_rotate.hidden(false);
     button_place.hidden(false);
     button_menu.hidden(false);
     
     text_status.hidden(true);
     text_score.hidden(true);
     
     current_status = "Place carrier (5)";
     
     button_rotate.set_focusable(false);
     button_place.set_focusable(false);
     button_menu.set_focusable(false);
     
     focus();
     configure_radio(false);
     set_dirty();
 }
 
 void BattleshipView::configure_radio(bool tx) {
     if (tx) {
         receiver_model.disable();
         audio::output::stop();
         
         baseband::shutdown();
         chThdSleepMilliseconds(100);
         
         baseband::run_image(portapack::spi_flash::image_tag_t{'P', 'F', 'S', 'K'});
         chThdSleepMilliseconds(100);
         
         transmitter_model.set_target_frequency(tx_frequency);
         transmitter_model.set_sampling_rate(2280000);
         transmitter_model.set_baseband_bandwidth(1750000);
         transmitter_model.set_rf_amp(false);
         transmitter_model.set_tx_gain(35);
         
         is_transmitting = true;
     } else {
         if (is_transmitting) {
             transmitter_model.disable();
             baseband::shutdown();
             chThdSleepMilliseconds(100);
             
             baseband::run_image(portapack::spi_flash::image_tag_t{'P', 'F', 'S', 'R'});
             chThdSleepMilliseconds(100);
         }
         
         receiver_model.set_target_frequency(rx_frequency);
         receiver_model.set_sampling_rate(2280000);
         receiver_model.set_baseband_bandwidth(1750000);
         receiver_model.set_rf_amp(false);
         receiver_model.set_lna(24);
         receiver_model.set_vga(24);
         receiver_model.enable();
         
         baseband::set_fsk(
             FSK_BAUDRATE, FSK_DEVIATION, 0x7E7E7E7E, 4, 0xAAAAAAAA, 8, 64
         );
         
         audio::set_rate(audio::Rate::Hz_24000);
         audio::output::start();
         is_transmitting = false;
         
         current_status = is_transmitting ? "Switched to RX" : "RX Ready";
         set_dirty();
     }
 }
 
 void BattleshipView::paint(Painter& painter) {
     painter.fill_rectangle({0, 0, 240, 320}, Color::black());
     
     if (game_state == GameState::MENU) {
         auto style_title = *ui::Theme::getInstance()->fg_light;
         painter.draw_string({60, 20}, style_title, "BATTLESHIP");
         painter.draw_string({40, 80}, style_title, "Choose your team:");
         painter.draw_string({10, 180}, *ui::Theme::getInstance()->fg_medium, "Set same freq on both!");
         return;
     }
     
     Color team_color = is_red_team ? Color::red() : Color::blue();
     painter.fill_rectangle({0, 5, 240, 16}, team_color);
     auto style_white = Style{
         .font = ui::font::fixed_8x16,
         .background = team_color,
         .foreground = Color::white()
     };
     painter.draw_string({85, 5}, style_white, is_red_team ? "RED TEAM" : "BLUE TEAM");
     
     auto style_status = *ui::Theme::getInstance()->fg_light;
     painter.fill_rectangle({0, 21, 240, 16}, Color::black());
     painter.draw_string({10, 21}, style_status, current_status);
     
     if (game_state != GameState::MENU) {
         painter.draw_string({170, 21}, style_status, current_score);
     }
     
     if (game_state == GameState::PLACING_SHIPS) {
         draw_grid(painter, GRID_OFFSET_X, GRID_OFFSET_Y + 5, my_grid, true);
         if (current_ship_index < 5) {
             draw_ship_preview(painter);
         }
     } else if (game_state == GameState::MY_TURN) {
         draw_grid(painter, GRID_OFFSET_X, GRID_OFFSET_Y + 5, enemy_grid, false, true);
         painter.draw_string({10, GRID_OFFSET_Y + GRID_SIZE * CELL_SIZE + 10}, style_status, 
                            "Enemy ships: " + to_string_dec_uint(enemy_ships_remaining));
     } else if (game_state == GameState::OPPONENT_TURN || game_state == GameState::WAITING_FOR_OPPONENT) {
         draw_grid(painter, GRID_OFFSET_X, GRID_OFFSET_Y + 5, my_grid, true);
         painter.draw_string({10, GRID_OFFSET_Y + GRID_SIZE * CELL_SIZE + 10}, style_status, 
                            "Your ships: " + to_string_dec_uint(ships_remaining));
     } else if (game_state == GameState::GAME_OVER) {
         painter.draw_string({50, 150}, style_status, "Game Over!");
         painter.draw_string({30, 170}, style_status, current_status);
     }
 }
 
 void BattleshipView::draw_grid(Painter& painter, uint8_t grid_x, uint8_t grid_y,
                                const std::array<std::array<CellState, GRID_SIZE>, GRID_SIZE>& grid,
                                bool show_ships, bool is_offense_grid) {
     painter.fill_rectangle({grid_x, grid_y, GRID_SIZE * CELL_SIZE, GRID_SIZE * CELL_SIZE}, 
                           Color::dark_blue());
     
     for (uint8_t i = 0; i <= GRID_SIZE; i++) {
         painter.draw_vline({grid_x + i * CELL_SIZE, grid_y}, 
                           GRID_SIZE * CELL_SIZE, Color::grey());
         painter.draw_hline({grid_x, grid_y + i * CELL_SIZE}, 
                           GRID_SIZE * CELL_SIZE, Color::grey());
     }
     
     for (uint8_t y = 0; y < GRID_SIZE; y++) {
         for (uint8_t x = 0; x < GRID_SIZE; x++) {
             draw_cell(painter, grid_x + x * CELL_SIZE + 1, grid_y + y * CELL_SIZE + 1,
                      grid[y][x], show_ships, is_offense_grid,
                      is_cursor_at(x, y, is_offense_grid));
         }
     }
 }
 
 void BattleshipView::draw_cell(Painter& painter, uint8_t cell_x, uint8_t cell_y,
                                CellState state, bool show_ships, bool is_offense_grid, bool is_cursor) {
     Color cell_color = Color::dark_blue();
     bool should_fill = false;
     
     if (game_state == GameState::PLACING_SHIPS && !is_offense_grid && current_ship_index < 5) {
         uint8_t ship_size = my_ships[current_ship_index].size();
         for (uint8_t i = 0; i < ship_size; i++) {
             uint8_t preview_x = placing_horizontal ? cursor_x + i : cursor_x;
             uint8_t preview_y = placing_horizontal ? cursor_y : cursor_y + i;
             uint8_t grid_x = (cell_x - 1) / CELL_SIZE;
             uint8_t grid_y = (cell_y - GRID_OFFSET_Y - 6) / CELL_SIZE;
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
             uint8_t cell_x = GRID_OFFSET_X + x * CELL_SIZE + 1;
             uint8_t cell_y = GRID_OFFSET_Y + 5 + y * CELL_SIZE + 1;
             
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
     
     configure_radio(true);
     
     std::vector<uint8_t> tx_data;
     tx_data.reserve(32);  // Pre-allocate
     
     // Preamble
     for (int i = 0; i < 8; i++) {
         tx_data.push_back(0xAA);
     }
     
     // Sync word
     tx_data.push_back(0x7E);
     tx_data.push_back(0x7E);
     tx_data.push_back(0x7E);
     tx_data.push_back(0x7E);
     
     // Message with markers
     tx_data.push_back(0x02); // STX
     for (char c : message) {
         tx_data.push_back(static_cast<uint8_t>(c));
     }
     tx_data.push_back(0x03); // ETX
     
     // Copy to shared memory
     size_t len = tx_data.size();
     for (size_t i = 0; i < len && i < 256; i++) {
         shared_memory.bb_data.data[i] = tx_data[i];
     }
     
     baseband::set_fsk_data(
         len * 8,                 
         2280000 / FSK_BAUDRATE,  
         FSK_DEVIATION,           
         32                       
     );
     
     transmitter_model.enable();
     
     current_status = "TX: " + message;
     set_dirty();
 }
 
 void BattleshipView::parse_fsk_data(const FskPacketData& packet) {
     if (packet.dataLen > 0) {
         current_status = "RX " + to_string_dec_uint(packet.dataLen) + "b @ " + 
                         to_string_dec_int(packet.max_dB) + "dB";
         set_dirty();
     }
     
     for (uint16_t i = 0; i < packet.dataLen; i++) {
         uint8_t byte = packet.data[i];
         
         if (byte == 0x02) {  // STX
             in_message = true;
             current_message.clear();
         } else if (byte == 0x03 && in_message) {  // ETX
             in_message = false;
             if (!current_message.empty()) {
                 process_message(current_message);
                 current_message.clear();
             }
         } else if (in_message && current_message.length() < 50) {
             if ((byte >= 32 && byte < 127) || byte == '\n' || byte == '\r') {
                 current_message += static_cast<char>(byte);
             }
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
     } else if ((msg_type == "HIT" || msg_type == "MISS" || msg_type == "SUNK") 
                && colon_pos != std::string::npos) {
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
         target_x = (delta > 0) ? (target_x + 1) % GRID_SIZE : 
                    (target_x == 0) ? GRID_SIZE - 1 : target_x - 1;
     }
     set_dirty();
     return true;
 }
 
 bool BattleshipView::on_key(const KeyEvent key) {
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
 
 }
 