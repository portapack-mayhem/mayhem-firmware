#ifndef __UI_ABOUT_SIMPLE_H__
#define __UI_ABOUT_SIMPLE_H__

#include "ui_widget.hpp"
#include "ui_navigation.hpp"

#include <cstdint>

namespace ui {
class AboutView : public View {
   public:
    AboutView(NavigationView& nav);
    void focus() override;
    std::string title() const override { return "About"; };

   private:
    MenuView menu_view{
        {0, 0, 240, 264},
        true};

    Button button_ok{
        {240 / 3, 270, 240 / 3, 24},
        "OK",
    };
};
}  // namespace ui

#endif /*__UI_ABOUT_SIMPLE_H__*/
