/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#include "ui_breakout.hpp"

namespace ui::external_app::breakout {

void BreakoutView::cls() {
    painter.fill_rectangle({0, 0, portapack::display.width(), portapack::display.height()}, Color::black());
}

void BreakoutView::background(int color) {
    (void)color;
}

void BreakoutView::fillrect(int x1, int y1, int x2, int y2, int color) {
    painter.fill_rectangle({x1, y1, x2 - x1, y2 - y1}, pp_colors[color]);
}

void BreakoutView::rect(int x1, int y1, int x2, int y2, int color) {
    painter.draw_rectangle({x1, y1, x2 - x1, y2 - y1}, pp_colors[color]);
}

void BreakoutView::check_game_timer() {
    if (game_update_callback) {
        if (++game_update_counter >= game_update_timeout) {
            game_update_counter = 0;
            game_timer_check();
        }
    }
}

void BreakoutView::attach(double delay_sec) {
    game_update_callback = true;
    game_update_timeout = delay_sec * 60;
}

void BreakoutView::detach() {
    game_update_callback = false;
}

void BreakoutView::game_timer_check() {
    if (game_state == STATE_PLAYING) {
        update_game();
    } else if (game_state == STATE_MENU) {
        show_menu();
    } else if (game_state == STATE_GAME_OVER) {
        show_game_over();
    }
}

void BreakoutView::init_game() {
    SCREEN_WIDTH = screen_width;
    SCREEN_HEIGHT = screen_height;
    GAME_AREA_BOTTOM = SCREEN_HEIGHT - 10;
    PADDLE_Y = GAME_AREA_BOTTOM - PADDLE_HEIGHT;
    PADDLE_SPEED = 10;

    // Calculate brick layout
    int available_width = SCREEN_WIDTH - 4;
    BRICK_COLS = available_width / 22;
    if (BRICK_COLS > 12) BRICK_COLS = 12;
    if (BRICK_COLS < 5) BRICK_COLS = 5;
    BRICK_WIDTH = (available_width - (BRICK_COLS - 1) * BRICK_GAP) / BRICK_COLS;

    // Initialize paddle position
    paddle_x = (SCREEN_WIDTH - PADDLE_WIDTH) / 2;
    score = 0;
    lives = 3;
    level = 1;

    // Set brick colors
    brick_colors[0] = Red;
    brick_colors[1] = Orange;
    brick_colors[2] = Yellow;
    brick_colors[3] = Green;
    brick_colors[4] = Purple;

    // Initialize bricks array
    bricks.resize(BRICK_ROWS);
    for (int i = 0; i < BRICK_ROWS; i++) {
        bricks[i].resize(BRICK_COLS);
    }

    init_level();

    game_state = STATE_MENU;
    menu_blink_state = true;
    menu_blink_counter = 0;
}

void BreakoutView::init_level() {
    ball_x = paddle_x + (PADDLE_WIDTH / 2) - (BALL_SIZE / 2);
    ball_y = GAME_AREA_BOTTOM - PADDLE_HEIGHT - BALL_SIZE - 1;

    float speed_multiplier = 1.0f + ((level - 1) * BALL_SPEED_INCREASE);
    ball_dx = 1.5f * speed_multiplier;
    ball_dy = -2.0f * speed_multiplier;

    ball_attached = true;

    // Fill all bricks
    brick_count = 0;
    for (int row = 0; row < BRICK_ROWS; row++) {
        for (int col = 0; col < BRICK_COLS; col++) {
            bricks[row][col] = true;
            brick_count++;
        }
    }
}

void BreakoutView::draw_screen() {
    cls();
    background(COLOR_BACKGROUND);
    draw_borders();
    draw_bricks();
    draw_paddle();
    draw_ball();
    draw_score();
    draw_lives();
    draw_level();
}

void BreakoutView::draw_borders() {
    rect(0, GAME_AREA_TOP - 1, SCREEN_WIDTH, GAME_AREA_TOP, COLOR_BORDER);
}

void BreakoutView::draw_bricks() {
    for (int row = 0; row < BRICK_ROWS; row++) {
        for (int col = 0; col < BRICK_COLS; col++) {
            if (bricks[row][col]) {
                int x = col * (BRICK_WIDTH + BRICK_GAP);
                int y = GAME_AREA_TOP + row * (BRICK_HEIGHT + BRICK_GAP) + 5;
                fillrect(x, y, x + BRICK_WIDTH, y + BRICK_HEIGHT, brick_colors[row]);
                rect(x, y, x + BRICK_WIDTH, y + BRICK_HEIGHT, Black);
            }
        }
    }
}

void BreakoutView::draw_paddle() {
    fillrect(paddle_x, PADDLE_Y, paddle_x + PADDLE_WIDTH, PADDLE_Y + PADDLE_HEIGHT, COLOR_PADDLE);
}

void BreakoutView::draw_ball() {
    fillrect(ball_x, ball_y, ball_x + BALL_SIZE, ball_y + BALL_SIZE, COLOR_BALL);
}

void BreakoutView::draw_score() {
    auto style = *ui::Theme::getInstance()->fg_green;
    painter.draw_string({5, 10}, style, "Score: " + std::to_string(score));
}

void BreakoutView::draw_lives() {
    auto style = *ui::Theme::getInstance()->fg_red;
    painter.draw_string({5, 30}, style, "Lives: " + std::to_string(lives));
}

void BreakoutView::draw_level() {
    auto style = *ui::Theme::getInstance()->fg_yellow;
    painter.draw_string({80, 30}, style, "Level: " + std::to_string(level));
}

void BreakoutView::move_paddle_left() {
    if (paddle_x > 0) {
        fillrect(paddle_x, PADDLE_Y, paddle_x + PADDLE_WIDTH, PADDLE_Y + PADDLE_HEIGHT, COLOR_BACKGROUND);
        if (ball_attached) {
            fillrect(ball_x, ball_y, ball_x + BALL_SIZE, ball_y + BALL_SIZE, COLOR_BACKGROUND);
        }

        paddle_x -= PADDLE_SPEED;
        if (paddle_x < 0) paddle_x = 0;

        if (ball_attached) {
            ball_x = paddle_x + (PADDLE_WIDTH / 2) - (BALL_SIZE / 2);
        }

        draw_paddle();
        if (ball_attached) {
            draw_ball();
        }
    }
}

void BreakoutView::move_paddle_right() {
    if (paddle_x < SCREEN_WIDTH - PADDLE_WIDTH) {
        fillrect(paddle_x, PADDLE_Y, paddle_x + PADDLE_WIDTH, PADDLE_Y + PADDLE_HEIGHT, COLOR_BACKGROUND);
        if (ball_attached) {
            fillrect(ball_x, ball_y, ball_x + BALL_SIZE, ball_y + BALL_SIZE, COLOR_BACKGROUND);
        }

        paddle_x += PADDLE_SPEED;
        if (paddle_x > SCREEN_WIDTH - PADDLE_WIDTH) paddle_x = SCREEN_WIDTH - PADDLE_WIDTH;

        if (ball_attached) {
            ball_x = paddle_x + (PADDLE_WIDTH / 2) - (BALL_SIZE / 2);
        }

        draw_paddle();
        if (ball_attached) {
            draw_ball();
        }
    }
}

void BreakoutView::launch_ball() {
    if (ball_attached) {
        ball_attached = false;
        float speed_multiplier = 1.0f + ((level - 1) * BALL_SPEED_INCREASE);
        ball_dx = 1.5f * speed_multiplier;
        ball_dy = -2.0f * speed_multiplier;
    }
}

void BreakoutView::update_game() {
    if (ball_attached) {
        return;
    }

    fillrect(ball_x, ball_y, ball_x + BALL_SIZE, ball_y + BALL_SIZE, COLOR_BACKGROUND);

    ball_x += ball_dx;
    ball_y += ball_dy;

    // Wall collisions
    if (ball_x < 0) {
        ball_x = 0;
        ball_dx = -ball_dx;
    } else if (ball_x > SCREEN_WIDTH - BALL_SIZE) {
        ball_x = SCREEN_WIDTH - BALL_SIZE;
        ball_dx = -ball_dx;
    }

    if (ball_y < GAME_AREA_TOP) {
        ball_y = GAME_AREA_TOP;
        ball_dy = -ball_dy;
    }

    // Bottom edge - lose life
    if (ball_y > GAME_AREA_BOTTOM) {
        lives--;
        draw_lives();
        if (lives <= 0) {
            handle_game_over();
        } else {
            ball_attached = true;
            ball_x = paddle_x + (PADDLE_WIDTH / 2) - (BALL_SIZE / 2);
            ball_y = GAME_AREA_BOTTOM - PADDLE_HEIGHT - BALL_SIZE - 1;
            draw_ball();
        }
        return;
    }

    // Paddle collision
    if (ball_y + BALL_SIZE >= PADDLE_Y && ball_y <= PADDLE_Y + PADDLE_HEIGHT) {
        if (ball_x + BALL_SIZE >= paddle_x && ball_x <= paddle_x + PADDLE_WIDTH) {
            ball_y = PADDLE_Y - BALL_SIZE;
            ball_dy = -ball_dy;

            // Add some angle based on hit position
            float hit_position = (ball_x + (BALL_SIZE / 2)) - paddle_x;
            float angle = (hit_position / PADDLE_WIDTH) - 0.5f;
            ball_dx = angle * 4.0f;
        }
    }

    check_collisions();
    draw_ball();

    if (check_level_complete()) {
        next_level();
    }
}

void BreakoutView::check_collisions() {
    int grid_x = ball_x / (BRICK_WIDTH + BRICK_GAP);
    int grid_y = (ball_y - GAME_AREA_TOP - 5) / (BRICK_HEIGHT + BRICK_GAP);

    if (grid_x >= 0 && grid_x < BRICK_COLS && grid_y >= 0 && grid_y < BRICK_ROWS) {
        check_brick_collision(grid_y, grid_x);
    }
}

bool BreakoutView::check_brick_collision(int row, int col) {
    if (!bricks[row][col]) return false;

    int brick_x = col * (BRICK_WIDTH + BRICK_GAP);
    int brick_y = GAME_AREA_TOP + row * (BRICK_HEIGHT + BRICK_GAP) + 5;

    if (ball_x + BALL_SIZE >= brick_x && ball_x <= brick_x + BRICK_WIDTH &&
        ball_y + BALL_SIZE >= brick_y && ball_y <= brick_y + BRICK_HEIGHT) {
        fillrect(brick_x, brick_y, brick_x + BRICK_WIDTH, brick_y + BRICK_HEIGHT, COLOR_BACKGROUND);
        bricks[row][col] = false;
        brick_count--;

        score += (5 - row) * 10;
        draw_score();

        ball_dy = -ball_dy;
        return true;
    }

    return false;
}

bool BreakoutView::check_level_complete() {
    return brick_count == 0;
}

void BreakoutView::next_level() {
    level++;
    init_level();
    draw_screen();
}

void BreakoutView::handle_game_over() {
    game_state = STATE_GAME_OVER;
    gameover_blink_state = true;
    gameover_blink_counter = 0;
}

void BreakoutView::show_menu() {
    if (menu_blink_counter == 0) {
        cls();
        background(COLOR_BACKGROUND);

        auto style_yellow = *ui::Theme::getInstance()->fg_yellow;
        auto style_blue = *ui::Theme::getInstance()->fg_blue;
        auto style_cyan = *ui::Theme::getInstance()->fg_cyan;

        painter.draw_string({UI_POS_X_CENTER(17), 40}, style_yellow, "*** BREAKOUT ***");
        painter.draw_string({UI_POS_X_CENTER(25), 100}, style_blue, "========================");
        painter.draw_string({UI_POS_X_CENTER(20), 130}, style_cyan, "ROTARY: MOVE PADDLE");
        painter.draw_string({UI_POS_X_CENTER(21), 160}, style_cyan, "SELECT: START/LAUNCH");
        painter.draw_string({UI_POS_X_CENTER(25), 190}, style_blue, "========================");
    }

    auto style_red = *ui::Theme::getInstance()->fg_red;

    if (++menu_blink_counter >= 30) {
        menu_blink_counter = 0;
        menu_blink_state = !menu_blink_state;

        painter.fill_rectangle({UI_POS_X_CENTER(17), 228, 128, 20}, Color::black());

        if (menu_blink_state) {
            painter.draw_string({UI_POS_X_CENTER(17), 230}, style_red, "* PRESS SELECT *");
        }
    }
}

void BreakoutView::show_game_over() {
    if (gameover_blink_counter == 0) {
        cls();
        background(COLOR_BACKGROUND);

        auto style_red = *ui::Theme::getInstance()->fg_red;
        auto style_yellow = *ui::Theme::getInstance()->fg_yellow;

        painter.draw_string({UI_POS_X_CENTER(10), 100}, style_red, "GAME OVER");
        painter.draw_string({UI_POS_X_CENTER(11), 150}, style_yellow, "SCORE: " + std::to_string(score));
    }

    auto style_green = *ui::Theme::getInstance()->fg_green;

    if (++gameover_blink_counter >= 30) {
        gameover_blink_counter = 0;
        gameover_blink_state = !gameover_blink_state;

        painter.fill_rectangle({UI_POS_X_CENTER(13), 198, 96, 20}, Color::black());

        if (gameover_blink_state) {
            painter.draw_string({UI_POS_X_CENTER(13), 200}, style_green, "PRESS SELECT");
        }
    }
}

void BreakoutView::reset_game() {
    level = 1;
    score = 0;
    lives = 3;
    game_state = STATE_PLAYING;
    init_level();
    draw_screen();
}

BreakoutView::BreakoutView(NavigationView& nav)
    : nav_{nav}, bricks{} {  // Add bricks{} here
    add_children({&dummy});
    attach(1.0 / 60.0);
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
        } else if (delta < 0) {
            move_paddle_left();
        }
        set_dirty();
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
    } else if (key == KeyEvent::Left && game_state == STATE_PLAYING) {
        move_paddle_left();
    } else if (key == KeyEvent::Right && game_state == STATE_PLAYING) {
        move_paddle_right();
    }

    set_dirty();
    return true;
}

}  // namespace ui::external_app::breakout