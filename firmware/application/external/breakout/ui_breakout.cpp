/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#include "ui_breakout.hpp"
#include "baseband_api.hpp"
#include "audio.hpp"
#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"

using namespace portapack;

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

    // Play startup sound
    if (sound_enabled) {
        baseband::request_audio_beep(262, 24000, 150);  // C4 -
        chThdSleepMilliseconds(200);
        baseband::request_audio_beep(262, 24000, 150);  // C4
        chThdSleepMilliseconds(200);
        baseband::request_audio_beep(392, 24000, 150);  // G4
        chThdSleepMilliseconds(200);
        baseband::request_audio_beep(392, 24000, 150);  // G4
        chThdSleepMilliseconds(200);
        baseband::request_audio_beep(440, 24000, 150);  // A4
        chThdSleepMilliseconds(200);
        baseband::request_audio_beep(440, 24000, 150);  // A4
        chThdSleepMilliseconds(200);
        baseband::request_audio_beep(392, 24000, 300);  // G4
        chThdSleepMilliseconds(350);
        baseband::request_audio_beep(349, 24000, 150);  // F4
        chThdSleepMilliseconds(200);
        baseband::request_audio_beep(349, 24000, 150);  // F4
        chThdSleepMilliseconds(200);
        baseband::request_audio_beep(330, 24000, 150);  // E4
        chThdSleepMilliseconds(200);
        baseband::request_audio_beep(330, 24000, 150);  // E4
        chThdSleepMilliseconds(200);
        baseband::request_audio_beep(294, 24000, 150);  // D4
        chThdSleepMilliseconds(200);
        baseband::request_audio_beep(294, 24000, 150);  // D4
        chThdSleepMilliseconds(200);
        baseband::request_audio_beep(262, 24000, 400);  // C4
    }
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

void BreakoutView::draw_menu_volume_bar() {
    // Clear the entire volume area first to handle bar shrinking
    painter.fill_rectangle({40, 259, 220, 40}, Color::black());

    // Draw volume label
    auto style = *ui::Theme::getInstance()->fg_light;
    painter.draw_string({40, 260}, style, "Volume:");

    // Draw volume bar background
    painter.draw_rectangle({100, 259, 102, 10}, Color::grey());

    // Draw volume level
    int bar_width = (volume_level * 100) / 99;
    if (bar_width > 0) {
        painter.fill_rectangle({101, 260, bar_width, 8}, Color::green());
    }

    // Draw percentage
    painter.draw_string({210, 260}, style, std::to_string(volume_level) + "%");

    // Draw sound status
    if (sound_enabled) {
        painter.draw_string({UI_POS_X_CENTER(11), 280}, *ui::Theme::getInstance()->fg_green, "Sound: ON");
    } else {
        painter.draw_string({UI_POS_X_CENTER(11), 280}, *ui::Theme::getInstance()->fg_red, "Sound: OFF");
    }
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

void BreakoutView::adjust_volume(int delta) {
    volume_level += delta;
    if (volume_level < 0) volume_level = 0;
    if (volume_level > 99) volume_level = 99;

    // Update the actual audio volume using decibel conversion
    // Map 0-99 to roughly -60dB to 0dB range
    int db = (volume_level - 99) * 60 / 99;  // Maps 0->-60dB, 99->0dB
    audio::headphone::set_volume(volume_t::decibel(db));

    // If we're in the menu, redraw the volume bar
    if (game_state == STATE_MENU) {
        draw_menu_volume_bar();
    }
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
        play_wall_hit();
    } else if (ball_x > SCREEN_WIDTH - BALL_SIZE) {
        ball_x = SCREEN_WIDTH - BALL_SIZE;
        ball_dx = -ball_dx;
        play_wall_hit();
    }

    if (ball_y < GAME_AREA_TOP) {
        ball_y = GAME_AREA_TOP;
        ball_dy = -ball_dy;
        play_wall_hit();
    }

    // Bottom edge - lose life
    if (ball_y > GAME_AREA_BOTTOM) {
        lives--;
        play_death_sound();  // Play death sound
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

            play_paddle_hit();

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

        play_brick_hit();

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
    play_level_complete();
    chThdSleepMilliseconds(500);  // Give time for sound to play
    init_level();
    draw_screen();
}

void BreakoutView::handle_game_over() {
    game_state = STATE_GAME_OVER;
    gameover_blink_state = true;
    gameover_blink_counter = 0;
    play_game_over();
}

void BreakoutView::show_menu() {
    // Only draw the static parts once
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

        // Volume adjustment instructions
        painter.draw_string({UI_POS_X_CENTER(24), 220}, style_cyan, "Press UP/DOWN for Volume");

        // Draw volume control at bottom
        draw_menu_volume_bar();
    }

    // Only flash the "PRESS SELECT" text
    if (++menu_blink_counter >= 30) {
        menu_blink_counter = 1;  // Keep it at 1 so we don't redraw everything
        menu_blink_state = !menu_blink_state;

        auto style_red = *ui::Theme::getInstance()->fg_red;
        painter.fill_rectangle({UI_POS_X_CENTER(17), 238, 128, 20}, Color::black());

        if (menu_blink_state) {
            painter.draw_string({UI_POS_X_CENTER(17), 240}, style_red, "* PRESS SELECT *");
        }
    }
}

void BreakoutView::show_game_over() {
    // Only draw the static parts once
    if (gameover_blink_counter == 0) {
        cls();
        background(COLOR_BACKGROUND);

        auto style_red = *ui::Theme::getInstance()->fg_red;
        auto style_yellow = *ui::Theme::getInstance()->fg_yellow;

        painter.draw_string({UI_POS_X_CENTER(10), 100}, style_red, "GAME OVER");
        painter.draw_string({UI_POS_X_CENTER(11), 150}, style_yellow, "SCORE: " + std::to_string(score));
    }

    // Only flash the "PRESS SELECT" text
    if (++gameover_blink_counter >= 30) {
        gameover_blink_counter = 1;  // Keep it at 1 so we don't redraw everything
        gameover_blink_state = !gameover_blink_state;

        auto style_green = *ui::Theme::getInstance()->fg_green;
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

    dummy.focus();

    init_level();
    draw_screen();
}

// Sound effect implementations with timing control
void BreakoutView::play_paddle_hit() {
    if (sound_enabled) {
        uint32_t current_time = chTimeNow();
        if (current_time - last_sound_time >= min_sound_interval) {
            baseband::request_beep_stop();                 // Stop any previous sound
            chThdSleepMilliseconds(5);                     // Small delay
            baseband::request_audio_beep(440, 24000, 40);  // 440Hz A4, 40ms
            last_sound_time = current_time;
        }
    }
}

void BreakoutView::play_brick_hit() {
    if (sound_enabled) {
        uint32_t current_time = chTimeNow();
        if (current_time - last_sound_time >= min_sound_interval) {
            baseband::request_beep_stop();  // Stop any previous sound
            chThdSleepMilliseconds(5);      // Small delay
            // Higher pitch that varies with remaining bricks
            uint32_t freq = 880 + ((25 - brick_count) * 20);  // Gets higher as bricks are destroyed
            baseband::request_audio_beep(freq, 24000, 30);
            last_sound_time = current_time;
        }
    }
}

void BreakoutView::play_wall_hit() {
    if (sound_enabled) {
        uint32_t current_time = chTimeNow();
        if (current_time - last_sound_time >= min_sound_interval) {
            baseband::request_beep_stop();                  // Stop any previous sound
            chThdSleepMilliseconds(5);                      // Small delay
            baseband::request_audio_beep(220, 24000, 120);  // 220Hz A3, 120ms
            last_sound_time = current_time;
        }
    }
}

void BreakoutView::play_death_sound() {
    if (sound_enabled) {
        baseband::request_audio_beep(493, 24000, 150);  // B4
        chThdSleepMilliseconds(180);
        baseband::request_audio_beep(466, 24000, 150);  // Bb4
        chThdSleepMilliseconds(180);
        baseband::request_audio_beep(440, 24000, 150);  // A4
        chThdSleepMilliseconds(180);
        baseband::request_audio_beep(415, 24000, 150);  // Ab4
        chThdSleepMilliseconds(180);
        baseband::request_audio_beep(392, 24000, 150);  // G4
        chThdSleepMilliseconds(180);
        baseband::request_audio_beep(369, 24000, 150);  // Gb4
        chThdSleepMilliseconds(180);
        baseband::request_audio_beep(349, 24000, 150);  // F4
        chThdSleepMilliseconds(180);
        baseband::request_audio_beep(329, 24000, 150);  // E4
        chThdSleepMilliseconds(180);
        baseband::request_audio_beep(293, 24000, 150);  // D4
        chThdSleepMilliseconds(180);
        baseband::request_audio_beep(261, 24000, 150);  // C4
        chThdSleepMilliseconds(180);
        baseband::request_audio_beep(196, 24000, 200);  // G3
        chThdSleepMilliseconds(230);
        baseband::request_audio_beep(130, 24000, 300);  // C3
    }
}

void BreakoutView::play_game_over() {
    if (sound_enabled) {
        // First phrase
        baseband::request_audio_beep(392, 24000, 300);  // G4
        chThdSleepMilliseconds(350);
        baseband::request_audio_beep(392, 24000, 150);  // G4
        chThdSleepMilliseconds(200);
        baseband::request_audio_beep(392, 24000, 150);  // G4
        chThdSleepMilliseconds(200);
        baseband::request_audio_beep(311, 24000, 500);  // Eb4 - long
        chThdSleepMilliseconds(600);

        // Second phrase
        baseband::request_audio_beep(349, 24000, 300);  // F4
        chThdSleepMilliseconds(350);
        baseband::request_audio_beep(349, 24000, 150);  // F4
        chThdSleepMilliseconds(200);
        baseband::request_audio_beep(349, 24000, 150);  // F4
        chThdSleepMilliseconds(200);
        baseband::request_audio_beep(294, 24000, 500);  // D4 - long
        chThdSleepMilliseconds(600);

        // Final phrase - slower and more dramatic
        baseband::request_audio_beep(262, 24000, 200);  // C4
        chThdSleepMilliseconds(250);
        baseband::request_audio_beep(247, 24000, 200);  // B3
        chThdSleepMilliseconds(250);
        baseband::request_audio_beep(233, 24000, 200);  // Bb3
        chThdSleepMilliseconds(250);
        baseband::request_audio_beep(220, 24000, 200);  // A3
        chThdSleepMilliseconds(250);
        baseband::request_audio_beep(196, 24000, 200);  // G3
        chThdSleepMilliseconds(250);
        baseband::request_audio_beep(175, 24000, 200);  // F3
        chThdSleepMilliseconds(250);
        baseband::request_audio_beep(131, 24000, 600);  // C3
    }
}

void BreakoutView::play_level_complete() {
    if (sound_enabled) {
        baseband::request_beep_stop();  // Stop any previous sound
        chThdSleepMilliseconds(10);
        baseband::request_audio_beep(1047, 24000, 300);  // 1047Hz C6, 300ms
    }
}

BreakoutView::BreakoutView(NavigationView& nav)
    : nav_{nav}, bricks{} {
    // Initialize audio system for beeps FIRST
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());  // Load the audio beep baseband

    add_children({&dummy});

    // Initialize audio
    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();

    // Set initial volume to 80% and ensure sound is ON
    volume_level = 80;
    sound_enabled = true;  // Make sure this is explicitly set to ON

    // Set the actual volume
    int db = (volume_level - 99) * 60 / 99;  // Maps to roughly -12dB for 80%
    audio::headphone::set_volume(volume_t::decibel(db));

    attach(1.0 / 60.0);
}

BreakoutView::~BreakoutView() {
    // Clean shutdown
    baseband::request_beep_stop();
    receiver_model.disable();
    baseband::shutdown();
    audio::output::stop();
}

void BreakoutView::on_show() {
    // Nothing needed here, initialization done in constructor
}

void BreakoutView::on_hide() {
    // Additional cleanup
    baseband::request_beep_stop();
    audio::output::stop();
}

void BreakoutView::focus() {
    dummy.focus();
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
            // Return to menu after game over
            game_state = STATE_MENU;
            menu_blink_counter = 0;
            init_game();
        }
    } else if (key == KeyEvent::Left && game_state == STATE_PLAYING) {
        move_paddle_left();
    } else if (key == KeyEvent::Right && game_state == STATE_PLAYING) {
        move_paddle_right();
    } else if (key == KeyEvent::Up && game_state == STATE_MENU) {
        // Only adjust volume in menu
        adjust_volume(5);
        if (!sound_enabled) sound_enabled = true;
        set_dirty();
    } else if (key == KeyEvent::Down && game_state == STATE_MENU) {
        // Only adjust volume in menu
        adjust_volume(-5);
        set_dirty();
    }

    set_dirty();
    return true;
}

bool BreakoutView::on_touch(const TouchEvent event) {
    (void)event;  // Intentionally unused - touch disabled to avoid nav bar issues
    return true;
}

}  // namespace ui::external_app::breakout