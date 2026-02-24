/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#include "ui_spaceinv.hpp"

namespace ui::external_app::spaceinv {

// Dynamic game constants
static int SCREEN_WIDTH = 240;
static int SCREEN_HEIGHT = 320;
static int PLAYER_WIDTH = 26;
static int PLAYER_HEIGHT = 16;
static int PLAYER_Y = 280;
static int BULLET_WIDTH = 3;
static int BULLET_HEIGHT = 10;
static int BULLET_SPEED = 8;
static int MAX_BULLETS = 3;
static int INVADER_WIDTH = 20;
static int INVADER_HEIGHT = 16;
static int INVADER_ROWS = 5;
static int INVADER_COLS = 6;
static int INVADER_GAP_X = 10;
static int INVADER_GAP_Y = 8;
static int MAX_ENEMY_BULLETS = 3;
static int ENEMY_BULLET_SPEED = 3;
static int INFO_BAR_HEIGHT = 50;
static int INVADER_MOVE_AMOUNT = 8;
static int INVADER_DROP_AMOUNT = 15;

// Game state
static int game_state = 0;  // 0=menu, 1=playing, 2=game over, 3=wave_complete
static bool initialized = false;
static int player_x = 120;
static uint32_t score = 0;
static int lives = 3;
static uint32_t wave = 1;
static uint32_t speed_bonus = 0;
static uint32_t wave_complete_timer = 0;

// Cannon Bullets
static int bullet_x[3] = {0, 0, 0};
static int bullet_y[3] = {0, 0, 0};
static bool bullet_active[3] = {false, false, false};

// Enemy bullets
static int enemy_bullet_x[3] = {0, 0, 0};
static int enemy_bullet_y[3] = {0, 0, 0};
static bool enemy_bullet_active[3] = {false, false, false};
static uint32_t enemy_fire_counter = 0;

// Invaders
static bool invaders[5][11];  // Max possible columns
static int invaders_x = 40;
static int invaders_y = 60;
static int invader_direction = 1;
static uint32_t invader_move_counter = 0;
static uint32_t invader_move_delay = 30;
static bool invader_animation_frame = false;

// Menu state
static bool menu_initialized = false;
static bool game_over_initialized = false;
static bool blink_state = true;
static uint32_t blink_counter = 0;
static int16_t prompt_x = 0;

// Timer
static Ticker game_timer;
static Callback game_update_callback = nullptr;
static uint32_t game_update_timeout = 0;
static uint32_t game_update_counter = 0;

// Painter
static Painter painter;

// For high score access
static SpaceInvadersView* current_instance = nullptr;

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

static void init_dimensions() {
    SCREEN_WIDTH = ui::screen_width;
    SCREEN_HEIGHT = ui::screen_height;

    // Scale player based on screen size
    PLAYER_WIDTH = SCREEN_WIDTH / 9;
    PLAYER_HEIGHT = SCREEN_HEIGHT / 20;
    PLAYER_Y = SCREEN_HEIGHT - 40;

    // Keep invader size fixed for proper pixel art
    INVADER_WIDTH = 20;
    INVADER_HEIGHT = 16;

    // Calculate columns based on available width
    int available_width = SCREEN_WIDTH - 40;  // 20px margins on each side
    INVADER_COLS = available_width / (INVADER_WIDTH + INVADER_GAP_X);
    if (INVADER_COLS > 11) INVADER_COLS = 11;
    if (INVADER_COLS < 6) INVADER_COLS = 6;

    // Center the invader formation
    int formation_width = INVADER_COLS * INVADER_WIDTH + (INVADER_COLS - 1) * INVADER_GAP_X;
    invaders_x = (SCREEN_WIDTH - formation_width) / 2;
    invaders_y = INFO_BAR_HEIGHT + 10;

    // Scale movement
    INVADER_MOVE_AMOUNT = SCREEN_WIDTH / 30;
    INVADER_DROP_AMOUNT = SCREEN_HEIGHT / 21;
}

static void init_invaders() {
    // Initialize invader array based on dynamic columns
    for (int row = 0; row < INVADER_ROWS; row++) {
        for (int col = 0; col < INVADER_COLS; col++) {
            invaders[row][col] = true;
        }
    }

    // Reset position to center
    int formation_width = INVADER_COLS * INVADER_WIDTH + (INVADER_COLS - 1) * INVADER_GAP_X;
    invaders_x = (SCREEN_WIDTH - formation_width) / 2;
    invaders_y = INFO_BAR_HEIGHT + 10;
    invader_direction = 1;
    invader_move_counter = 0;

    // Clear any active enemy bullets
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        enemy_bullet_active[i] = false;
    }

    // Clear any active player bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        bullet_active[i] = false;
    }
}

static void save_high_score() {
    if (current_instance && score > current_instance->highScore) {
        current_instance->highScore = score;
        // Settings are automatically saved when the view is destroyed
    }
}

static void init_menu() {
    painter.fill_rectangle({0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, Color::black());

    auto style_green = *ui::Theme::getInstance()->fg_green;
    auto style_yellow = *ui::Theme::getInstance()->fg_yellow;
    auto style_cyan = *ui::Theme::getInstance()->fg_cyan;
    auto style_red = *ui::Theme::getInstance()->fg_red;

    // Title section
    painter.draw_string({UI_POS_X_CENTER(15), 30}, style_green, "SPACE INVADERS");
    painter.draw_string({UI_POS_X_CENTER(24), 55}, style_yellow, "========================");

    // Instructions box
    int box_width = 22 * 8;
    int box_x = UI_POS_X_CENTER(22);
    painter.fill_rectangle({box_x - 5, 85, box_width + 10, 80}, Color::black());
    painter.draw_rectangle({box_x - 5, 85, box_width + 10, 80}, Color::white());

    painter.draw_string({box_x, 95}, style_cyan, " ROTARY: MOVE SHIP");
    painter.draw_string({box_x, 115}, style_cyan, " SELECT: FIRE");
    painter.draw_string({box_x, 135}, style_cyan, " DEFEND EARTH!");

    // Point values
    painter.draw_string({UI_POS_X_CENTER(16), 175}, style_red, "TOP ROW = 30 PTS");
    painter.draw_string({UI_POS_X_CENTER(16), 190}, style_yellow, "MID ROW = 20 PTS");
    painter.draw_string({UI_POS_X_CENTER(16), 205}, style_green, "BOT ROW = 10 PTS");

    // High score
    if (current_instance) {
        auto style_white = *ui::Theme::getInstance()->fg_light;
        std::string high_score_text = "HIGH SCORE: " + std::to_string(current_instance->highScore);
        painter.draw_string({UI_POS_X_CENTER(high_score_text.length()), 225}, style_white, high_score_text);
    }

    // Set prompt position (will be used for blinking text)
    prompt_x = UI_POS_X_CENTER(12);

    menu_initialized = true;
}

static void init_game_over() {
    painter.fill_rectangle({0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, Color::black());

    auto style_red = *ui::Theme::getInstance()->fg_red;
    auto style_yellow = *ui::Theme::getInstance()->fg_yellow;
    auto style_cyan = *ui::Theme::getInstance()->fg_cyan;
    auto style_white = *ui::Theme::getInstance()->fg_light;

    // Game Over title
    painter.draw_string({UI_POS_X_CENTER(9), 40}, style_red, "GAME OVER");
    painter.draw_string({UI_POS_X_CENTER(24), 65}, style_yellow, "========================");

    // Score box
    int box_width = 22 * 8;
    int box_x = UI_POS_X_CENTER(22);
    painter.fill_rectangle({box_x - 5, 90, box_width + 10, 80}, Color::black());
    painter.draw_rectangle({box_x - 5, 90, box_width + 10, 80}, Color::white());

    // Display scores
    std::string score_text = "SCORE: " + std::to_string(score);
    painter.draw_string({UI_POS_X_CENTER(score_text.length()), 110}, style_cyan, score_text);

    std::string wave_text = "WAVE: " + std::to_string(wave);
    painter.draw_string({UI_POS_X_CENTER(wave_text.length()), 130}, style_cyan, wave_text);

    // High score section
    if (current_instance) {
        if (score > current_instance->highScore) {
            // New high score!
            painter.draw_string({UI_POS_X_CENTER(15), 190}, style_yellow, "NEW HIGH SCORE!");

            std::string high_score_text = std::to_string(score);
            painter.draw_string({UI_POS_X_CENTER(high_score_text.length()), 210}, style_yellow, high_score_text);
        } else {
            // Show existing high score
            painter.draw_string({UI_POS_X_CENTER(11), 190}, style_white, "HIGH SCORE:");

            std::string high_score_text = std::to_string(current_instance->highScore);
            painter.draw_string({UI_POS_X_CENTER(high_score_text.length()), 210}, style_white, high_score_text);
        }
    }

    // Set prompt position for blinking text
    prompt_x = UI_POS_X_CENTER(12);
    game_over_initialized = true;
}

static void draw_player() {
    // Clear the entire player area
    painter.fill_rectangle({0, PLAYER_Y - 4, SCREEN_WIDTH, PLAYER_HEIGHT + 8}, Color::black());

    // Draw the classic Space Invaders cannon
    Color green = Color::green();

    // Scale the drawing based on player size
    int unit_x = PLAYER_WIDTH / 26;
    int unit_y = PLAYER_HEIGHT / 16;

    // Top part - the cannon barrel
    painter.fill_rectangle({player_x + 12 * unit_x, PLAYER_Y, 2 * unit_x, 2 * unit_y}, green);

    // Upper body
    painter.fill_rectangle({player_x + 11 * unit_x, PLAYER_Y + 2 * unit_y, 4 * unit_x, 2 * unit_y}, green);
    painter.fill_rectangle({player_x + 10 * unit_x, PLAYER_Y + 4 * unit_y, 6 * unit_x, 2 * unit_y}, green);

    // Main body
    painter.fill_rectangle({player_x + 2 * unit_x, PLAYER_Y + 6 * unit_y, 22 * unit_x, 10 * unit_y}, green);
    painter.fill_rectangle({player_x + unit_x, PLAYER_Y + 8 * unit_y, 24 * unit_x, 8 * unit_y}, green);
    painter.fill_rectangle({player_x, PLAYER_Y + 10 * unit_y, 26 * unit_x, 6 * unit_y}, green);
}

static void draw_invader(int row, int col) {
    int x = invaders_x + col * (INVADER_WIDTH + INVADER_GAP_X);
    int y = invaders_y + row * (INVADER_HEIGHT + INVADER_GAP_Y);

    // Clear the area first
    painter.fill_rectangle({x, y, INVADER_WIDTH, INVADER_HEIGHT}, Color::black());

    // Different colors and shapes for different rows
    Color color;
    if (row == 0) {
        color = Color::red();  // Top row - 30 points
    } else if (row == 1 || row == 2) {
        color = Color::yellow();  // Middle rows - 20 points
    } else {
        color = Color::green();  // Bottom rows - 10 points
    }

    // Draw different invader types based on row
    if (row == 0) {
        // Top row - squid-like invader (30 points)
        if (invader_animation_frame) {
            // Frame 1 - arms down
            painter.draw_hline({x + 8, y + 2}, 4, color);
            painter.draw_hline({x + 6, y + 3}, 8, color);
            painter.draw_hline({x + 4, y + 4}, 12, color);
            painter.draw_hline({x + 2, y + 5}, 16, color);
            painter.draw_hline({x + 2, y + 6}, 16, color);
            painter.draw_hline({x, y + 7}, 20, color);
            painter.draw_hline({x, y + 8}, 20, color);
            painter.draw_hline({x + 2, y + 9}, 2, color);
            painter.draw_hline({x + 6, y + 9}, 8, color);
            painter.draw_hline({x + 16, y + 9}, 2, color);
            painter.draw_hline({x + 4, y + 10}, 2, color);
            painter.draw_hline({x + 14, y + 10}, 2, color);
        } else {
            // Frame 2 - arms up
            painter.draw_hline({x + 8, y + 2}, 4, color);
            painter.draw_hline({x + 6, y + 3}, 8, color);
            painter.draw_hline({x + 4, y + 4}, 12, color);
            painter.draw_hline({x + 2, y + 5}, 16, color);
            painter.draw_hline({x + 2, y + 6}, 16, color);
            painter.draw_hline({x, y + 7}, 20, color);
            painter.draw_hline({x, y + 8}, 20, color);
            painter.draw_hline({x + 4, y + 9}, 2, color);
            painter.draw_hline({x + 8, y + 9}, 4, color);
            painter.draw_hline({x + 14, y + 9}, 2, color);
            painter.draw_hline({x + 2, y + 10}, 2, color);
            painter.draw_hline({x + 16, y + 10}, 2, color);
        }
    } else if (row == 1 || row == 2) {
        // Middle rows - crab-like invader (20 points)
        if (invader_animation_frame) {
            // Frame 1 - arms in
            painter.draw_hline({x + 4, y + 2}, 2, color);
            painter.draw_hline({x + 14, y + 2}, 2, color);
            painter.draw_hline({x + 2, y + 3}, 2, color);
            painter.draw_hline({x + 6, y + 3}, 8, color);
            painter.draw_hline({x + 16, y + 3}, 2, color);
            painter.draw_hline({x + 2, y + 4}, 16, color);
            painter.draw_hline({x, y + 5}, 6, color);
            painter.draw_hline({x + 8, y + 5}, 4, color);
            painter.draw_hline({x + 14, y + 5}, 6, color);
            painter.draw_hline({x, y + 6}, 20, color);
            painter.draw_hline({x + 2, y + 7}, 16, color);
            painter.draw_hline({x + 4, y + 8}, 2, color);
            painter.draw_hline({x + 14, y + 8}, 2, color);
            painter.draw_hline({x + 2, y + 9}, 4, color);
            painter.draw_hline({x + 14, y + 9}, 4, color);
        } else {
            // Frame 2 - arms out
            painter.draw_hline({x + 4, y + 2}, 2, color);
            painter.draw_hline({x + 14, y + 2}, 2, color);
            painter.draw_hline({x + 2, y + 3}, 2, color);
            painter.draw_hline({x + 6, y + 3}, 8, color);
            painter.draw_hline({x + 16, y + 3}, 2, color);
            painter.draw_hline({x + 2, y + 4}, 16, color);
            painter.draw_hline({x, y + 5}, 6, color);
            painter.draw_hline({x + 8, y + 5}, 4, color);
            painter.draw_hline({x + 14, y + 5}, 6, color);
            painter.draw_hline({x, y + 6}, 20, color);
            painter.draw_hline({x + 2, y + 7}, 16, color);
            painter.draw_hline({x + 4, y + 8}, 2, color);
            painter.draw_hline({x + 14, y + 8}, 2, color);
            painter.draw_hline({x, y + 9}, 2, color);
            painter.draw_hline({x + 6, y + 9}, 2, color);
            painter.draw_hline({x + 12, y + 9}, 2, color);
            painter.draw_hline({x + 18, y + 9}, 2, color);
        }
    } else {
        // Bottom rows - octopus-like invader (10 points)
        if (invader_animation_frame) {
            // Frame 1
            painter.draw_hline({x + 6, y + 2}, 8, color);
            painter.draw_hline({x + 4, y + 3}, 12, color);
            painter.draw_hline({x + 2, y + 4}, 16, color);
            painter.draw_hline({x, y + 5}, 20, color);
            painter.draw_hline({x, y + 6}, 20, color);
            painter.draw_hline({x + 4, y + 7}, 4, color);
            painter.draw_hline({x + 12, y + 7}, 4, color);
            painter.draw_hline({x + 2, y + 8}, 4, color);
            painter.draw_hline({x + 8, y + 8}, 4, color);
            painter.draw_hline({x + 14, y + 8}, 4, color);
            painter.draw_hline({x + 4, y + 9}, 2, color);
            painter.draw_hline({x + 14, y + 9}, 2, color);
        } else {
            // Frame 2
            painter.draw_hline({x + 6, y + 2}, 8, color);
            painter.draw_hline({x + 4, y + 3}, 12, color);
            painter.draw_hline({x + 2, y + 4}, 16, color);
            painter.draw_hline({x, y + 5}, 20, color);
            painter.draw_hline({x, y + 6}, 20, color);
            painter.draw_hline({x + 4, y + 7}, 4, color);
            painter.draw_hline({x + 12, y + 7}, 4, color);
            painter.draw_hline({x + 2, y + 8}, 4, color);
            painter.draw_hline({x + 8, y + 8}, 4, color);
            painter.draw_hline({x + 14, y + 8}, 4, color);
            painter.draw_hline({x + 2, y + 9}, 2, color);
            painter.draw_hline({x + 16, y + 9}, 2, color);
        }
    }
}

static void draw_all_invaders() {
    for (int row = 0; row < INVADER_ROWS; row++) {
        for (int col = 0; col < INVADER_COLS; col++) {
            if (invaders[row][col]) {
                draw_invader(row, col);
            }
        }
    }
}

static void clear_all_invaders() {
    for (int row = 0; row < INVADER_ROWS; row++) {
        for (int col = 0; col < INVADER_COLS; col++) {
            if (invaders[row][col]) {
                int x = invaders_x + col * (INVADER_WIDTH + INVADER_GAP_X);
                int y = invaders_y + row * (INVADER_HEIGHT + INVADER_GAP_Y);
                painter.fill_rectangle({x, y, INVADER_WIDTH, INVADER_HEIGHT}, Color::black());
            }
        }
    }
}

static void clear_all_enemy_bullets() {
    // Clear any visible enemy bullets
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (enemy_bullet_active[i]) {
            painter.fill_rectangle({enemy_bullet_x[i], enemy_bullet_y[i], BULLET_WIDTH, BULLET_HEIGHT}, Color::black());
            enemy_bullet_active[i] = false;
        }
    }
}

static void fire_enemy_bullet() {
    // Find a random invader to fire from
    int active_count = 0;
    for (int row = 0; row < INVADER_ROWS; row++) {
        for (int col = 0; col < INVADER_COLS; col++) {
            if (invaders[row][col]) active_count++;
        }
    }

    if (active_count == 0) return;

    int shooter = rand() % active_count;
    int count = 0;

    for (int row = 0; row < INVADER_ROWS; row++) {
        for (int col = 0; col < INVADER_COLS; col++) {
            if (invaders[row][col]) {
                if (count == shooter) {
                    // Fire from this invader
                    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
                        if (!enemy_bullet_active[i]) {
                            int x = invaders_x + col * (INVADER_WIDTH + INVADER_GAP_X);
                            int y = invaders_y + row * (INVADER_HEIGHT + INVADER_GAP_Y);
                            enemy_bullet_x[i] = x + INVADER_WIDTH / 2;
                            enemy_bullet_y[i] = y + INVADER_HEIGHT;
                            enemy_bullet_active[i] = true;
                            return;
                        }
                    }
                }
                count++;
            }
        }
    }
}

static void update_enemy_bullets() {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (enemy_bullet_active[i]) {
            // Clear old position
            if (enemy_bullet_y[i] != INFO_BAR_HEIGHT - 1) {
                painter.fill_rectangle({enemy_bullet_x[i], enemy_bullet_y[i], BULLET_WIDTH, BULLET_HEIGHT}, Color::black());
            }

            enemy_bullet_y[i] += ENEMY_BULLET_SPEED;

            if (enemy_bullet_y[i] > SCREEN_HEIGHT) {
                enemy_bullet_active[i] = false;
            } else {
                // Draw at new position
                painter.fill_rectangle({enemy_bullet_x[i], enemy_bullet_y[i], BULLET_WIDTH, BULLET_HEIGHT}, Color::red());

                // Check collision with player
                if (enemy_bullet_x[i] >= player_x &&
                    enemy_bullet_x[i] <= player_x + PLAYER_WIDTH &&
                    enemy_bullet_y[i] >= PLAYER_Y &&
                    enemy_bullet_y[i] <= PLAYER_Y + PLAYER_HEIGHT) {
                    // Player hit!
                    enemy_bullet_active[i] = false;
                    lives--;

                    // Update lives display
                    auto style = *ui::Theme::getInstance()->fg_green;
                    painter.fill_rectangle({5, 30, 100, 20}, Color::black());
                    painter.draw_string({5, 30}, style, "Lives: " + std::to_string(lives));

                    if (lives <= 0) {
                        game_state = 2;  // Game over
                        save_high_score();
                    }
                }
            }
        }
    }
}

static void update_invaders() {
    uint32_t adjusted_delay = (speed_bonus >= invader_move_delay) ? 1 : invader_move_delay - speed_bonus;

    if (++invader_move_counter >= adjusted_delay) {
        invader_move_counter = 0;

        // Clear old positions
        clear_all_invaders();

        // Check bounds
        int leftmost = INVADER_COLS;
        int rightmost = -1;

        for (int col = 0; col < INVADER_COLS; col++) {
            for (int row = 0; row < INVADER_ROWS; row++) {
                if (invaders[row][col]) {
                    if (col < leftmost) leftmost = col;
                    if (col > rightmost) rightmost = col;
                }
            }
        }

        bool hit_edge = false;
        if (invader_direction > 0) {
            // Moving right - check rightmost invader with margin
            int right_edge = invaders_x + rightmost * (INVADER_WIDTH + INVADER_GAP_X) + INVADER_WIDTH;
            if (right_edge >= SCREEN_WIDTH - 5) {  // 5px margin
                hit_edge = true;
            }
        } else {
            // Moving left - check leftmost invader with margin
            int left_edge = invaders_x + leftmost * (INVADER_WIDTH + INVADER_GAP_X);
            if (left_edge <= 5) {  // 5px margin
                hit_edge = true;
            }
        }

        // Move invaders
        if (hit_edge) {
            invaders_y += INVADER_DROP_AMOUNT;
            invader_direction = -invader_direction;
        } else {
            invaders_x += invader_direction * INVADER_MOVE_AMOUNT;
        }

        // Toggle animation frame
        invader_animation_frame = !invader_animation_frame;

        // Draw new positions
        draw_all_invaders();
    }

    // Enemy firing logic
    if (!current_instance || !current_instance->easy_mode) {
        if (++enemy_fire_counter >= 120) {  // Fire every 2 seconds at 60fps
            enemy_fire_counter = 0;
            fire_enemy_bullet();
        }
    }
}

static void fire_bullet() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullet_active[i]) {
            bullet_active[i] = true;
            bullet_x[i] = player_x + PLAYER_WIDTH / 2 - BULLET_WIDTH / 2;
            bullet_y[i] = PLAYER_Y - BULLET_HEIGHT;
            break;
        }
    }
}

static void update_bullets() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullet_active[i]) {
            painter.fill_rectangle({bullet_x[i], bullet_y[i], BULLET_WIDTH, BULLET_HEIGHT}, Color::black());

            bullet_y[i] -= BULLET_SPEED;

            if (bullet_y[i] < INFO_BAR_HEIGHT) {
                bullet_active[i] = false;
            } else {
                painter.fill_rectangle({bullet_x[i], bullet_y[i], BULLET_WIDTH, BULLET_HEIGHT}, Color::white());
            }
        }
    }
}

static void check_collisions() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullet_active[i]) continue;

        for (int row = 0; row < INVADER_ROWS; row++) {
            for (int col = 0; col < INVADER_COLS; col++) {
                if (!invaders[row][col]) continue;

                int inv_x = invaders_x + col * (INVADER_WIDTH + INVADER_GAP_X);
                int inv_y = invaders_y + row * (INVADER_HEIGHT + INVADER_GAP_Y);

                // Simple collision check
                if (bullet_x[i] < inv_x + INVADER_WIDTH &&
                    bullet_x[i] + BULLET_WIDTH > inv_x &&
                    bullet_y[i] < inv_y + INVADER_HEIGHT &&
                    bullet_y[i] + BULLET_HEIGHT > inv_y) {
                    // Hit!
                    bullet_active[i] = false;
                    painter.fill_rectangle({bullet_x[i], bullet_y[i], BULLET_WIDTH, BULLET_HEIGHT}, Color::black());

                    invaders[row][col] = false;
                    painter.fill_rectangle({inv_x, inv_y, INVADER_WIDTH, INVADER_HEIGHT}, Color::black());

                    // Score based on row
                    if (row == 0) {
                        score += 30;
                    } else if (row == 1 || row == 2) {
                        score += 20;
                    } else {
                        score += 10;
                    }

                    return;
                }
            }
        }
    }
}

static void check_wave_complete() {
    // Check if all invaders are destroyed
    bool all_destroyed = true;
    for (int row = 0; row < INVADER_ROWS; row++) {
        for (int col = 0; col < INVADER_COLS; col++) {
            if (invaders[row][col]) {
                all_destroyed = false;
                break;
            }
        }
        if (!all_destroyed) break;
    }

    if (all_destroyed) {
        // Next wave!
        wave++;
        speed_bonus = (wave - 1) * 3;            // Each wave is slightly faster
        if (speed_bonus > 15) speed_bonus = 15;  // Cap the speed increase

        // Clear any enemy bullets
        clear_all_enemy_bullets();

        // Set state to wave complete
        game_state = 3;
        wave_complete_timer = 60;  // 1 second

        // Clear screen and show wave message
        painter.fill_rectangle({0, INFO_BAR_HEIGHT + 1, SCREEN_WIDTH, SCREEN_HEIGHT - INFO_BAR_HEIGHT - 1}, Color::black());
        painter.draw_hline({0, INFO_BAR_HEIGHT - 1}, SCREEN_WIDTH, Color::white());

        auto style = *ui::Theme::getInstance()->fg_green;
        std::string wave_text = "WAVE " + std::to_string(wave);
        painter.draw_string({UI_POS_X_CENTER(wave_text.length()), SCREEN_HEIGHT / 2}, style, wave_text);

        return;
    }

    // Check if invaders reached player
    for (int row = 0; row < INVADER_ROWS; row++) {
        for (int col = 0; col < INVADER_COLS; col++) {
            if (invaders[row][col]) {
                int y = invaders_y + row * (INVADER_HEIGHT + INVADER_GAP_Y);
                if (y + INVADER_HEIGHT >= PLAYER_Y) {
                    game_state = 2;  // Game over
                    save_high_score();
                    return;
                }
            }
        }
    }
}

void game_timer_check() {
    if (game_state == 0) {
        // Menu state
        if (!menu_initialized) {
            init_menu();
        }

        if (++blink_counter >= 30) {
            blink_counter = 0;
            blink_state = !blink_state;

            auto style = *ui::Theme::getInstance()->fg_red;
            int blink_y = 250;  // Position below the high score

            if (blink_state) {
                painter.draw_string({prompt_x, blink_y}, style, "PRESS SELECT");
            } else {
                painter.fill_rectangle({prompt_x, blink_y, 12 * 8, 20}, Color::black());
            }
        }
    } else if (game_state == 1) {
        // Playing state
        update_bullets();
        update_enemy_bullets();
        update_invaders();
        check_collisions();
        check_wave_complete();

        // Update score display
        static uint32_t last_score = 999999;
        if (score != last_score) {
            last_score = score;
            auto style = *ui::Theme::getInstance()->fg_green;
            painter.fill_rectangle({5, 10, 100, 20}, Color::black());
            painter.draw_string({5, 10}, style, "Score: " + std::to_string(score));
            painter.draw_hline({0, INFO_BAR_HEIGHT - 1}, SCREEN_WIDTH, Color::white());
        }

        // Periodically redraw the border line
        static uint32_t border_redraw_counter = 0;
        if (++border_redraw_counter >= 60) {
            border_redraw_counter = 0;
            painter.draw_hline({0, INFO_BAR_HEIGHT - 1}, SCREEN_WIDTH, Color::white());
        }
    } else if (game_state == 2) {
        // Game over state
        if (!game_over_initialized) {
            init_game_over();
        }

        if (++blink_counter >= 30) {
            blink_counter = 0;
            blink_state = !blink_state;

            auto style = *ui::Theme::getInstance()->fg_red;
            int blink_y = 250;  // Position below high score section

            if (blink_state) {
                painter.draw_string({prompt_x, blink_y}, style, "PRESS SELECT");
            } else {
                painter.fill_rectangle({prompt_x, blink_y, 12 * 8, 20}, Color::black());
            }
        }
    } else if (game_state == 3) {
        // Wave complete state - wait for timer
        if (--wave_complete_timer <= 0) {
            game_state = 1;

            // Clear and redraw game area
            painter.fill_rectangle({0, INFO_BAR_HEIGHT + 1, SCREEN_WIDTH, SCREEN_HEIGHT - INFO_BAR_HEIGHT - 1}, Color::black());

            // Restore UI
            auto style = *ui::Theme::getInstance()->fg_green;
            painter.draw_string({5, 10}, style, "Score: " + std::to_string(score));
            painter.draw_string({5, 30}, style, "Lives: " + std::to_string(lives));
            painter.draw_hline({0, INFO_BAR_HEIGHT - 1}, SCREEN_WIDTH, Color::white());

            // Initialize new wave
            init_invaders();
            draw_all_invaders();
            draw_player();
        }
    }
}

SpaceInvadersView::SpaceInvadersView(NavigationView& nav)
    : nav_{nav} {
    add_children({&dummy, &button_difficulty});

    // Initialize dimensions first
    current_instance = this;
    init_dimensions();

    // Now reposition button with proper centering
    int button_y = SCREEN_HEIGHT - 45;        // 45px from bottom
    int button_x = (SCREEN_WIDTH - 100) / 2;  // Center horizontally
    button_difficulty.set_parent_rect({button_x, button_y, 100, 20});

    game_timer.attach(&game_timer_check, 1.0 / 60.0);

    // Update button text based on loaded setting
    button_difficulty.set_text(easy_mode ? "Mode: EASY" : "Mode: HARD");

    button_difficulty.on_select = [this](Button&) {
        easy_mode = !easy_mode;
        button_difficulty.set_text(easy_mode ? "Mode: EASY" : "Mode: HARD");
    };
}

SpaceInvadersView::~SpaceInvadersView() {
    current_instance = nullptr;
}

void SpaceInvadersView::on_show() {
}

void SpaceInvadersView::paint(Painter& painter) {
    (void)painter;

    if (!initialized) {
        initialized = true;
        game_state = 0;
        menu_initialized = false;
        game_over_initialized = false;
        blink_state = true;
        blink_counter = 0;
        player_x = SCREEN_WIDTH / 2 - PLAYER_WIDTH / 2;
        score = 0;
        lives = 3;
        wave = 1;
        speed_bonus = 0;

        // Initialize arrays
        for (int i = 0; i < MAX_BULLETS; i++) {
            bullet_active[i] = false;
            bullet_x[i] = 0;
            bullet_y[i] = 0;
        }

        for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
            enemy_bullet_active[i] = false;
            enemy_bullet_x[i] = 0;
            enemy_bullet_y[i] = 0;
        }

        init_invaders();
    }
}

void SpaceInvadersView::frame_sync() {
    check_game_timer();
    set_dirty();
}

bool SpaceInvadersView::on_encoder(const EncoderEvent delta) {
    if (game_state == 1) {
        int move_speed = SCREEN_WIDTH / 48;  // Scale movement speed
        if (delta > 0) {
            player_x += move_speed;
            if (player_x > SCREEN_WIDTH - PLAYER_WIDTH) player_x = SCREEN_WIDTH - PLAYER_WIDTH;
            draw_player();
        } else if (delta < 0) {
            player_x -= move_speed;
            if (player_x < 0) player_x = 0;
            draw_player();
        }

        set_dirty();
    }
    return true;
}

bool SpaceInvadersView::on_key(const KeyEvent key) {
    if (key == KeyEvent::Select) {
        if (game_state == 0) {
            // Start game
            game_state = 1;
            button_difficulty.hidden(true);
            score = 0;
            lives = 3;
            wave = 1;
            speed_bonus = 0;
            invader_move_delay = 30;
            enemy_fire_counter = 0;
            init_invaders();

            painter.fill_rectangle({0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, Color::black());
            painter.draw_hline({0, INFO_BAR_HEIGHT - 1}, SCREEN_WIDTH, Color::white());

            auto style = *ui::Theme::getInstance()->fg_green;
            painter.draw_string({5, 10}, style, "Score: 0");
            painter.draw_string({5, 30}, style, "Lives: 3");

            draw_all_invaders();
            draw_player();

            set_dirty();
        } else if (game_state == 1) {
            fire_bullet();
        } else if (game_state == 2) {
            // Return to menu
            game_state = 0;
            menu_initialized = false;
            game_over_initialized = false;
            button_difficulty.hidden(false);
            set_dirty();
        }
    }

    return true;
}

}  // namespace ui::external_app::spaceinv