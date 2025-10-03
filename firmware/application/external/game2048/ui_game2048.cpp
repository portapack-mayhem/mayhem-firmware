/*
 * copyleft 2025 zxkmm AKA zix aka sommermorgentraum
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

#include "ui_game2048.hpp"
#include "string_format.hpp"

namespace ui::external_app::game2048 {

Game2048View::Game2048View(NavigationView& nav)
    : nav_(nav), score(0), best_score(0), game_state(STATE_PLAYING) {
    add_children({&dummy});
    TILE_SIZE = (screen_width - 2 * BOARD_START_X - (GRID_SIZE + 1) * TILE_MARGIN) / GRID_SIZE;
    init_game();
}

void Game2048View::on_show() {
    if (!initialized) {
        std::srand(LPC_RTC->CTIME0);
        reset_game();
        initialized = true;
    }
}

void Game2048View::init_game() {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j] = 0;
        }
    }
}

void Game2048View::reset_game() {
    need_repaint = true;
    init_game();
    score = 0;
    game_state = STATE_PLAYING;
    add_random_tile();
    add_random_tile();
}

void Game2048View::add_random_tile() {
    std::vector<std::pair<int, int>> empty_cells;

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (grid[i][j] == 0) {
                empty_cells.push_back(std::make_pair(i, j));
            }
        }
    }

    if (!empty_cells.empty()) {
        auto& random_cell = empty_cells[rand() % empty_cells.size()];
        grid[random_cell.first][random_cell.second] = (rand() % 10 == 0) ? 4 : 2;
    }
}

void Game2048View::compress_left(int row[]) {
    int temp[GRID_SIZE];
    int index = 0;

    for (int i = 0; i < GRID_SIZE; i++) {
        temp[i] = 0;
    }

    for (int i = 0; i < GRID_SIZE; i++) {
        if (row[i] != 0) {
            temp[index++] = row[i];
        }
    }

    for (int i = 0; i < GRID_SIZE; i++) {
        row[i] = temp[i];
    }
}

void Game2048View::merge_left(int row[]) {
    for (int i = 0; i < GRID_SIZE - 1; i++) {
        if (row[i] != 0 && row[i] == row[i + 1]) {
            row[i] *= 2;
            score += row[i];
            row[i + 1] = 0;
        }
    }
}

bool Game2048View::move_row_left(int row[]) {
    int original[GRID_SIZE];
    for (int i = 0; i < GRID_SIZE; i++) {
        original[i] = row[i];
    }

    compress_left(row);
    merge_left(row);
    compress_left(row);

    for (int i = 0; i < GRID_SIZE; i++) {
        if (original[i] != row[i]) {
            return true;
        }
    }
    return false;
}

void Game2048View::rotate_grid_clockwise() {
    int temp[GRID_SIZE][GRID_SIZE];

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            temp[j][GRID_SIZE - 1 - i] = grid[i][j];
        }
    }

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j] = temp[i][j];
        }
    }
}

void Game2048View::rotate_grid_counter_clockwise() {
    int temp[GRID_SIZE][GRID_SIZE];

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            temp[GRID_SIZE - 1 - j][i] = grid[i][j];
        }
    }

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j] = temp[i][j];
        }
    }
}

bool Game2048View::move_left() {
    bool moved = false;
    for (int i = 0; i < GRID_SIZE; i++) {
        if (move_row_left(grid[i])) {
            moved = true;
        }
    }
    return moved;
}

bool Game2048View::move_right() {
    rotate_grid_clockwise();
    rotate_grid_clockwise();
    bool moved = move_left();
    rotate_grid_clockwise();
    rotate_grid_clockwise();
    return moved;
}

bool Game2048View::move_up() {
    rotate_grid_counter_clockwise();
    bool moved = move_left();
    rotate_grid_clockwise();
    return moved;
}

bool Game2048View::move_down() {
    rotate_grid_clockwise();
    bool moved = move_left();
    rotate_grid_counter_clockwise();
    return moved;
}

bool Game2048View::can_move() {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (grid[i][j] == 0) return true;

            if (j < GRID_SIZE - 1 && grid[i][j] == grid[i][j + 1]) return true;
            if (i < GRID_SIZE - 1 && grid[i][j] == grid[i + 1][j]) return true;
        }
    }
    return false;
}

bool Game2048View::is_game_won() {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (grid[i][j] == 2048) {
                return true;
            }
        }
    }
    return false;
}

Color Game2048View::get_tile_color(int value) {
    switch (value) {
        case 0:
            return Color::light_grey();
        case 2:
            return Color::white();
        case 4:
            return Color::yellow();
        case 8:
            return Color::orange();
        case 16:
            return Color::red();
        case 32:
            return Color::magenta();
        case 64:
            return Color::purple();
        case 128:
            return Color::blue();
        case 256:
            return Color::cyan();
        case 512:
            return Color::green();
        case 1024:
            return Color::dark_green();
        case 2048:
            return Color::dark_red();
        default:
            return Color::black();
    }
}

Color Game2048View::get_text_color(int value) {
    if (value <= 4) {
        return Color::black();
    }
    return Color::white();
}

void Game2048View::draw_tile(int x, int y, int value) {
    int tile_x = BOARD_START_X + x * (TILE_SIZE + TILE_MARGIN);
    int tile_y = BOARD_START_Y + y * (TILE_SIZE + TILE_MARGIN);

    Color tile_color = get_tile_color(value);

    painter.fill_rectangle({tile_x, tile_y, TILE_SIZE, TILE_SIZE}, tile_color);
    painter.draw_rectangle({tile_x, tile_y, TILE_SIZE, TILE_SIZE}, Color::dark_grey());

    if (value > 0) {
        std::string value_str = to_string_dec_uint(value);
        int text_width = value_str.length() * 8;
        int text_height = 16;
        int text_x = tile_x + (TILE_SIZE - text_width) / 2;
        int text_y = tile_y + (TILE_SIZE - text_height) / 2;

        painter.draw_string({text_x, text_y}, *Theme::getInstance()->bg_darkest, value_str);
    }
}

void Game2048View::draw_score() {
    painter.fill_rectangle({0, 0, screen_width, INFO_HEIGHT}, Color::black());

    std::string score_str = "Score: " + to_string_dec_uint(score);
    painter.draw_string({10, 5}, *Theme::getInstance()->bg_darkest, score_str);

    if (best_score > 0) {
        std::string best_str = "Best: " + to_string_dec_uint(best_score);
        painter.draw_string({150, 5}, *Theme::getInstance()->bg_darkest, best_str);
    }
}

void Game2048View::draw_game() {
    // the paint timing is always bad at app enterences,
    // in all apps it behave like this.
    // this is a buffer timer to let it flash a time until app loaded.
    if (!need_repaint && load_wait_time_counter++ < 40000) {
        return;
    }
    if (need_repaint_bg_frame) painter.fill_rectangle({0, INFO_HEIGHT, screen_width, screen_height - INFO_HEIGHT}, Color::dark_grey());

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            draw_tile(j, i, grid[i][j]);
        }
    }

    draw_score();

    need_repaint = false;
    need_repaint_bg_frame = false;
}

void Game2048View::show_game_over() {
    if (show_you_won_you_lost_slow_down_cunter++ % 1000 != 0) return;
    // slow down otherwies it repaint even faster than it finished paint full stuff
    painter.fill_rectangle({UI_POS_X_CENTER(20), 100, 160, 100}, Color::black());
    painter.draw_rectangle({UI_POS_X_CENTER(20), 100, 160, 100}, Color::white());
    painter.draw_string({UI_POS_X_CENTER(10), 115}, *Theme::getInstance()->bg_darkest, "GAME OVER");
    painter.draw_string({UI_POS_X_CENTER(13), 135}, *Theme::getInstance()->bg_darkest, "Press SELECT");
    painter.draw_string({UI_POS_X_CENTER(11), 155}, *Theme::getInstance()->bg_darkest, "to restart");
    need_repaint = false;
    need_repaint_bg_frame = true;
}

void Game2048View::show_you_won() {
    if (show_you_won_you_lost_slow_down_cunter++ % 1000 != 0) return;
    // slow down otherwies it repaint even faster than it finished paint full stuff
    painter.fill_rectangle({UI_POS_X_CENTER(20), 100, 160, 100}, Color::black());
    painter.draw_rectangle({UI_POS_X_CENTER(20), 100, 160, 100}, Color::white());
    painter.draw_string({UI_POS_X_CENTER(9), 115}, *Theme::getInstance()->bg_darkest, "YOU WON!");
    painter.draw_string({UI_POS_X_CENTER(13), 135}, *Theme::getInstance()->bg_darkest, "Press SELECT");
    painter.draw_string({UI_POS_X_CENTER(11), 155}, *Theme::getInstance()->bg_darkest, "to restart");
    need_repaint = false;
    need_repaint_bg_frame = true;
}

void Game2048View::paint(Painter&) {
    draw_game();

    if (game_state == STATE_GAME_OVER) {
        need_repaint = true;
        need_repaint_bg_frame = true;
        show_game_over();
    } else if (game_state == STATE_WON) {
        need_repaint = true;
        need_repaint_bg_frame = true;
        show_you_won();
    }
}

void Game2048View::frame_sync() {
    draw_game();

    if (game_state == STATE_GAME_OVER) {
        show_game_over();
    } else if (game_state == STATE_WON) {
        show_you_won();
    }
}

bool Game2048View::on_key(KeyEvent key) {
    if (key == KeyEvent::Select) {
        if (game_state == STATE_GAME_OVER || game_state == STATE_WON) {
            reset_game();
            return true;
        }
    }

    if (game_state != STATE_PLAYING) {
        return false;
    }

    bool moved = false;

    switch (key) {
        case KeyEvent::Left:
            moved = move_left();
            break;
        case KeyEvent::Right:
            moved = move_right();
            break;
        case KeyEvent::Up:
            moved = move_up();
            break;
        case KeyEvent::Down:
            moved = move_down();
            break;
        default:
            return false;
    }

    if (moved) {
        need_repaint = true;
        add_random_tile();

        if (score > best_score) {
            best_score = score;
        }

        if (is_game_won() && game_state == STATE_PLAYING) {
            game_state = STATE_WON;
        } else if (!can_move()) {
            game_state = STATE_GAME_OVER;
        }
    }

    return true;
}

bool Game2048View::on_encoder(const EncoderEvent) {
    if (game_state != STATE_PLAYING) {
        return false;
    }

    bool moved = false;

    if (moved) {
        add_random_tile();

        if (score > best_score) {
            best_score = score;
        }

        if (is_game_won() && game_state == STATE_PLAYING) {
            game_state = STATE_WON;
        } else if (!can_move()) {
            game_state = STATE_GAME_OVER;
        }
    }

    return true;
}

}  // namespace ui::external_app::game2048