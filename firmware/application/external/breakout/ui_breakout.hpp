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

extern const Color pp_colors[];
extern Painter painter;
extern bool but_RIGHT;
extern bool but_LEFT;
extern bool but_SELECT;

void cls();
void background(int color);
void fillrect(int x1, int y1, int x2, int y2, int color);
void rect(int x1, int y1, int x2, int y2, int color);

#define wait(x) chThdSleepMilliseconds(x * 1000)

using Callback = void (*)(void);

void check_game_timer();

class Ticker {
   public:
    Ticker() = default;
    void attach(Callback func, double delay_sec);
    void detach();
};

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

extern Ticker game_timer;

extern int paddle_x;
extern float ball_x;
extern float ball_y;
extern float ball_dx;
extern float ball_dy;
extern int score;
extern int lives;
extern int level;
extern int game_state;
extern bool initialized;
extern bool ball_attached;
extern unsigned int brick_count;
extern bool bricks[BRICK_ROWS][BRICK_COLS];
extern int brick_colors[BRICK_ROWS];

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

   private:
    bool initialized = false;
    NavigationView& nav_;

    Button dummy{
        {240, 0, 0, 0},
        ""};

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->frame_sync();
        }};
};

}  // namespace ui::external_app::breakout

#endif /* __UI_BREAKOUT_H__ */