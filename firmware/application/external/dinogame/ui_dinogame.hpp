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

#ifndef __UI_DINOGAME_H__
#define __UI_DINOGAME_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "event_m0.hpp"
#include "message.hpp"
#include "irq_controls.hpp"
#include "random.hpp"
#include "lpc43xx_cpp.hpp"
#include "ui_widget.hpp"
#include "file.hpp"
#include <string>
#include "app_settings.hpp"

namespace ui::external_app::dinogame {

// Color definitions
enum {
    White,
    Blue,
    Yellow,
    Purple,
    Green,
    Red,
    Maroon,
    Orange,
    Black,
};

// Game states
enum class GameState {
    MENU,
    PLAYING,
    GAME_OVER
};

// Walk styles
enum class WalkStyle {
    WALKING,
    DUCKING
};

// Bird position
enum class BirdPosition {
    UP,
    DOWN
};

// Structures
struct Pterodactyl {
    bool inGame = false;
    BirdPosition y_position = BirdPosition::UP;
    int16_t x_offset = 320;
    int16_t y_offset = 0;
    int8_t y_velocity = 0;
    bool flop = false;
};

// Simple obstacle structure with proper initialization
struct SimpleObstacle {
    bool active;
    int16_t x;
    int16_t last_x;
    uint8_t width;
    uint8_t height;
    uint8_t type;  // 0=small, 1=large, 2=double, 3=triple

    SimpleObstacle()
        : active(false), x(0), last_x(0), width(0), height(0), type(0) {}
};

// External references to sprite data
extern const uint16_t dino_default[];
extern const uint16_t dino_leftstep[];
extern const uint16_t dino_rightstep[];
extern const uint16_t dino_ducking_leftstep[];
extern const uint16_t dino_ducking_rightstep[];
extern const uint16_t dino_gameover[];
extern const uint16_t pterodactyl_upflop[];
extern const uint16_t pterodactyl_downflop[];

// Global painter
extern Painter painter;

// Function declarations
void cls();
void fillrect(int x1, int y1, int x2, int y2, int color);
void rect(int x1, int y1, int x2, int y2, int color);
void check_game_timer();
void game_timer_check();

// Ticker class
using Callback = void (*)(void);

class Ticker {
   public:
    Ticker() = default;
    void attach(Callback func, double delay_sec);
    void detach();
};

// Main view class
class DinoGameView : public View {
   public:
    DinoGameView(NavigationView& nav);
    void on_show() override;

    std::string title() const override { return "Dino Game"; }

    void focus() override { dummy.focus(); }
    void paint(Painter& painter) override;
    void frame_sync();
    bool on_encoder(const EncoderEvent event) override;
    bool on_key(KeyEvent key) override;

    // Public for timer callback
    GameState game_state = GameState::MENU;
    void game_loop();
    void show_menu();

   private:
    bool initialized = false;
    NavigationView& nav_;

    // Dynamic screen dimensions
    int SCREEN_WIDTH = 240;
    int SCREEN_HEIGHT = 320;
    int DINO_WIDTH = 34;
    int DINO_HEIGHT = 36;
    int DINO_DUCK_WIDTH = 45;
    int DINO_DUCK_HEIGHT = 22;
    int BIRD_WIDTH = 34;
    int BIRD_HEIGHT = 27;
    int GROUND_HEIGHT = 10;
    int GAME_AREA_TOP = 78;
    int GAME_AREA_HEIGHT = 160;
    int DINO_X = 30;
    int DINO_Y = 0;       // Will be calculated
    int DINO_DUCK_Y = 0;  // Will be calculated
    int BIRD_Y_UP = 0;    // Will be calculated
    int BIRD_Y_DOWN = 0;  // Will be calculated
    int JUMP_MAX_HEIGHT = 70;
    int JUMP_SPEED = 3;
    int GAME_SPEED_BASE = 3;
    int MIN_OBSTACLE_DISTANCE = 300;
    int MAX_OBSTACLE_DISTANCE = 600;

    // Sprite constants
    static constexpr uint16_t SPRITE_COLOR = 0x528A;
    static constexpr uint16_t TRANSPARENT_COLOR = 0xFFFF;

    // Game variables
    static constexpr uint8_t MAX_OBSTACLES = 1;
    SimpleObstacle obstacles[MAX_OBSTACLES];
    Pterodactyl bird_info;

    int16_t jumpHeight = 0;
    uint8_t ground_offset = 0;
    uint8_t speed_modifier = 0;
    bool runstate = false;
    bool jumping = false;
    bool falling = false;
    bool collided = false;
    bool ducking = false;
    bool displayedGameOver = false;
    bool last_ducking = false;
    bool last_runstate = false;

    uint32_t steps = 0;
    uint32_t score = 0;
    uint32_t highScore = 0;
    uint32_t last_score = 999999;  // Initialize to impossible value
    int32_t next_obstacle_distance = 100;
    int16_t last_obstacle_x = 320;  // Track position of last spawned obstacle
    int32_t obstacle_spawn_timer = 0;

    // Position tracking for minimal redraw
    int16_t last_dino_y = 0;
    int16_t last_bird_x = -1;
    int16_t last_bird_y = -1;
    uint8_t duck_timer = 0;

    // Menu animation
    bool menu_initialized = false;
    bool blink_state = true;
    uint32_t blink_counter = 0;

    // Current Score
    bool score_drawn = false;

    // Game timer
    Ticker game_timer;

    // Private methods
    void init_dimensions();
    void init_game();
    void new_game();
    void update_obstacles();
    void spawn_obstacle();
    void draw_screen();
    void draw_ground();
    void draw_obstacle(const SimpleObstacle& obstacle);
    void clear_obstacle_area(int x, int width, int height);
    void draw_dino_sprite(int x, int y, const uint16_t* sprite);
    void draw_bird_sprite(int x, int y, const uint16_t* sprite);
    void draw_dino_at(int x, int y, bool is_ducking, bool run_frame);
    void manage_bird();
    void draw_score();
    void draw_high_score();
    void draw_current_score();
    void show_game_over();
    void jump();
    void duck();
    void stand();
    void check_collision();
    void step();
    uint32_t get_steps() { return steps; }
    std::string score_to_string(uint32_t score);

    bool easy_mode = false;

    Button button_difficulty{
        {70, 195, 100, 20},
        "Mode: HARD"};

    app_settings::SettingsManager settings_{
        "dinogame",
        app_settings::Mode::NO_RF,
        {{"highscore"sv, &highScore},
         {"easy_mode"sv, &easy_mode}}};

    Button dummy{
        {240, 0, 0, 0},
        ""};

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->frame_sync();
        }};
};

}  // namespace ui::external_app::dinogame

#endif /* __UI_DINOGAME_H__ */