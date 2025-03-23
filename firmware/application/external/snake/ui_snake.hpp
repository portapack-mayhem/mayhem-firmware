/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#ifndef __UI_SNAKE_H__
#define __UI_SNAKE_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "event_m0.hpp"
#include "message.hpp"
#include "irq_controls.hpp"
#include "random.hpp"
#include "lpc43xx_cpp.hpp"
#include "ui_widget.hpp"

namespace ui::external_app::snake {

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

class Ticker {
   public:
    Ticker() = default;
    void attach(Callback func, double delay_sec);
    void detach();
};

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

extern Ticker game_timer;

extern int snake_x[GRID_WIDTH * GRID_HEIGHT];
extern int snake_y[GRID_WIDTH * GRID_HEIGHT];
extern int snake_length;
extern int snake_dx;
extern int snake_dy;
extern int food_x;
extern int food_y;
extern int score;
extern int game_state;
extern bool initialized;

void game_timer_check();
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

class SnakeView : public View {
   public:
    SnakeView(NavigationView& nav);
    void on_show() override;

    std::string title() const override { return "Snake"; }

    void focus() override { dummy.focus(); }
    void paint(Painter& painter) override;
    void frame_sync();
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

}  // namespace ui::external_app::snake

#endif /* __UI_SNAKE_H__ */