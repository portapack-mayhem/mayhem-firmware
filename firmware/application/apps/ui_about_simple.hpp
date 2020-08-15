#ifndef __UI_ABOUT_SIMPLE_H__
#define __UI_ABOUT_SIMPLE_H__

#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_font_fixed_8x16.hpp"

#include <cstdint>

namespace ui
{
    class AboutView : public View
    {
    public:
        AboutView(NavigationView &nav);
        void focus() override;
        std::string title() const override { return "About"; };
        int32_t timer{180};
        short frame{0};

    private:
        void update();

        Console console{
            {0, 10, 240, 240}};

        Button button_ok{
            {240/3, 270, 240/3, 24},
            "OK",
        };

        MessageHandlerRegistration message_handler_update{
            Message::ID::DisplayFrameSync,
            [this](const Message *const) {
                this->update();
            }};
    };
} // namespace ui

#endif /*__UI_ABOUT_SIMPLE_H__*/
