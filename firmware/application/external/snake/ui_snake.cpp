/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#include "ui_snake.hpp"

namespace ui::external_app::snake {

void SnakeView::cls() {
    painter.fill_rectangle({0, 0, portapack::display.width(), portapack::display.height()}, Color::black());
}

void SnakeView::background(int color) {
    (void)color;
}

void SnakeView::fillrect(int x1, int y1, int x2, int y2, int color) {
    painter.fill_rectangle({x1, y1, x2 - x1, y2 - y1}, pp_colors[color]);
}

void SnakeView::rect(int x1, int y1, int x2, int y2, int color) {
    painter.draw_rectangle({x1, y1, x2 - x1, y2 - y1}, pp_colors[color]);
}

void SnakeView::check_game_timer() {
    if (game_update_callback) {
        if (++game_update_counter >= game_update_timeout) {
            game_update_counter = 0;
            game_timer_check();
        }
    }
}

void SnakeView::attach(double delay_sec) {
    game_update_callback = true;
    game_update_timeout = delay_sec * 60;
}

void SnakeView::detach() {
    game_update_callback = false;
}

void SnakeView::game_timer_check() {
    if (game_state == STATE_PLAYING) {
        update_game();
    }
}

void SnakeView::init_game() {
    SCREEN_WIDTH = screen_width;
    SCREEN_HEIGHT = screen_height;
    GAME_AREA_HEIGHT = (SCREEN_HEIGHT - INFO_BAR_HEIGHT - 2);
    GRID_WIDTH = ((SCREEN_WIDTH - 2) / SNAKE_SIZE);
    GRID_HEIGHT = (GAME_AREA_HEIGHT / SNAKE_SIZE);
    snake_x.resize(GRID_WIDTH * GRID_HEIGHT);
    snake_y.resize(GRID_WIDTH * GRID_HEIGHT);
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

void SnakeView::spawn_food() {
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

void SnakeView::update_game() {
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

bool SnakeView::check_collision() {
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

void SnakeView::draw_screen() {
    cls();
    background(COLOR_BACKGROUND);
    draw_borders();
    draw_full_snake();
    draw_food();
    draw_score();
}

void SnakeView::draw_snake() {
    fillrect(1 + snake_x[0] * SNAKE_SIZE, GAME_AREA_TOP + snake_y[0] * SNAKE_SIZE,
             1 + snake_x[0] * SNAKE_SIZE + SNAKE_SIZE, GAME_AREA_TOP + snake_y[0] * SNAKE_SIZE + SNAKE_SIZE, COLOR_SNAKE);
}

void SnakeView::draw_full_snake() {
    for (int i = 0; i < snake_length; i++) {
        fillrect(1 + snake_x[i] * SNAKE_SIZE, GAME_AREA_TOP + snake_y[i] * SNAKE_SIZE,
                 1 + snake_x[i] * SNAKE_SIZE + SNAKE_SIZE, GAME_AREA_TOP + snake_y[i] * SNAKE_SIZE + SNAKE_SIZE, COLOR_SNAKE);
    }
}

void SnakeView::erase_tail(int x, int y) {
    fillrect(1 + x * SNAKE_SIZE, GAME_AREA_TOP + y * SNAKE_SIZE,
             1 + x * SNAKE_SIZE + SNAKE_SIZE, GAME_AREA_TOP + y * SNAKE_SIZE + SNAKE_SIZE, COLOR_BACKGROUND);
}

void SnakeView::draw_food() {
    fillrect(1 + food_x * SNAKE_SIZE, GAME_AREA_TOP + food_y * SNAKE_SIZE,
             1 + food_x * SNAKE_SIZE + SNAKE_SIZE, GAME_AREA_TOP + food_y * SNAKE_SIZE + SNAKE_SIZE, COLOR_FOOD);
}

void SnakeView::erase_food() {
    fillrect(1 + food_x * SNAKE_SIZE, GAME_AREA_TOP + food_y * SNAKE_SIZE,
             1 + food_x * SNAKE_SIZE + SNAKE_SIZE, GAME_AREA_TOP + food_y * SNAKE_SIZE + SNAKE_SIZE, COLOR_BACKGROUND);
}

void SnakeView::draw_score() {
    auto style = *ui::Theme::getInstance()->fg_blue;
    painter.draw_string({5, 5}, style, "Score: " + std::to_string(score));
}

void SnakeView::draw_borders() {
    rect(0, GAME_AREA_TOP - 1, SCREEN_WIDTH, GAME_AREA_TOP, COLOR_BORDER);
    rect(0, GAME_AREA_TOP, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BORDER);
}

void SnakeView::show_menu() {
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

void SnakeView::show_game_over() {
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

SnakeView::SnakeView(NavigationView& nav)
    : nav_{nav} {
    add_children({&dummy});
    attach(1.0 / 5.0);
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
        if (key == KeyEvent::Left) {
            if (snake_dx == 0) {
                snake_dx = -1;
                snake_dy = 0;
            }
        } else if (key == KeyEvent::Right) {
            if (snake_dx == 0) {
                snake_dx = 1;
                snake_dy = 0;
            }
        } else if (key == KeyEvent::Up) {
            if (snake_dy == 0) {
                snake_dx = 0;
                snake_dy = -1;
            }
        } else if (key == KeyEvent::Down) {
            if (snake_dy == 0) {
                snake_dx = 0;
                snake_dy = 1;
            }
        }
    }
    set_dirty();
    return true;
}

}  // namespace ui::external_app::snake