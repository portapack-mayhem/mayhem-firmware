/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#include "ui_breakout.hpp"

namespace ui::external_app::breakout {

Ticker game_timer;

int paddle_x = (SCREEN_WIDTH - PADDLE_WIDTH) / 2;
float ball_x = SCREEN_WIDTH / 2;
float ball_y = GAME_AREA_BOTTOM - PADDLE_HEIGHT - BALL_SIZE - 1;
float ball_dx = 1.5f;
float ball_dy = -2.0f;
int score = 0;
int lives = 3;
int level = 1;
int game_state = STATE_MENU;
bool initialized = false;
bool ball_attached = true;
unsigned int brick_count = 0;

bool menu_initialized = false;
bool blink_state = true;
uint32_t blink_counter = 0;
int16_t prompt_x = 0;

bool gameover_initialized = false;
bool gameover_blink_state = true;
uint32_t gameover_blink_counter = 0;
int16_t restart_x = 0;

bool bricks[BRICK_ROWS][BRICK_COLS];
int brick_colors[BRICK_ROWS];

const Color pp_colors[] = {
    Color::white(),
    Color::blue(),
    Color::yellow(),
    Color::purple(),
    Color::green(),
    Color::red(),
    Color::magenta(),
    Color::orange(),
    Color::black(),
};

Painter painter;

bool but_RIGHT = false;
bool but_LEFT = false;
bool but_SELECT = false;

static Callback game_update_callback = nullptr;
static uint32_t game_update_timeout = 0;
static uint32_t game_update_counter = 0;

void cls() {
    painter.fill_rectangle({0, 0, portapack::display.width(), portapack::display.height()}, Color::black());
}

void background(int color) {
    (void)color;
}

void fillrect(int x1, int y1, int x2, int y2, int color) {
    painter.fill_rectangle({x1, y1, x2 - x1, y2 - y1}, pp_colors[color]);
}

void rect(int x1, int y1, int x2, int y2, int color) {
    painter.draw_rectangle({x1, y1, x2 - x1, y2 - y1}, pp_colors[color]);
}

void check_game_timer() {
    if (game_update_callback) {
        if (++game_update_counter >= game_update_timeout) {
            game_update_counter = 0;
            game_update_callback();
        }
    }
}

void Ticker::attach(Callback func, double delay_sec) {
    game_update_callback = func;
    game_update_timeout = delay_sec * 60;
}

void Ticker::detach() {
    game_update_callback = nullptr;
}

void game_timer_check() {
    if (game_state == STATE_PLAYING) {
        update_game();
    } else if (game_state == STATE_MENU) {
        show_menu();
    } else if (game_state == STATE_GAME_OVER) {
        show_game_over();
    }
}

void init_game() {
    paddle_x = (SCREEN_WIDTH - PADDLE_WIDTH) / 2;
    score = 0;
    lives = 3;
    level = 1;

    brick_colors[0] = Red;
    brick_colors[1] = Orange;
    brick_colors[2] = Yellow;
    brick_colors[3] = Green;
    brick_colors[4] = Purple;

    init_level();

    game_state = STATE_MENU;
    menu_initialized = false;
    blink_state = true;
    blink_counter = 0;
}

void init_level() {
    ball_x = paddle_x + (PADDLE_WIDTH / 2) - (BALL_SIZE / 2);
    ball_y = GAME_AREA_BOTTOM - PADDLE_HEIGHT - BALL_SIZE - 1;

    float speed_multiplier = (level == 1) ? 1.0f : 1.0f + ((level - 1) * BALL_SPEED_INCREASE);
    ball_dx = (ball_dx > 0 ? 1.5f : -1.5f) * speed_multiplier;
    ball_dy = -2.0f * speed_multiplier;

    ball_attached = true;

    brick_count = 0;
    for (int row = 0; row < BRICK_ROWS; row++) {
        for (int col = 0; col < BRICK_COLS; col++) {
            bricks[row][col] = true;
            brick_count++;
        }
    }
}

void draw_screen() {
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

void draw_borders() {
    rect(0, GAME_AREA_TOP - 1, SCREEN_WIDTH, GAME_AREA_TOP, COLOR_BORDER);
}

void draw_bricks() {
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

void draw_paddle() {
    fillrect(paddle_x, PADDLE_Y, paddle_x + PADDLE_WIDTH, PADDLE_Y + PADDLE_HEIGHT, COLOR_PADDLE);
}

void draw_ball() {
    fillrect(ball_x, ball_y, ball_x + BALL_SIZE, ball_y + BALL_SIZE, COLOR_BALL);
}

void draw_score() {
    auto style = *ui::Theme::getInstance()->fg_green;
    painter.draw_string({5, 10}, style, "Score: " + std::to_string(score));
}

void draw_lives() {
    auto style = *ui::Theme::getInstance()->fg_red;
    painter.draw_string({5, 30}, style, "Lives: " + std::to_string(lives));
}

void draw_level() {
    auto style = *ui::Theme::getInstance()->fg_yellow;
    painter.draw_string({80, 30}, style, "Level: " + std::to_string(level));
}

void move_paddle_left() {
    if (paddle_x > 0) {
        fillrect(paddle_x, PADDLE_Y, paddle_x + PADDLE_WIDTH, PADDLE_Y + PADDLE_HEIGHT, COLOR_BACKGROUND);
        if (ball_attached) {
            fillrect(ball_x, ball_y, ball_x + BALL_SIZE, ball_y + BALL_SIZE, COLOR_BACKGROUND);
        }

        paddle_x -= 10;
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

void move_paddle_right() {
    if (paddle_x < SCREEN_WIDTH - PADDLE_WIDTH) {
        fillrect(paddle_x, PADDLE_Y, paddle_x + PADDLE_WIDTH, PADDLE_Y + PADDLE_HEIGHT, COLOR_BACKGROUND);
        if (ball_attached) {
            fillrect(ball_x, ball_y, ball_x + BALL_SIZE, ball_y + BALL_SIZE, COLOR_BACKGROUND);
        }

        paddle_x += 10;
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

void launch_ball() {
    if (ball_attached) {
        ball_attached = false;
        ball_x = paddle_x + (PADDLE_WIDTH / 2) - (BALL_SIZE / 2);
        ball_y = GAME_AREA_BOTTOM - PADDLE_HEIGHT - BALL_SIZE - 1;
        float speed_multiplier = (level == 1) ? 1.0f : 1.0f + ((level - 1) * BALL_SPEED_INCREASE);
        ball_dx = 1.5f * speed_multiplier;
        ball_dy = -2.0f * speed_multiplier;
    }
}

void update_game() {
    if (ball_attached) {
        return;
    }

    fillrect(ball_x, ball_y, ball_x + BALL_SIZE, ball_y + BALL_SIZE, COLOR_BACKGROUND);

    float next_ball_y = ball_y + ball_dy;
    if (next_ball_y > GAME_AREA_BOTTOM) {
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

    ball_x += ball_dx;
    ball_y = next_ball_y;

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

    if (ball_y + BALL_SIZE >= PADDLE_Y && ball_y <= PADDLE_Y + PADDLE_HEIGHT) {
        if (ball_x + BALL_SIZE >= paddle_x && ball_x <= paddle_x + PADDLE_WIDTH) {
            ball_y = PADDLE_Y - BALL_SIZE;
            float hit_position = (ball_x + (BALL_SIZE / 2)) - paddle_x;
            float angle = (hit_position / PADDLE_WIDTH) - 0.5f;
            ball_dx = angle * 4.0f;
            if (ball_dx > -0.5f && ball_dx < 0.5f) {
                ball_dx = (ball_dx > 0) ? 0.5f : -0.5f;
            }
            ball_dy = -ball_dy;
        }
    }

    check_collisions();

    draw_ball();

    if (check_level_complete()) {
        next_level();
    }
}

void check_collisions() {
    int grid_x = ball_x / (BRICK_WIDTH + BRICK_GAP);
    int grid_y = (ball_y - GAME_AREA_TOP - 5) / (BRICK_HEIGHT + BRICK_GAP);

    for (int row = grid_y - 1; row <= grid_y + 1; row++) {
        for (int col = grid_x - 1; col <= grid_x + 1; col++) {
            if (row >= 0 && row < BRICK_ROWS && col >= 0 && col < BRICK_COLS) {
                if (bricks[row][col] && check_brick_collision(row, col)) {
                    return;
                }
            }
        }
    }
}

bool check_brick_collision(int row, int col) {
    int brick_x = col * (BRICK_WIDTH + BRICK_GAP);
    int brick_y = GAME_AREA_TOP + row * (BRICK_HEIGHT + BRICK_GAP) + 5;

    if (ball_x + BALL_SIZE >= brick_x && ball_x <= brick_x + BRICK_WIDTH &&
        ball_y + BALL_SIZE >= brick_y && ball_y <= brick_y + BRICK_HEIGHT) {
        fillrect(brick_x, brick_y, brick_x + BRICK_WIDTH, brick_y + BRICK_HEIGHT, COLOR_BACKGROUND);

        bricks[row][col] = false;
        brick_count--;

        score += (5 - row) * 10;
        draw_score();

        float center_x = brick_x + BRICK_WIDTH / 2;
        float center_y = brick_y + BRICK_HEIGHT / 2;
        float ball_center_x = ball_x + BALL_SIZE / 2;
        float ball_center_y = ball_y + BALL_SIZE / 2;
        float dx = std::abs(ball_center_x - center_x);
        float dy = std::abs(ball_center_y - center_y);

        if (dx * BRICK_HEIGHT > dy * BRICK_WIDTH) {
            ball_dx = -ball_dx;
        } else {
            ball_dy = -ball_dy;
        }

        return true;
    }

    return false;
}

bool check_level_complete() {
    return brick_count == 0;
}

void next_level() {
    level++;
    init_level();
    draw_screen();
}

void handle_game_over() {
    game_state = STATE_GAME_OVER;
    gameover_initialized = false;
    show_game_over();
}

void init_menu() {
    cls();
    background(COLOR_BACKGROUND);

    auto style_yellow = *ui::Theme::getInstance()->fg_yellow;
    auto style_blue = *ui::Theme::getInstance()->fg_blue;
    auto style_cyan = *ui::Theme::getInstance()->fg_cyan;

    int16_t screen_width = 240;
    int16_t title_x = (screen_width - 17 * 8) / 2;
    int16_t divider_width = 24 * 8;
    int16_t divider_x = (screen_width - divider_width) / 2;
    int16_t instruction_width = 22 * 8;
    int16_t instruction_x = (screen_width - instruction_width) / 2;
    int16_t prompt_width = 16 * 8;
    prompt_x = (screen_width - prompt_width) / 2;

    painter.fill_rectangle({0, 30, screen_width, 30}, Color::black());
    painter.draw_string({title_x + 2, 42}, style_yellow, "*** BREAKOUT ***");
    painter.draw_string({divider_x, 70}, style_blue, "========================");
    painter.fill_rectangle({instruction_x - 5, 110, instruction_width + 10, 70}, Color::black());
    painter.draw_rectangle({instruction_x - 5, 110, instruction_width + 10, 70}, Color::white());
    painter.draw_string({instruction_x, 120}, style_cyan, "  ROTARY: MOVE PADDLE");
    painter.draw_string({instruction_x, 150}, style_cyan, " SELECT: START/LAUNCH");
    painter.draw_string({divider_x, 190}, style_blue, "========================");

    menu_initialized = true;
}

void show_menu() {
    if (!menu_initialized) {
        init_menu();
    }

    auto style_red = *ui::Theme::getInstance()->fg_red;

    if (++blink_counter >= 30) {
        blink_counter = 0;
        blink_state = !blink_state;

        painter.fill_rectangle({prompt_x - 2, 228, 16 * 8 + 4, 20}, Color::black());

        if (blink_state) {
            painter.draw_string({prompt_x, 230}, style_red, "* PRESS SELECT *");
        }
    }
}

void init_game_over() {
    cls();
    background(COLOR_BACKGROUND);

    auto style_red = *ui::Theme::getInstance()->fg_red;
    auto style_yellow = *ui::Theme::getInstance()->fg_yellow;

    int16_t screen_width = 240;
    int16_t title_width = 9 * 8;
    int16_t title_x = (screen_width - title_width) / 2;
    int16_t score_text_width = (16 + std::to_string(score).length()) * 8;
    int16_t score_x = (screen_width - score_text_width) / 2;

    painter.draw_rectangle({20, 80, screen_width - 40, 160}, Color::red());
    painter.draw_rectangle({22, 82, screen_width - 44, 156}, Color::white());

    painter.draw_string({title_x, 100}, style_red, "GAME OVER");

    painter.fill_rectangle({40, 140, screen_width - 80, 30}, Color::black());
    painter.draw_rectangle({40, 140, screen_width - 80, 30}, Color::yellow());
    painter.draw_string({score_x, 150}, style_yellow, " FINAL SCORE: " + std::to_string(score));

    int16_t restart_width = 12 * 8;
    restart_x = (screen_width - restart_width) / 2;

    gameover_initialized = true;
    gameover_blink_state = true;
    gameover_blink_counter = 0;
}

void show_game_over() {
    if (!gameover_initialized) {
        init_game_over();
    }

    auto style_green = *ui::Theme::getInstance()->fg_green;

    if (++gameover_blink_counter >= 30) {
        gameover_blink_counter = 0;
        gameover_blink_state = !gameover_blink_state;

        painter.fill_rectangle({restart_x - 2, 198, 12 * 8 + 4, 20}, Color::black());

        if (gameover_blink_state) {
            painter.draw_string({restart_x, 200}, style_green, "PRESS SELECT");
        }
    }
}

void reset_game() {
    level = 1;
    score = 0;
    lives = 3;
    game_state = STATE_PLAYING;
    init_level();
    draw_screen();
    gameover_initialized = false;
    gameover_blink_state = true;
    gameover_blink_counter = 0;
}

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
    but_SELECT = (key == KeyEvent::Select);
    but_LEFT = (key == KeyEvent::Left);
    but_RIGHT = (key == KeyEvent::Right);

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