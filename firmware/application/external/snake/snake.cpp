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
#include "random.hpp"

extern int game_state;

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define SNAKE_SIZE 10
#define INFO_BAR_HEIGHT 25
#define GAME_AREA_TOP (INFO_BAR_HEIGHT + 1)
#define GAME_AREA_HEIGHT (SCREEN_HEIGHT - INFO_BAR_HEIGHT - 2)
#define GRID_WIDTH ((SCREEN_WIDTH - 2) / SNAKE_SIZE)
#define GRID_HEIGHT (GAME_AREA_HEIGHT / SNAKE_SIZE)
#define STATE_MENU 0
#define STATE_PLAYING 1
#define STATE_GAME_OVER 2

#define COLOR_BACKGROUND Black
#define COLOR_SNAKE Green
#define COLOR_FOOD Red
#define COLOR_BORDER White

Ticker game_timer;

int snake_x[GRID_WIDTH * GRID_HEIGHT];
int snake_y[GRID_WIDTH * GRID_HEIGHT];
int snake_length = 1;
int snake_dx = 1, snake_dy = 0;
int food_x, food_y;
int score = 0;
int game_state = STATE_MENU;
bool initialized = false;

extern ui::Painter painter;

void init_game();
void update_game();
void draw_screen();
void draw_snake();
void draw_full_snake();
void erase_tail(int x, int y);
void draw_food();
void erase_food();
void draw_score();
void draw_borders();
void spawn_food();
bool check_collision();
void show_menu();
void show_game_over();

void game_timer_check() {
    if (game_state == STATE_PLAYING) {
        update_game();
    }
}

void init_game() {
    claim(stdout);
    set_orientation(2);
    set_font((unsigned char*)Arial12x12);
    snake_x[0] = GRID_WIDTH / 2;
    snake_y[0] = GRID_HEIGHT / 2;
    snake_length = 1;
    snake_dx = 1;
    snake_dy = 0;
    score = 0;
    spawn_food();
    if (game_state == STATE_MENU) {
        show_menu();
    } else if (game_state == STATE_PLAYING) {
        draw_screen();
    }
}

void spawn_food() {
    bool valid;
    do {
        food_x = rand() % GRID_WIDTH;
        food_y = rand() % GRID_HEIGHT;
        valid = true;
        for (int i = 0; i < snake_length; i++) {
            if (snake_x[i] == food_x && snake_y[i] == food_y) {
                valid = false;
                break;
            }
        }
    } while (!valid);
}

void update_game() {
    int new_x = snake_x[0] + snake_dx;
    int new_y = snake_y[0] + snake_dy;
    bool ate_food = (new_x == food_x && new_y == food_y);

    int tail_x = snake_x[snake_length - 1];
    int tail_y = snake_y[snake_length - 1];

    for (int i = snake_length - 1; i > 0; i--) {
        snake_x[i] = snake_x[i - 1];
        snake_y[i] = snake_y[i - 1];
    }

    snake_x[0] = new_x;
    snake_y[0] = new_y;

    if (ate_food) {
        snake_x[snake_length] = tail_x;
        snake_y[snake_length] = tail_y;
        snake_length++;
        score += 10;
        spawn_food();
        draw_food();
    } else {
        erase_tail(tail_x, tail_y);
    }

    draw_snake();
    draw_score();

    if (check_collision()) {
        draw_borders();
        game_state = STATE_GAME_OVER;
        show_game_over();
        return;
    }
}

bool check_collision() {
    if (snake_x[0] < 0 || snake_x[0] >= GRID_WIDTH || snake_y[0] < 0 || snake_y[0] >= GRID_HEIGHT) {
        return true;
    }
    for (int i = 1; i < snake_length; i++) {
        if (snake_x[0] == snake_x[i] && snake_y[0] == snake_y[i]) {
            return true;
        }
    }
    return false;
}

void draw_screen() {
    cls();
    background(COLOR_BACKGROUND);
    draw_borders();
    draw_full_snake();
    draw_food();
    draw_score();
}

void draw_snake() {
    fillrect(1 + snake_x[0] * SNAKE_SIZE, GAME_AREA_TOP + snake_y[0] * SNAKE_SIZE,
             1 + snake_x[0] * SNAKE_SIZE + SNAKE_SIZE, GAME_AREA_TOP + snake_y[0] * SNAKE_SIZE + SNAKE_SIZE, COLOR_SNAKE);
}

void draw_full_snake() {
    for (int i = 0; i < snake_length; i++) {
        fillrect(1 + snake_x[i] * SNAKE_SIZE, GAME_AREA_TOP + snake_y[i] * SNAKE_SIZE,
                 1 + snake_x[i] * SNAKE_SIZE + SNAKE_SIZE, GAME_AREA_TOP + snake_y[i] * SNAKE_SIZE + SNAKE_SIZE, COLOR_SNAKE);
    }
}

void erase_tail(int x, int y) {
    fillrect(1 + x * SNAKE_SIZE, GAME_AREA_TOP + y * SNAKE_SIZE,
             1 + x * SNAKE_SIZE + SNAKE_SIZE, GAME_AREA_TOP + y * SNAKE_SIZE + SNAKE_SIZE, COLOR_BACKGROUND);
}

void draw_food() {
    fillrect(1 + food_x * SNAKE_SIZE, GAME_AREA_TOP + food_y * SNAKE_SIZE,
             1 + food_x * SNAKE_SIZE + SNAKE_SIZE, GAME_AREA_TOP + food_y * SNAKE_SIZE + SNAKE_SIZE, COLOR_FOOD);
}

void erase_food() {
    fillrect(1 + food_x * SNAKE_SIZE, GAME_AREA_TOP + food_y * SNAKE_SIZE,
             1 + food_x * SNAKE_SIZE + SNAKE_SIZE, GAME_AREA_TOP + food_y * SNAKE_SIZE + SNAKE_SIZE, COLOR_BACKGROUND);
}

void draw_score() {
    auto style = *ui::Theme::getInstance()->fg_blue;
    painter.draw_string({5, 5}, style, "Score: " + std::to_string(score));
}

void draw_borders() {
    rect(0, GAME_AREA_TOP - 1, SCREEN_WIDTH, GAME_AREA_TOP, COLOR_BORDER);
    rect(0, GAME_AREA_TOP, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BORDER);
}

void show_menu() {
    cls();
    background(COLOR_BACKGROUND);
    auto style_yellow = *ui::Theme::getInstance()->fg_yellow;
    auto style_green = *ui::Theme::getInstance()->fg_green;
    auto style_blue = *ui::Theme::getInstance()->fg_blue;
    painter.draw_string({50, 40}, style_yellow, "* * * SNAKE * * *");
    painter.draw_string({0, 120}, style_blue, "USE THE D-PAD TO MOVE");
    painter.draw_string({0, 150}, style_blue, "EAT THE RED SQUARES TO GROW");
    painter.draw_string({0, 180}, style_blue, "DON'T HIT THE WALLS OR SELF");
    painter.draw_string({15, 240}, style_green, "** PRESS SELECT TO START **");
}

void show_game_over() {
    cls();
    background(COLOR_BACKGROUND);
    auto style_red = *ui::Theme::getInstance()->fg_red;
    auto style_yellow = *ui::Theme::getInstance()->fg_yellow;
    auto style_green = *ui::Theme::getInstance()->fg_green;
    painter.draw_string({75, 90}, style_red, "GAME OVER");
    painter.draw_string({74, 150}, style_yellow, "SCORE: " + std::to_string(score));
    painter.draw_string({20, 220}, style_green, "PRESS SELECT TO RESTART");
    wait(1);
}

int main() {
    if (!initialized) {
        initialized = true;
        game_timer.attach(&game_timer_check, 1.0 / 5.0);
        init_game();
    }
    while (1) {
        if (but_SELECT && (game_state == STATE_MENU || game_state == STATE_GAME_OVER)) {
            game_state = STATE_PLAYING;
            init_game();
        }
    }
}