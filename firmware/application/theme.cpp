
#include "theme.hpp"

namespace ui {

void Theme::SetTheme(ThemeId theme) {
    /*
        When you create a new theme, please add it to the settings app, and to the ThemeId enum.
        Down, create a case for that theme, and OVERWRITE EVERY PROP that ANY OTHER THEME MODIFIES!
    */

    switch (theme) {
        case DefaultGrey:

            break;

        default:
            break;
    }
}

// DEFAULT VALUES FOR ALL + DefaultGrey

Style Theme::bg_lightest{
    .font = font::fixed_8x16,
    .background = Color::white(),
    .foreground = Color::black(),
};

Style Theme::bg_lightest_small{
    .font = font::fixed_8x16,
    .background = Color::white(),
    .foreground = Color::black(),
};

Style Theme::bg_light{
    .font = font::fixed_8x16,
    .background = Color::light_grey(),
    .foreground = Color::white(),
};

Style Theme::bg_medium{
    .font = font::fixed_8x16,
    .background = Color::grey(),
    .foreground = Color::white(),
};

Style Theme::bg_dark{
    .font = font::fixed_8x16,
    .background = Color::dark_grey(),
    .foreground = Color::white(),
};

Style Theme::bg_darker{
    .font = font::fixed_8x16,
    .background = Color::darker_grey(),
    .foreground = Color::white(),
};

Style Theme::bg_darkest{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::white(),
};

Style Theme::bg_darkest_small{
    .font = font::fixed_5x8,
    .background = Color::black(),
    .foreground = Color::white(),
};

Style Theme::error_dark{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::red(),
};

Style Theme::warning_dark{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::yellow(),
};

Style Theme::ok_dark{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::green(),
};

Style Theme::fg_dark{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::dark_grey(),
};

Style Theme::fg_medium{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::grey(),
};

Style Theme::fg_light{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::light_grey(),
};

Style Theme::fg_red{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::red(),
};

Style Theme::fg_green{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::green(),
};

Style Theme::fg_yellow{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::yellow(),
};

Style Theme::fg_orange{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::orange(),
};

Style Theme::fg_blue{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::blue(),
};

Style Theme::fg_cyan{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::cyan(),
};

Style Theme::fg_darkcyan{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::dark_cyan(),
};

Style Theme::fg_magenta{
    .font = font::fixed_8x16,
    .background = Color::black(),
    .foreground = Color::magenta(),
};

Style Theme::option_active{
    .font = font::fixed_8x16,
    .background = Color::blue(),
    .foreground = Color::white(),
};

Style Theme::bg_important_small{
    .font = ui::font::fixed_5x8,
    .background = ui::Color::yellow(),
    .foreground = ui::Color::black(),
};

Color Theme::status_active{0, 255, 0};    // green
Color Theme::bg_table_header{0, 0, 255};  // blue

}  // namespace ui