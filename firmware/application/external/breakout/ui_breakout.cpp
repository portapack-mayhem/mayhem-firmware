/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#include "ui_breakout.hpp"

namespace ui::external_app::breakout {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#include "breakout.cpp"
#pragma GCC diagnostic pop

BreakoutView::BreakoutView(NavigationView& nav)
    : nav_{nav} {
    add_children({&dummy});
    game_timer.attach(&game_timer_check, 1.0 / 60.0);
}

void BreakoutView::on_show() {
}

void BreakoutView::paint(Painter& painter) {
    (void)painter;

    if (!initialized) {
        initialized = true;
        std::srand(LPC_RTC->CTIME0);
        init_game();
    }
}

void BreakoutView::frame_sync() {
    check_game_timer();
    set_dirty();
}

bool BreakoutView::on_encoder(const EncoderEvent delta) {
    if (game_state == STATE_PLAYING) {
        if (delta > 0) {
            move_paddle_right();
            set_dirty();
        } else if (delta < 0) {
            move_paddle_left();
            set_dirty();
        }
    }
    return true;
}

bool BreakoutView::on_key(const KeyEvent key) {
    if (key == KeyEvent::Select) {
        if (game_state == STATE_MENU) {
            game_state = STATE_PLAYING;
            reset_game();
        } else if (game_state == STATE_PLAYING && ball_attached) {
            launch_ball();
        } else if (game_state == STATE_GAME_OVER) {
            reset_game();
        }
    } else if (key == KeyEvent::Left) {
        if (game_state == STATE_PLAYING) {
            move_paddle_left();
        }
    } else if (key == KeyEvent::Right) {
        if (game_state == STATE_PLAYING) {
            move_paddle_right();
        }
    }

    set_dirty();
    return true;
}

}  // namespace ui::external_app::breakout