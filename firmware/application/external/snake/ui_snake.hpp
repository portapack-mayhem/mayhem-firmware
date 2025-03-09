/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#ifndef __UI_SNAKE_H__
#define __UI_SNAKE_H__

#include "ui_navigation.hpp"
#include "event_m0.hpp"
#include "message.hpp"
#include "irq_controls.hpp"
#include "random.hpp"
#include "lpc43xx_cpp.hpp"
#include "limits.h"
#include "ui_widget.hpp"

namespace ui::external_app::snake {

class SnakeView : public View {
   public:
    SnakeView(NavigationView& nav);
    void on_show() override;
    std::string title() const override { return "Snake"; };
    void focus() override { dummy.focus(); };
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

#endif /*__UI_SNAKE_H__*/