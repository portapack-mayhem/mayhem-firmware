/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#ifndef __UI_SNAKE_H__
#define __UI_SNAKE_H__

<<<<<<< HEAD
<<<<<<< HEAD
#include "ui.hpp"
=======
>>>>>>> eb50b790 (Snake (#2549))
=======
#include "ui.hpp"
>>>>>>> 139ade06 (Combine cpp, move helpers to hpp (#2584))
#include "ui_navigation.hpp"
#include "event_m0.hpp"
#include "message.hpp"
#include "irq_controls.hpp"
#include "random.hpp"
#include "lpc43xx_cpp.hpp"
<<<<<<< HEAD
<<<<<<< HEAD
=======
#include "limits.h"
>>>>>>> eb50b790 (Snake (#2549))
=======
>>>>>>> 139ade06 (Combine cpp, move helpers to hpp (#2584))
#include "ui_widget.hpp"

namespace ui::external_app::snake {

<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 139ade06 (Combine cpp, move helpers to hpp (#2584))
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

<<<<<<< HEAD
<<<<<<< HEAD
#define wait(x) chThdSleepMilliseconds(x * 1000)

#define SNAKE_SIZE 10
#define INFO_BAR_HEIGHT 25
#define GAME_AREA_TOP (INFO_BAR_HEIGHT + 1)
=======
extern const Color pp_colors[];
extern Painter painter;
extern bool but_RIGHT;
extern bool but_LEFT;
extern bool but_SELECT;

void cls();
void background(int color);
void fillrect(int x1, int y1, int x2, int y2, int color);
void rect(int x1, int y1, int x2, int y2, int color);

=======
>>>>>>> a1d7cf2b (Prepare for display orientation part 1 (#2661))
#define wait(x) chThdSleepMilliseconds(x * 1000)

#define SNAKE_SIZE 10
#define INFO_BAR_HEIGHT 25
#define GAME_AREA_TOP (INFO_BAR_HEIGHT + 1)
<<<<<<< HEAD
#define GAME_AREA_HEIGHT (SCREEN_HEIGHT - INFO_BAR_HEIGHT - 2)
#define GRID_WIDTH ((SCREEN_WIDTH - 2) / SNAKE_SIZE)
#define GRID_HEIGHT (GAME_AREA_HEIGHT / SNAKE_SIZE)
>>>>>>> 139ade06 (Combine cpp, move helpers to hpp (#2584))
=======
>>>>>>> a1d7cf2b (Prepare for display orientation part 1 (#2661))
#define STATE_MENU 0
#define STATE_PLAYING 1
#define STATE_GAME_OVER 2

#define COLOR_BACKGROUND Black
#define COLOR_SNAKE Green
#define COLOR_FOOD Red
#define COLOR_BORDER White

<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> eb50b790 (Snake (#2549))
=======
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

>>>>>>> 139ade06 (Combine cpp, move helpers to hpp (#2584))
=======
>>>>>>> a1d7cf2b (Prepare for display orientation part 1 (#2661))
class SnakeView : public View {
   public:
    SnakeView(NavigationView& nav);
    void on_show() override;
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 139ade06 (Combine cpp, move helpers to hpp (#2584))

    std::string title() const override { return "Snake"; }

    void focus() override { dummy.focus(); }
<<<<<<< HEAD
=======
    std::string title() const override { return "Snake"; };
    void focus() override { dummy.focus(); };
>>>>>>> eb50b790 (Snake (#2549))
=======
>>>>>>> 139ade06 (Combine cpp, move helpers to hpp (#2584))
    void paint(Painter& painter) override;
    void frame_sync();
    bool on_key(KeyEvent key) override;

<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> a1d7cf2b (Prepare for display orientation part 1 (#2661))
    void cls();
    void background(int color);
    void fillrect(int x1, int y1, int x2, int y2, int color);
    void rect(int x1, int y1, int x2, int y2, int color);
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
    void check_game_timer();

    void attach(double delay_sec);
    void detach();

<<<<<<< HEAD
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

    std::vector<int> snake_x{};  //[GRID_WIDTH * GRID_HEIGHT];
    std::vector<int> snake_y{};  //[GRID_WIDTH * GRID_HEIGHT];
    int snake_length = 1;
    int snake_dx = 1;
    int snake_dy = 0;
    int food_x = 0, food_y = 0;
    int score = 0;
    int game_state = STATE_MENU;
    bool initialized = false;

    int SCREEN_WIDTH = 0;
    int SCREEN_HEIGHT = 0;
    int GAME_AREA_HEIGHT = 0;  //(SCREEN_HEIGHT - INFO_BAR_HEIGHT - 2);
    int GRID_WIDTH = 0;        // ((SCREEN_WIDTH - 2) / SNAKE_SIZE);
    int GRID_HEIGHT = 0;       //(GAME_AREA_HEIGHT / SNAKE_SIZE);

    bool game_update_callback = false;
    double game_update_timeout = 0;
    uint32_t game_update_counter = 0;

    Button dummy{
        {screen_width, 0, 0, 0},
        ""};

=======
=======
>>>>>>> a1d7cf2b (Prepare for display orientation part 1 (#2661))
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

    std::vector<int> snake_x{};  //[GRID_WIDTH * GRID_HEIGHT];
    std::vector<int> snake_y{};  //[GRID_WIDTH * GRID_HEIGHT];
    int snake_length = 1;
    int snake_dx = 1;
    int snake_dy = 0;
    int food_x = 0, food_y = 0;
    int score = 0;
    int game_state = STATE_MENU;
    bool initialized = false;

    int SCREEN_WIDTH = 0;
    int SCREEN_HEIGHT = 0;
    int GAME_AREA_HEIGHT = 0;  //(SCREEN_HEIGHT - INFO_BAR_HEIGHT - 2);
    int GRID_WIDTH = 0;        // ((SCREEN_WIDTH - 2) / SNAKE_SIZE);
    int GRID_HEIGHT = 0;       //(GAME_AREA_HEIGHT / SNAKE_SIZE);

    bool game_update_callback = false;
    double game_update_timeout = 0;
    uint32_t game_update_counter = 0;

    Button dummy{
        {screen_width, 0, 0, 0},
        ""};
<<<<<<< HEAD
>>>>>>> eb50b790 (Snake (#2549))
=======

>>>>>>> 139ade06 (Combine cpp, move helpers to hpp (#2584))
    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->frame_sync();
        }};
};

}  // namespace ui::external_app::snake

<<<<<<< HEAD
<<<<<<< HEAD
#endif /* __UI_SNAKE_H__ */
=======
#endif /*__UI_SNAKE_H__*/
>>>>>>> eb50b790 (Snake (#2549))
=======
#endif /* __UI_SNAKE_H__ */
>>>>>>> 139ade06 (Combine cpp, move helpers to hpp (#2584))
