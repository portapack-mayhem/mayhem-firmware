/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#include "ui_snake.hpp"

namespace ui::external_app::snake {

#include "snake.cpp"

SnakeView::SnakeView(NavigationView& nav)
    : nav_{nav} {
    add_children({&dummy});
    game_timer.attach(&game_timer_check, 1.0 / 5.0);
}

void SnakeView::on_show() {
}

void SnakeView::paint(Painter& painter) {
    (void)painter;
    if (!initialized) {
        initialized = true;
        std::srand(LPC_RTC->CTIME0);
        init_game();
    }
}

void SnakeView::frame_sync() {
    check_game_timer();
    set_dirty();
}

bool SnakeView::on_key(const KeyEvent key) {
    if (key == KeyEvent::Select) {
        if (game_state == STATE_MENU || game_state == STATE_GAME_OVER) {
            game_state = STATE_PLAYING;
            init_game();
        }
    } else if (game_state == STATE_PLAYING) {
        if (key == KeyEvent::Left) { if (snake_dx == 0) { snake_dx = -1; snake_dy = 0; } }
        else if (key == KeyEvent::Right) { if (snake_dx == 0) { snake_dx = 1; snake_dy = 0; } }
        else if (key == KeyEvent::Up) { if (snake_dy == 0) { snake_dx = 0; snake_dy = -1; } }
        else if (key == KeyEvent::Down) { if (snake_dy == 0) { snake_dx = 0; snake_dy = 1; } }
    }
    set_dirty();
    return true;
}

}  // namespace ui::external_app::snake