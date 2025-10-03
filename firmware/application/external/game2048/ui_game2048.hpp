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

#ifndef __UI_GAME2048_H__
#define __UI_GAME2048_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "event_m0.hpp"
#include "message.hpp"
#include "irq_controls.hpp"
#include "lpc43xx_cpp.hpp"
#include "ui_widget.hpp"

namespace ui::external_app::game2048 {

#define GRID_SIZE 4
#define TILE_MARGIN 5
#define BOARD_START_X 15
#define BOARD_START_Y 35
#define INFO_HEIGHT 30

enum GameState {
    STATE_PLAYING,
    STATE_GAME_OVER,
    STATE_WON
};

class Game2048View : public View {
   public:
    Game2048View(NavigationView& nav);
    void on_show() override;

    std::string title() const override { return "2048"; }

    void focus() override { dummy.focus(); }
    void paint(Painter& painter) override;
    void frame_sync();
    bool on_key(KeyEvent key) override;
    bool on_encoder(const EncoderEvent event) override;

   private:
    NavigationView& nav_;
    Painter painter{};

    int grid[GRID_SIZE][GRID_SIZE];
    int TILE_SIZE = 50;
    int score;
    int best_score;
    GameState game_state;
    bool initialized = false;
    bool need_repaint = true;
    bool need_repaint_bg_frame = true;  // drar bg frame is too flickering, so make it paint as less as possible
    bool need_set_dirty = false;
    uint32_t load_wait_time_counter = 0;                  // the paint timing is always bad at app enterences,
                                                          // in all apps it behave like this.
                                                          // this is a buffer timer to let it flash a time until app loaded.
    uint32_t show_you_won_you_lost_slow_down_cunter = 0;  // slow down otherwies it repaint even faster than it finished paint fulll stuff

    void init_game();
    void reset_game();
    void add_random_tile();
    bool move_left();
    bool move_right();
    bool move_up();
    bool move_down();
    bool can_move();
    bool is_game_won();
    void draw_game();
    void draw_tile(int x, int y, int value);
    void draw_score();
    void show_game_over();
    void show_you_won();
    Color get_tile_color(int value);
    Color get_text_color(int value);
    void compress_left(int row[]);
    void merge_left(int row[]);
    bool move_row_left(int row[]);
    void rotate_grid_clockwise();
    void rotate_grid_counter_clockwise();

    Button dummy{
        {screen_width, 0, 0, 0},
        ""};

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->frame_sync();
        }};
};

}  // namespace ui::external_app::game2048

#endif /* __UI_GAME2048_H__ */