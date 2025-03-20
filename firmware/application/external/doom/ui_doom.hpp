/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#ifndef __UI_DOOM_H__
#define __UI_DOOM_H__

#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "message.hpp"

namespace ui::external_app::doom {
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 4ecc9d04 (Put ticker class and pp_colors in hpp file in namespace and remove helper files (#2577))
using Callback = void (*)();

class Ticker {
   public:
    Ticker() {}
    void attach(Callback func, double delay_sec) {
        game_update_callback = func;
        game_update_timeout = delay_sec * 60;
    }
    void detach() {
        game_update_callback = nullptr;
    }

   private:
    Callback game_update_callback = nullptr;
    uint32_t game_update_timeout = 0;
};

extern ui::Painter painter;

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

static const Color pp_colors[] = {
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
<<<<<<< HEAD
=======
>>>>>>> 40cf2b3f (Doom - Mayhem Edition (#2570))
=======
>>>>>>> 4ecc9d04 (Put ticker class and pp_colors in hpp file in namespace and remove helper files (#2577))

class DoomView : public View {
   public:
    DoomView(NavigationView& nav);
    void on_show() override;
    std::string title() const override { return "Doom"; }
    void focus() override { dummy.focus(); }
    void paint(Painter& painter) override;
    void frame_sync();
    bool on_key(const KeyEvent key) override;

   private:
    NavigationView& nav_;
<<<<<<< HEAD
    Button dummy{{screen_width, 0, 0, 0}, ""};
=======
    Button dummy{{240, 0, 0, 0}, ""};
>>>>>>> 40cf2b3f (Doom - Mayhem Edition (#2570))
    bool initialized{false};
    bool prev_velocity_moving{false};
    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->frame_sync();
        }};
};
<<<<<<< HEAD
<<<<<<< HEAD
=======

>>>>>>> 40cf2b3f (Doom - Mayhem Edition (#2570))
=======
>>>>>>> 4ecc9d04 (Put ticker class and pp_colors in hpp file in namespace and remove helper files (#2577))
}  // namespace ui::external_app::doom

#endif /*__UI_DOOM_H__*/