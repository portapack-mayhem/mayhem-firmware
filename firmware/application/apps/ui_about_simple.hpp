#ifndef __UI_ABOUT_SIMPLE_H__
#define __UI_ABOUT_SIMPLE_H__

#include "ui_widget.hpp"
#include "ui_navigation.hpp"

#include <cstdint>

/*
    TODO: Now it is dyn width. There should be an algorithm to fill the menu based on it's size.
*/

namespace ui {
class AboutView : public View {
   public:
    AboutView(NavigationView& nav);
    void focus() override;
    std::string title() const override { return "About"; };

   private:
    virtual bool on_key(const KeyEvent event);
    virtual bool on_encoder(const EncoderEvent event);
    virtual bool on_touch(const TouchEvent event);

    bool interacted{false};
    uint16_t frame_sync_count{0};
    void on_frame_sync();
    MenuView menu_view{
        {0, 0, screen_width, screen_height - 56},
        true};

    Button button_ok{
        {screen_width / 3, screen_height - 50, screen_width / 3, 24},
        "OK",
    };

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->on_frame_sync();
        }};
};
}  // namespace ui

#endif /*__UI_ABOUT_SIMPLE_H__*/
