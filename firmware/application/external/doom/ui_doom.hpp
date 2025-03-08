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
    Button dummy{{240, 0, 0, 0}, ""};
    bool initialized{false};
    bool prev_velocity_moving{false};
    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->frame_sync();
        }};
};

}  // namespace ui::external_app::doom

#endif /*__UI_DOOM_H__*/