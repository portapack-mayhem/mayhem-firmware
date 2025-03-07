/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#include "mbed.h"
#include "SPI_TFT_ILI9341.h"
#include "Arial12x12.h"
#include "ui.hpp"

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define PADDLE_WIDTH 40
#define PADDLE_HEIGHT 10
#define BALL_SIZE 8
#define BRICK_WIDTH 20
#define BRICK_HEIGHT 10
#define BRICK_ROWS 5
#define BRICK_COLS 10
#define BRICK_GAP 2
#define GAME_AREA_TOP 50
#define GAME_AREA_BOTTOM 310
#define PADDLE_Y (GAME_AREA_BOTTOM - PADDLE_HEIGHT)
#define BALL_SPEED_INCREASE 0.1f

#define STATE_MENU 0
#define STATE_PLAYING 1
#define STATE_GAME_OVER 3

#define COLOR_BACKGROUND Black
#define COLOR_PADDLE Blue
#define COLOR_BALL White
#define COLOR_BORDER White
#define COLOR_BRICK_COLORS \
    { Red, Orange, Yellow, Green, Purple }

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

bool bricks[BRICK_ROWS][BRICK_COLS];
int brick_colors[BRICK_ROWS];

extern ui::Painter painter;

void init_game();
void init_level();
void draw_screen();
void draw_bricks();
void draw_paddle();
void draw_ball();
void draw_score();
void draw_lives();
void draw_level();
void draw_borders();
void move_paddle_left();
void move_paddle_right();
void launch_ball();
void update_game();
void check_collisions();
bool check_brick_collision(int row, int col);
void handle_game_over();
void show_menu();
void show_game_over();
bool check_level_complete();
void next_level();
void reset_game();

void game_timer_check() {
    if (game_state == STATE_PLAYING) {
        update_game();
    }
}

void init_game() {
    claim(stdout);
    set_orientation(2);
    set_font((unsigned char*)Arial12x12);

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
    show_menu();
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
    show_game_over();
}

void show_menu() {
    cls();
    background(COLOR_BACKGROUND);

    auto style_yellow = *ui::Theme::getInstance()->fg_yellow;
    auto style_white = *ui::Theme::getInstance()->fg_light;
    auto style_green = *ui::Theme::getInstance()->fg_green;
    auto style_red = *ui::Theme::getInstance()->fg_red;

    painter.draw_string({0, 40}, style_yellow, "* * * BREAKOUT * * *");
    painter.draw_string({0, 70}, style_white, "========================");
    painter.draw_string({0, 120}, style_green, "| ROTARY: MOVE PADDLE  |");
    painter.draw_string({0, 150}, style_green, "| SELECT: START/LAUNCH |");
    painter.draw_string({0, 190}, style_white, "========================");
    painter.draw_string({24, 230}, style_red, "* PRESS SELECT *");
}

void show_game_over() {
    cls();
    background(COLOR_BACKGROUND);

    auto style_red = *ui::Theme::getInstance()->fg_red;
    auto style_yellow = *ui::Theme::getInstance()->fg_yellow;
    auto style_green = *ui::Theme::getInstance()->fg_green;

    painter.draw_string({72, 120}, style_red, "GAME OVER");
    painter.draw_string({12, 160}, style_yellow, "FINAL SCORE: " + std::to_string(score));
    painter.draw_string({0, 200}, style_green, "PRESS SELECT TO RESTART");

    wait(1);
}

void reset_game() {
    level = 1;
    score = 0;
    lives = 3;
    game_state = STATE_PLAYING;
    init_level();
    draw_screen();
}

int main() {
    if (!initialized) {
        initialized = true;
        game_timer.attach(&game_timer_check, 1.0 / 60.0);
        init_game();
    }

    while (1) {
        if (but_SELECT && game_state == STATE_MENU) {
            game_state = STATE_PLAYING;
            reset_game();
        }

        if (but_SELECT && game_state == STATE_GAME_OVER) {
            reset_game();
        }

        if (but_SELECT && game_state == STATE_PLAYING && ball_attached) {
            launch_ball();
        }

        if (but_LEFT && game_state == STATE_PLAYING) {
            move_paddle_left();
        }

        if (but_RIGHT && game_state == STATE_PLAYING) {
            move_paddle_right();
        }
    }
}