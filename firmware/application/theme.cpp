
#include "theme.hpp"

namespace ui {

void Theme::SetTheme(ThemeId theme) {
    switch (theme) {
        case Yellow:
            current = themeYellow;
            break;

        default:
        case DefaultGrey:
            current = themeDefault;
            break;
    }
}

}  // namespace ui