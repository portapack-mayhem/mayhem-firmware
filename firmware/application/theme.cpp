
#include "theme.hpp"

namespace ui {

ThemeDefault* Theme::current = nullptr;  // new ThemeDefault();

void Theme::SetTheme(ThemeId theme) {
    if (current != nullptr) delete current;
    switch (theme) {
        case Yellow:
            current = new ThemeYellow();
            break;

        default:
        case DefaultGrey:
            current = new ThemeDefault();
            break;
    }
}

}  // namespace ui