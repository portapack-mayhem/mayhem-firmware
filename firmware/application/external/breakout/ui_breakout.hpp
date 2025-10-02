/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#ifndef __UI_BREAKOUT_H__
#define __UI_BREAKOUT_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "event_m0.hpp"
#include "message.hpp"
#include "irq_controls.hpp"
#include "random.hpp"
#include "lpc43xx_cpp.hpp"
#include "ui_widget.hpp"
#include <vector>

namespace ui::external_app::breakout {

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

#define wait(x) chThdSleepMilliseconds(x * 1000)

#define PADDLE_WIDTH 40
#define PADDLE_HEIGHT 10
#define BALL_SIZE 8
#define BRICK_HEIGHT 10
#define BRICK_ROWS 5
#define BRICK_GAP 2
#define GAME_AREA_TOP 50
#define BALL_SPEED_INCREASE 0.1f

#define STATE_MENU 0
#define STATE_PLAYING 1
#define STATE_GAME_OVER 3

#define COLOR_BACKGROUND Black
#define COLOR_PADDLE Blue
#define COLOR_BALL White
#define COLOR_BORDER White

class BreakoutView : public View {
   public:
    BreakoutView(NavigationView& nav);
    void on_show() override;

    std::string title() const override { return "Breakout"; }

    void focus() override { dummy.focus(); }
    void paint(Painter& painter) override;
    void frame_sync();
    bool on_encoder(const EncoderEvent event) override;
    bool on_key(KeyEvent key) override;

    void cls();
    void background(int color);
    void fillrect(int x1, int y1, int x2, int y2, int color);
    void rect(int x1, int y1, int x2, int y2, int color);
    void game_timer_check();
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
    void check_game_timer();
    void attach(double delay_sec);
    void detach();

   private:
    const Color pp_colors[9] = {
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
    NavigationView& nav_;
    Painter painter{};

    // Dynamic screen-dependent values
    int SCREEN_WIDTH = 0;
    int SCREEN_HEIGHT = 0;
    int GAME_AREA_BOTTOM = 0;
    int PADDLE_Y = 0;
    int BRICK_WIDTH = 0;
    int BRICK_COLS = 0;
    int PADDLE_SPEED = 0;

    int paddle_x = 0;
    float ball_x = 0;
    float ball_y = 0;
    float ball_dx = 1.5f;
    float ball_dy = -2.0f;
    int score = 0;
    int lives = 3;
    int level = 1;
    int game_state = STATE_MENU;
    bool initialized = false;
    bool ball_attached = true;
    unsigned int brick_count = 0;

    bool menu_blink_state = true;
    uint32_t menu_blink_counter = 0;

    bool gameover_blink_state = true;
    uint32_t gameover_blink_counter = 0;

    std::vector<std::vector<bool>> bricks;
    int brick_colors[BRICK_ROWS];

    bool game_update_callback = false;
    double game_update_timeout = 0;
    uint32_t game_update_counter = 0;

    Button dummy{
        {screen_width, 0, 0, 0},
        ""};

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->frame_sync();
        }};
};

}  // namespace ui::external_app::breakout

#endif /* __UI_BREAKOUT_H__ */