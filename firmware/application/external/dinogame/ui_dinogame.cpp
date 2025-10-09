/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

/*
 * Chrome Dino Game for Portapack Mayhem
 * Based on the original DinoGame by various contributors
 */

#include "ui_dinogame.hpp"

namespace ui::external_app::dinogame {

#define PROGMEM
#include "sprites/dino.h"
#include "sprites/pterodactyl.h"

#undef PROGMEM

// Global variables
Ticker game_timer;
Painter painter;
static Callback game_update_callback = nullptr;
static uint32_t game_update_timeout = 0;
static uint32_t game_update_counter = 0;
static DinoGameView* current_instance = nullptr;

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

// Drawing functions
void cls() {
    painter.fill_rectangle({0, 0, ui::screen_width, ui::screen_height}, Color::black());
}

void fillrect(int x1, int y1, int x2, int y2, int color) {
    painter.fill_rectangle({x1, y1, x2 - x1, y2 - y1}, pp_colors[color]);
}

void rect(int x1, int y1, int x2, int y2, int color) {
    painter.draw_rectangle({x1, y1, x2 - x1, y2 - y1}, pp_colors[color]);
}

// Timer implementation
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

// String helper
std::string DinoGameView::score_to_string(uint32_t score) {
    std::string score_s = std::to_string(score);
    if (score_s.length() < 5) {
        std::string temp = "";
        for (uint8_t i = 0; i < 5 - score_s.length(); ++i) temp += "0";
        temp += score_s;
        score_s = temp;
    }
    return score_s;
}

// Game timer callback
void game_timer_check() {
    if (current_instance) {
        if (current_instance->game_state == GameState::PLAYING) {
            current_instance->game_loop();
        } else if (current_instance->game_state == GameState::MENU) {
            current_instance->show_menu();
        }
    }
}

DinoGameView::DinoGameView(NavigationView& nav)
    : nav_{nav}, bird_info{}, game_timer{} {
    add_children({&dummy, &button_difficulty});
    current_instance = this;

    // Initialize dimensions first
    init_dimensions();

    // Now reposition button with proper centering
    int button_y = SCREEN_HEIGHT - 100;
    int button_x = (SCREEN_WIDTH - 100) / 2;
    button_difficulty.set_parent_rect({button_x, button_y, 100, 20});

    game_timer.attach(&game_timer_check, 1.0 / 60.0);

    button_difficulty.on_select = [this](Button&) {
        easy_mode = !easy_mode;
        button_difficulty.set_text(easy_mode ? "Mode: EASY" : "Mode: HARD");
    };
}

void DinoGameView::on_show() {
}

void DinoGameView::paint(Painter& painter) {
    (void)painter;

    if (!initialized) {
        initialized = true;
        std::srand(LPC_RTC->CTIME0);
        init_game();
    }
}

void DinoGameView::frame_sync() {
    check_game_timer();
    set_dirty();
}

void DinoGameView::init_dimensions() {
    SCREEN_WIDTH = ui::screen_width;
    SCREEN_HEIGHT = ui::screen_height;

    // Scale game area based on screen size
    GAME_AREA_HEIGHT = (SCREEN_HEIGHT * 160) / 320;  // Scale proportionally
    GAME_AREA_TOP = SCREEN_HEIGHT / 4;

    // Calculate positions
    DINO_Y = GAME_AREA_TOP + GAME_AREA_HEIGHT - GROUND_HEIGHT - DINO_HEIGHT;
    DINO_DUCK_Y = GAME_AREA_TOP + GAME_AREA_HEIGHT - GROUND_HEIGHT - DINO_DUCK_HEIGHT;
    BIRD_Y_UP = GAME_AREA_TOP + 20;
    BIRD_Y_DOWN = GAME_AREA_TOP + (GAME_AREA_HEIGHT * 60) / 160;

    // Scale jump height
    JUMP_MAX_HEIGHT = (GAME_AREA_HEIGHT * 70) / 160;

    // Scale distances based on screen width
    MIN_OBSTACLE_DISTANCE = (SCREEN_WIDTH * 300) / 240;
    MAX_OBSTACLE_DISTANCE = (SCREEN_WIDTH * 600) / 240;

    // Scale horizontal position
    DINO_X = SCREEN_WIDTH / 8;

    last_dino_y = DINO_Y;
}

void DinoGameView::init_game() {
    game_state = GameState::MENU;
    menu_initialized = false;
    blink_state = true;
    blink_counter = 0;
    new_game();
}

void DinoGameView::new_game() {
    steps = 0;
    score = 0;
    collided = false;
    ducking = false;
    duck_timer = 0;
    displayedGameOver = false;
    bird_info.inGame = false;
    bird_info.x_offset = SCREEN_WIDTH;
    bird_info.y_offset = 0;
    bird_info.y_velocity = 0;
    jumping = false;
    falling = false;
    jumpHeight = 0;
    last_dino_y = DINO_Y;
    last_bird_x = -1;
    last_bird_y = -1;
    last_ducking = false;
    last_runstate = false;
    ground_offset = 0;
    speed_modifier = 0;
    runstate = false;
    score_drawn = false;
    last_score = 999999;
    obstacle_spawn_timer = 100;  // Initial delay before first obstacle

    // Initialize obstacles
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        obstacles[i].active = false;
        obstacles[i].x = -100;
        obstacles[i].last_x = -100;
    }

    cls();
}

void DinoGameView::show_menu() {
    if (!menu_initialized) {
        cls();

        auto style = *ui::Theme::getInstance()->fg_medium;
        auto style_title = *ui::Theme::getInstance()->fg_light;

        // Draw title
        painter.draw_string({UI_POS_X_CENTER(10), 60}, style_title, "DINO GAME");

        // Draw instructions
        painter.draw_string({UI_POS_X_CENTER(19), 120}, style, "SELECT: Jump/Start");
        painter.draw_string({UI_POS_X_CENTER(11), 140}, style, "DOWN: Duck");
        painter.draw_string({UI_POS_X_CENTER(17), 160}, style, "Avoid obstacles!");

        // Draw high score
        draw_high_score();

        menu_initialized = true;
    }

    // Show difficulty button
    button_difficulty.hidden(false);

    // Animate the menu dino - use frame counter for smooth animation
    bool menu_run_frame = (blink_counter / 10) % 2;

    int menu_dino_x = UI_POS_X_CENTER(DINO_WIDTH);
    int menu_dino_y = 90;

    // Clear previous dino position - clear a bit extra to handle any artifacts
    static bool last_menu_frame = false;
    if (last_menu_frame != menu_run_frame || blink_counter == 0) {
        fillrect(menu_dino_x - 5, menu_dino_y - 5, menu_dino_x + DINO_WIDTH + 5, menu_dino_y + DINO_HEIGHT + 5, Black);
        last_menu_frame = menu_run_frame;
    }

    // Draw animated dino
    draw_dino_at(menu_dino_x, menu_dino_y, false, menu_run_frame);

    // Blinking start prompt
    auto style_prompt = *ui::Theme::getInstance()->fg_light;
    if (++blink_counter >= 30) {
        blink_counter = 0;
        blink_state = !blink_state;

        int prompt_y = SCREEN_HEIGHT - 60;
        painter.fill_rectangle({UI_POS_X_CENTER(17), prompt_y - 2, 130, 20}, Color::black());
        if (blink_state) {
            painter.draw_string({UI_POS_X_CENTER(17), prompt_y}, style_prompt, "* PRESS SELECT *");
        }
    }
}

void DinoGameView::show_game_over() {
    if (!displayedGameOver) {
        displayedGameOver = true;

        // Clear the entire play area to ensure clean display
        fillrect(0, GAME_AREA_TOP, SCREEN_WIDTH, GAME_AREA_HEIGHT + 20, Black);

        // Draw the game over dino sprite in the center
        int gameover_dino_x = UI_POS_X_CENTER(DINO_WIDTH);
        int gameover_dino_y = GAME_AREA_TOP + 40;
        draw_dino_sprite(gameover_dino_x, gameover_dino_y, dino_gameover);

        auto style = *ui::Theme::getInstance()->fg_light;
        auto style_score = *ui::Theme::getInstance()->fg_medium;

        // Game over text
        painter.draw_string({UI_POS_X_CENTER(10), GAME_AREA_TOP + 90}, style, "GAME OVER");

        // Show final score
        std::string score_text = "SCORE: " + score_to_string(score);
        painter.draw_string({UI_POS_X_CENTER(score_text.length()), GAME_AREA_TOP + 110}, style_score, score_text);

        painter.draw_string({UI_POS_X_CENTER(16), GAME_AREA_TOP + 130}, style, "SELECT TO RETRY");

        // Update high score
        if (score > highScore) {
            highScore = score;
            painter.draw_string({UI_POS_X_CENTER(16), GAME_AREA_TOP + 150}, style, "NEW HIGH SCORE!");
        }
    }
}

void DinoGameView::game_loop() {
    button_difficulty.hidden(true);

    if (collided && game_state == GameState::PLAYING) {
        game_state = GameState::GAME_OVER;
        show_game_over();
        return;
    }

    // Update ground animation
    ground_offset = (ground_offset + GAME_SPEED_BASE + speed_modifier) % 20;

    // Draw ground
    draw_ground();

    // Update and draw obstacles
    update_obstacles();

    // Manage bird
    manage_bird();

    // Handle jumping
    if (jumping) {
        if (!falling) jumpHeight += JUMP_SPEED + speed_modifier;
        if (jumpHeight > JUMP_MAX_HEIGHT && !falling) falling = true;
        if (falling) jumpHeight -= JUMP_SPEED + speed_modifier;
        if (jumpHeight < 0) {
            falling = false;
            jumping = false;
            jumpHeight = 0;
        }
    }

    // Handle auto-stand from duck
    if (ducking && duck_timer > 0) {
        duck_timer--;
        if (duck_timer == 0) {
            stand();
        }
    }

    // Update run animation
    if (get_steps() % (10 - speed_modifier) == 0) runstate = !runstate;

    // Draw dino with proper clearing
    int current_dino_y = jumping ? (DINO_Y - jumpHeight) : (ducking ? DINO_DUCK_Y : DINO_Y);

    // Only clear and redraw if something changed
    if (current_dino_y != last_dino_y || runstate != last_runstate || ducking != last_ducking) {
        // Clear old position with extra margin to prevent artifacts
        if (last_ducking) {
            fillrect(DINO_X - 2, last_dino_y - 2, DINO_X + DINO_DUCK_WIDTH + 2, last_dino_y + DINO_DUCK_HEIGHT + 2, Black);
        } else {
            fillrect(DINO_X - 2, last_dino_y - 2, DINO_X + DINO_WIDTH + 2, last_dino_y + DINO_HEIGHT + 2, Black);
        }

        // Draw dino at new position
        if (jumping) {
            draw_dino_sprite(DINO_X, current_dino_y, dino_default);
        } else if (ducking) {
            if (runstate)
                draw_dino_sprite(DINO_X, current_dino_y, dino_ducking_leftstep);
            else
                draw_dino_sprite(DINO_X, current_dino_y, dino_ducking_rightstep);
        } else {
            if (runstate)
                draw_dino_sprite(DINO_X, current_dino_y, dino_leftstep);
            else
                draw_dino_sprite(DINO_X, current_dino_y, dino_rightstep);
        }

        // Update last state
        last_dino_y = current_dino_y;
        last_ducking = ducking;
        last_runstate = runstate;
    }

    // Check collisions
    check_collision();

    // Update score
    score = get_steps() / 10;
    if (score % 100 == 0 && score > 0 && get_steps() % 1000 == 0) {
        if (speed_modifier < 5) speed_modifier++;
    }

    step();
    draw_current_score();
    draw_high_score();
}

void DinoGameView::draw_ground() {
    int ground_y = GAME_AREA_TOP + GAME_AREA_HEIGHT - GROUND_HEIGHT;

    // Clear ground area
    fillrect(0, ground_y, SCREEN_WIDTH, ground_y + GROUND_HEIGHT, Black);

    // Draw ground line
    painter.draw_hline({0, ground_y}, SCREEN_WIDTH, Color::white());
    painter.draw_hline({0, ground_y + 1}, SCREEN_WIDTH, Color::dark_grey());

    // Draw ground texture
    for (int x = -ground_offset; x < SCREEN_WIDTH; x += 20) {
        painter.draw_hline({x, ground_y + 3}, 10, Color::dark_grey());
        painter.draw_hline({x + 5, ground_y + 5}, 5, Color::dark_grey());
    }
}

void DinoGameView::update_obstacles() {
    // Move obstacles
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (obstacles[i].active) {
            // Clear old position
            if (obstacles[i].last_x != obstacles[i].x && obstacles[i].last_x < SCREEN_WIDTH) {
                clear_obstacle_area(obstacles[i].last_x, obstacles[i].width + 10, obstacles[i].height + 10);
            }

            obstacles[i].last_x = obstacles[i].x;
            obstacles[i].x -= GAME_SPEED_BASE + speed_modifier;

            // Remove off-screen obstacles
            if (obstacles[i].x < -50) {
                obstacles[i].active = false;
                clear_obstacle_area(obstacles[i].last_x, obstacles[i].width + 10, obstacles[i].height + 10);
            } else {
                // Draw obstacle at new position
                draw_obstacle(obstacles[i]);
            }
        }
    }

    // Decrement spawn timer
    if (obstacle_spawn_timer > 0) {
        obstacle_spawn_timer -= GAME_SPEED_BASE + speed_modifier;
    }

    // Check if we should spawn a new obstacle
    if (obstacle_spawn_timer <= 0) {
        // Try to spawn an obstacle
        bool spawned = false;
        for (int i = 0; i < MAX_OBSTACLES; i++) {
            if (!obstacles[i].active) {
                obstacles[i].active = true;
                obstacles[i].x = SCREEN_WIDTH;
                obstacles[i].last_x = SCREEN_WIDTH;
                obstacles[i].type = rand() % 4;

                // Set obstacle dimensions based on type
                switch (obstacles[i].type) {
                    case 0:  // Small cactus
                        obstacles[i].width = 15;
                        obstacles[i].height = 25;
                        break;
                    case 1:  // Large cactus
                        obstacles[i].width = 20;
                        obstacles[i].height = 35;
                        break;
                    case 2:  // Double cactus
                        obstacles[i].width = 35;
                        obstacles[i].height = 30;
                        break;
                    case 3:  // Triple cactus
                        obstacles[i].width = 45;
                        obstacles[i].height = 28;
                        break;
                }

                spawned = true;
                break;
            }
        }

        if (spawned) {
            // Set timer for next obstacle with random variation
            obstacle_spawn_timer = MIN_OBSTACLE_DISTANCE + (rand() % (MAX_OBSTACLE_DISTANCE - MIN_OBSTACLE_DISTANCE));
        }
    }
}

void DinoGameView::clear_obstacle_area(int x, int width, int height) {
    int ground_y = GAME_AREA_TOP + GAME_AREA_HEIGHT - GROUND_HEIGHT;
    // Only clear the obstacle area, not the entire vertical space
    fillrect(x - 5, ground_y - height - 5, x + width + 5, ground_y, Black);
}

void DinoGameView::draw_obstacle(const SimpleObstacle& obstacle) {
    if (!obstacle.active) return;

    int ground_y = GAME_AREA_TOP + GAME_AREA_HEIGHT - GROUND_HEIGHT;
    int y = ground_y - obstacle.height;

    // Draw cactus with green color
    switch (obstacle.type) {
        case 0:  // Small single cactus
            fillrect(obstacle.x + 5, y, obstacle.x + 10, ground_y, Green);
            fillrect(obstacle.x, y + 8, obstacle.x + 5, y + 15, Green);
            fillrect(obstacle.x + 10, y + 12, obstacle.x + 15, y + 19, Green);
            // Add darker edges for definition
            painter.draw_vline({obstacle.x + 4, y + 8}, 7, Color::dark_green());
            painter.draw_vline({obstacle.x + 10, y + 12}, 7, Color::dark_green());
            break;

        case 1:  // Large single cactus
            fillrect(obstacle.x + 7, y, obstacle.x + 13, ground_y, Green);
            fillrect(obstacle.x, y + 10, obstacle.x + 7, y + 20, Green);
            fillrect(obstacle.x + 13, y + 15, obstacle.x + 20, y + 25, Green);
            fillrect(obstacle.x + 5, y + 5, obstacle.x + 7, y + 12, Green);
            fillrect(obstacle.x + 13, y + 8, obstacle.x + 15, y + 15, Green);
            // Add darker edges
            painter.draw_vline({obstacle.x + 6, y}, ground_y - y, Color::dark_green());
            painter.draw_vline({obstacle.x + 13, y}, ground_y - y, Color::dark_green());
            break;

        case 2:  // Double cactus
            fillrect(obstacle.x + 5, y + 5, obstacle.x + 10, ground_y, Green);
            fillrect(obstacle.x + 20, y, obstacle.x + 25, ground_y, Green);
            fillrect(obstacle.x, y + 12, obstacle.x + 5, y + 18, Green);
            fillrect(obstacle.x + 10, y + 15, obstacle.x + 15, y + 22, Green);
            fillrect(obstacle.x + 25, y + 10, obstacle.x + 30, y + 17, Green);
            // Darker outlines
            painter.draw_vline({obstacle.x + 4, y + 5}, ground_y - y - 5, Color::dark_green());
            painter.draw_vline({obstacle.x + 19, y}, ground_y - y, Color::dark_green());
            break;

        case 3:  // Triple cactus
            fillrect(obstacle.x + 5, y + 8, obstacle.x + 10, ground_y, Green);
            fillrect(obstacle.x + 20, y, obstacle.x + 25, ground_y, Green);
            fillrect(obstacle.x + 35, y + 5, obstacle.x + 40, ground_y, Green);
            fillrect(obstacle.x, y + 15, obstacle.x + 5, y + 20, Green);
            fillrect(obstacle.x + 25, y + 8, obstacle.x + 30, y + 15, Green);
            fillrect(obstacle.x + 40, y + 12, obstacle.x + 45, y + 18, Green);
            // Darker outlines
            painter.draw_vline({obstacle.x + 4, y + 8}, ground_y - y - 8, Color::dark_green());
            painter.draw_vline({obstacle.x + 19, y}, ground_y - y, Color::dark_green());
            painter.draw_vline({obstacle.x + 34, y + 5}, ground_y - y - 5, Color::dark_green());
            break;
    }
}

void DinoGameView::manage_bird() {
    if (!bird_info.inGame) {
        // Only spawn bird if no obstacles are too close
        bool obstacle_nearby = false;
        for (int i = 0; i < MAX_OBSTACLES; i++) {
            if (obstacles[i].active && obstacles[i].x > SCREEN_WIDTH * 0.8) {
                obstacle_nearby = true;
                break;
            }
        }

        // Randomly spawn bird if no nearby obstacles
        if (!obstacle_nearby && rand() % 400 == 0 && get_steps() > 100) {
            bird_info.inGame = true;
            bird_info.x_offset = SCREEN_WIDTH;
            bird_info.y_offset = 0;
            bird_info.y_velocity = easy_mode ? 0 : (rand() % 3) - 1;  // No vertical movement in easy mode
            bird_info.y_position = BirdPosition::DOWN;                // Always spawn at standing height in easy mode
        }
    } else {
        // Calculate bird Y position
        int base_y = (bird_info.y_position == BirdPosition::UP) ? BIRD_Y_UP : BIRD_Y_DOWN;
        int current_y = base_y + bird_info.y_offset;

        // Clear previous bird position with extra margin
        if (last_bird_x < SCREEN_WIDTH && last_bird_x >= -BIRD_WIDTH) {
            int clear_start_x = last_bird_x - 2;
            int clear_end_x = last_bird_x + BIRD_WIDTH + 2;

            if (clear_start_x < 0) clear_start_x = 0;
            if (clear_end_x > SCREEN_WIDTH) clear_end_x = SCREEN_WIDTH;

            if (clear_end_x > clear_start_x) {
                fillrect(clear_start_x, last_bird_y - 2, clear_end_x, last_bird_y + BIRD_HEIGHT + 2, Black);
            }
        }

        // Move bird
        bird_info.x_offset -= GAME_SPEED_BASE + speed_modifier + 1;

        // Update vertical position only in hard mode
        if (!easy_mode) {
            bird_info.y_offset += bird_info.y_velocity;
            if (rand() % 30 == 0) {
                bird_info.y_velocity = (rand() % 3) - 1;
            }

            // Keep bird within bounds
            if (current_y < GAME_AREA_TOP + 10) {
                current_y = GAME_AREA_TOP + 10;
                bird_info.y_offset = current_y - base_y;
                bird_info.y_velocity = 1;
            } else if (current_y > GAME_AREA_TOP + GAME_AREA_HEIGHT - GROUND_HEIGHT - BIRD_HEIGHT - 10) {
                current_y = GAME_AREA_TOP + GAME_AREA_HEIGHT - GROUND_HEIGHT - BIRD_HEIGHT - 10;
                bird_info.y_offset = current_y - base_y;
                bird_info.y_velocity = -1;
            }
        } else {
            // In easy mode, keep bird at perfect ducking height
            current_y = DINO_Y - 10;  // Position bird just above ducking height
        }

        // Animate wings
        if (get_steps() % 15 == 0) bird_info.flop = !bird_info.flop;

        // Draw bird only if any part is visible
        if (bird_info.x_offset > -BIRD_WIDTH && bird_info.x_offset < SCREEN_WIDTH) {
            if (bird_info.flop) {
                draw_bird_sprite(bird_info.x_offset, current_y, pterodactyl_upflop);
            } else {
                draw_bird_sprite(bird_info.x_offset, current_y, pterodactyl_downflop);
            }
        }

        // Update last position
        last_bird_x = bird_info.x_offset;
        last_bird_y = current_y;

        // Remove bird only when completely off screen
        if (bird_info.x_offset < -BIRD_WIDTH - 5) {
            bird_info.inGame = false;
            last_bird_x = -1;
            last_bird_y = -1;
        }
    }
}

void DinoGameView::draw_dino_sprite(int x, int y, const uint16_t* sprite) {
    int height, width;

    // Determine dimensions based on which sprite we're drawing
    if (sprite == dino_ducking_leftstep || sprite == dino_ducking_rightstep) {
        height = DINO_DUCK_HEIGHT;
        width = DINO_DUCK_WIDTH;
    } else {
        height = DINO_HEIGHT;
        width = DINO_WIDTH;
    }

    // Draw sprite efficiently with run-length optimization
    for (int dy = 0; dy < height; dy++) {
        int run_start = -1;
        uint16_t run_color = 0;

        for (int dx = 0; dx < width; dx++) {
            uint16_t pixel = sprite[dy * width + dx];

            if (pixel != TRANSPARENT_COLOR) {
                if (run_start == -1 || pixel != run_color) {
                    // Draw previous run if any
                    if (run_start != -1) {
                        painter.fill_rectangle({x + run_start, y + dy, dx - run_start, 1}, Color(run_color));
                    }
                    run_start = dx;
                    run_color = pixel;
                }
            } else {
                // Draw previous run if any
                if (run_start != -1) {
                    painter.fill_rectangle({x + run_start, y + dy, dx - run_start, 1}, Color(run_color));
                    run_start = -1;
                }
            }
        }

        // Draw final run if any
        if (run_start != -1) {
            painter.fill_rectangle({x + run_start, y + dy, width - run_start, 1}, Color(run_color));
        }
    }
}

void DinoGameView::draw_bird_sprite(int x, int y, const uint16_t* sprite) {
    // Draw sprite efficiently with clipping
    for (int dy = 0; dy < BIRD_HEIGHT; dy++) {
        int run_start = -1;
        uint16_t run_color = 0;

        for (int dx = 0; dx < BIRD_WIDTH; dx++) {
            // Skip pixels that would be off-screen
            if (x + dx < 0 || x + dx >= SCREEN_WIDTH) {
                // End any current run
                if (run_start != -1 && x + run_start < SCREEN_WIDTH) {
                    int run_width = dx - run_start;
                    if (x + run_start + run_width > SCREEN_WIDTH) {
                        run_width = SCREEN_WIDTH - (x + run_start);
                    }
                    painter.fill_rectangle({x + run_start, y + dy, run_width, 1}, Color(run_color));
                    run_start = -1;
                }
                continue;
            }

            uint16_t pixel = sprite[dy * BIRD_WIDTH + dx];

            if (pixel != TRANSPARENT_COLOR) {
                if (pixel == SPRITE_COLOR) {
                    pixel = Color::red().v;
                }

                if (run_start == -1 || pixel != run_color) {
                    if (run_start != -1) {
                        painter.fill_rectangle({x + run_start, y + dy, dx - run_start, 1}, Color(run_color));
                    }
                    run_start = dx;
                    run_color = pixel;
                }
            } else {
                if (run_start != -1) {
                    painter.fill_rectangle({x + run_start, y + dy, dx - run_start, 1}, Color(run_color));
                    run_start = -1;
                }
            }
        }

        if (run_start != -1 && x + run_start < SCREEN_WIDTH) {
            int width = BIRD_WIDTH - run_start;
            if (x + run_start + width > SCREEN_WIDTH) {
                width = SCREEN_WIDTH - (x + run_start);
            }
            if (width > 0) {
                painter.fill_rectangle({x + run_start, y + dy, width, 1}, Color(run_color));
            }
        }
    }
}

void DinoGameView::draw_dino_at(int x, int y, bool is_ducking, bool run_frame) {
    if (is_ducking) {
        if (run_frame) {
            draw_dino_sprite(x, y, dino_ducking_leftstep);
        } else {
            draw_dino_sprite(x, y, dino_ducking_rightstep);
        }
    } else {
        if (run_frame) {
            draw_dino_sprite(x, y, dino_leftstep);
        } else {
            draw_dino_sprite(x, y, dino_rightstep);
        }
    }
}

void DinoGameView::check_collision() {
    int dino_left = DINO_X;
    int dino_right = DINO_X + (ducking ? DINO_DUCK_WIDTH : DINO_WIDTH);
    int dino_top = ducking ? DINO_DUCK_Y : (DINO_Y - jumpHeight);
    int dino_bottom = dino_top + (ducking ? DINO_DUCK_HEIGHT : DINO_HEIGHT);

    // Give a small hitbox reduction for fairness
    dino_left += 5;
    dino_right -= 5;
    dino_top += 5;
    dino_bottom -= 5;

    // Check cactus collisions
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (obstacles[i].active) {
            int ground_y = GAME_AREA_TOP + GAME_AREA_HEIGHT - GROUND_HEIGHT;
            int obs_top = ground_y - obstacles[i].height;
            int obs_bottom = ground_y;

            if (dino_right > obstacles[i].x &&
                dino_left < obstacles[i].x + obstacles[i].width &&
                dino_bottom > obs_top &&
                dino_top < obs_bottom) {
                collided = true;
                return;
            }
        }
    }

    // Check bird collision
    if (bird_info.inGame) {
        int base_y = (bird_info.y_position == BirdPosition::UP) ? BIRD_Y_UP : BIRD_Y_DOWN;
        int bird_y = base_y + bird_info.y_offset;

        if (dino_right > bird_info.x_offset + 5 &&
            dino_left < bird_info.x_offset + BIRD_WIDTH - 5 &&
            dino_bottom > bird_y + 5 &&
            dino_top < bird_y + BIRD_HEIGHT - 5) {
            collided = true;
        }
    }
}

void DinoGameView::draw_current_score() {
    auto style = *ui::Theme::getInstance()->fg_medium;

    if (last_score != score) {
        painter.fill_rectangle({10, 28, 60, 14}, Color::black());
        painter.draw_string({10, 30}, style, score_to_string(score));
        last_score = score;
    }
}

void DinoGameView::draw_high_score() {
    auto style = *ui::Theme::getInstance()->fg_light;
    painter.fill_rectangle({UI_POS_X_RIGHT(90), 28, 90, 18}, Color::black());
    painter.draw_string({UI_POS_X_RIGHT(90), 30}, style, "HI " + score_to_string(highScore));
}

void DinoGameView::jump() {
    if (!jumping) {
        jumping = true;
        // If we're ducking when we jump, stand up
        if (ducking) {
            stand();
        }
    }
}

void DinoGameView::duck() {
    if (!jumping) {
        ducking = true;
        duck_timer = 60;  // 1 second at 60 FPS
    }
}

void DinoGameView::stand() {
    ducking = false;
    duck_timer = 0;
}

void DinoGameView::step() {
    ++steps;
}

bool DinoGameView::on_key(const KeyEvent key) {
    if (key == KeyEvent::Select) {
        if (game_state == GameState::MENU) {
            game_state = GameState::PLAYING;
            new_game();
            draw_high_score();
        } else if (game_state == GameState::PLAYING && !collided) {
            jump();
        } else if (game_state == GameState::GAME_OVER || collided) {
            game_state = GameState::PLAYING;  // Restart immediately
            new_game();
            draw_high_score();
        }
    } else if (key == KeyEvent::Down) {
        if (game_state == GameState::PLAYING && !collided) {
            duck();
        }
    } else if (key == KeyEvent::Up) {
        if (game_state == GameState::PLAYING && !collided) {
            stand();
        }
    }

    set_dirty();
    return true;
}

bool DinoGameView::on_encoder(const EncoderEvent delta) {
    (void)delta;
    return true;
}

}  // namespace ui::external_app::dinogame