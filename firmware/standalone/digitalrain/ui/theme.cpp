#include "theme.hpp"

namespace ui {

ThemeTemplate* Theme::current = nullptr;

ThemeTemplate* Theme::getInstance() {
    if (current == nullptr) SetTheme(DefaultGrey);
    return Theme::current;
}

void Theme::destroy() {
    if (current != nullptr) {
        delete current;
        current = nullptr;
    }
}

void Theme::SetTheme(ThemeId theme) {
    if (current != nullptr) delete current;
    switch (theme) {
        case Yellow:
            current = new ThemeYellow();
            break;
        case Aqua:
            current = new ThemeAqua();
            break;
        case Green:
            current = new ThemeGreen();
            break;
        case Red:
            current = new ThemeRed();
            break;
        case Dark:
            current = new ThemeDark();
            break;
        case DefaultGrey:
        default:
            current = new ThemeDefault();
            break;
    }
}

ThemeTemplate::~ThemeTemplate() {
    delete bg_lightest;
    delete bg_lightest_small;
    delete bg_light;
    delete bg_medium;
    delete bg_dark;
    delete bg_darker;
    delete bg_darkest;
    delete bg_darkest_small;
    delete bg_important_small;
    delete error_dark;
    delete warning_dark;
    delete ok_dark;
    delete fg_dark;
    delete fg_medium;
    delete fg_light;
    delete fg_red;
    delete fg_green;
    delete fg_yellow;
    delete fg_orange;
    delete fg_blue;
    delete fg_cyan;
    delete fg_darkcyan;
    delete fg_magenta;
    delete option_active;
    delete status_active;  // green, the status bar icons when active
    delete bg_table_header;
}

ThemeYellow::ThemeYellow() {
    bg_lightest = new Style{
        .font = font::fixed_8x16(),
        .background = {255, 255, 204},
        .foreground = Color::black(),
    };
    bg_lightest_small = new Style{
        .font = font::fixed_5x8(),
        .background = {255, 255, 204},
        .foreground = Color::black(),
    };
    bg_light = new Style{
        .font = font::fixed_8x16(),
        .background = {255, 255, 102},
        .foreground = Color::white(),
    };
    bg_medium = new Style{
        .font = font::fixed_8x16(),
        .background = {204, 204, 0},
        .foreground = Color::white(),
    };
    bg_dark = new Style{
        .font = font::fixed_8x16(),
        .background = {153, 153, 0},
        .foreground = Color::white(),
    };
    bg_darker = new Style{
        .font = font::fixed_8x16(),
        .background = {102, 102, 0},
        .foreground = Color::white(),
    };

    bg_darkest = new Style{
        .font = font::fixed_8x16(),
        .background = {31, 31, 0},
        .foreground = Color::white(),
    };
    bg_darkest_small = new Style{
        .font = font::fixed_5x8(),
        .background = {31, 31, 0},
        .foreground = Color::white(),
    };

    bg_important_small = new Style{
        .font = font::fixed_5x8(),
        .background = Color::yellow(),
        .foreground = {31, 31, 0},
    };

    error_dark = new Style{
        .font = font::fixed_8x16(),
        .background = {31, 31, 0},
        .foreground = Color::red(),
    };
    warning_dark = new Style{
        .font = font::fixed_8x16(),
        .background = {31, 31, 0},
        .foreground = Color::yellow(),
    };
    ok_dark = new Style{
        .font = font::fixed_8x16(),
        .background = {31, 31, 0},
        .foreground = Color::green(),
    };

    fg_dark = new Style{
        .font = font::fixed_8x16(),
        .background = {31, 31, 0},
        .foreground = {153, 153, 0},
    };
    fg_medium = new Style{
        .font = font::fixed_8x16(),
        .background = {31, 31, 0},
        .foreground = {204, 204, 0},
    };
    fg_light = new Style{
        .font = font::fixed_8x16(),
        .background = {31, 31, 0},
        .foreground = Color::light_grey(),
    };

    fg_red = new Style{
        .font = font::fixed_8x16(),
        .background = {31, 31, 0},
        .foreground = Color::red(),
    };
    fg_green = new Style{
        .font = font::fixed_8x16(),
        .background = {31, 31, 0},
        .foreground = Color::green(),
    };
    fg_yellow = new Style{
        .font = font::fixed_8x16(),
        .background = {31, 31, 0},
        .foreground = Color::yellow(),
    };
    fg_orange = new Style{
        .font = font::fixed_8x16(),
        .background = {31, 31, 0},
        .foreground = Color::orange(),
    };
    fg_blue = new Style{
        .font = font::fixed_8x16(),
        .background = {31, 31, 0},
        .foreground = Color::blue(),
    };
    fg_cyan = new Style{
        .font = font::fixed_8x16(),
        .background = {31, 31, 0},
        .foreground = Color::cyan(),
    };
    fg_darkcyan = new Style{
        .font = font::fixed_8x16(),
        .background = {31, 31, 0},
        .foreground = Color::dark_cyan(),
    };
    fg_magenta = new Style{
        .font = font::fixed_8x16(),
        .background = {31, 31, 0},
        .foreground = Color::magenta(),
    };

    option_active = new Style{
        .font = font::fixed_8x16(),
        .background = Color::orange(),
        .foreground = Color::white(),
    };

    status_active = new Color{0, 255, 0};  // green, the status bar icons when active

    bg_table_header = new Color{205, 205, 0};
}

ThemeAqua::ThemeAqua() {
    bg_lightest = new Style{
        .font = font::fixed_8x16(),
        .background = {204, 255, 255},
        .foreground = Color::black(),
    };
    bg_lightest_small = new Style{
        .font = font::fixed_5x8(),
        .background = {204, 255, 255},
        .foreground = Color::black(),
    };
    bg_light = new Style{
        .font = font::fixed_8x16(),
        .background = {102, 255, 255},
        .foreground = Color::white(),
    };
    bg_medium = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 144, 200},
        .foreground = Color::white(),
    };
    bg_dark = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 153, 153},
        .foreground = Color::white(),
    };
    bg_darker = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 102, 102},
        .foreground = Color::white(),
    };

    bg_darkest = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 31, 31},
        .foreground = Color::white(),
    };
    bg_darkest_small = new Style{
        .font = font::fixed_5x8(),
        .background = {0, 31, 31},
        .foreground = Color::white(),
    };

    bg_important_small = new Style{
        .font = font::fixed_5x8(),
        .background = Color::yellow(),
        .foreground = {0, 31, 31},
    };

    error_dark = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 31, 31},
        .foreground = Color::red(),
    };
    warning_dark = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 31, 31},
        .foreground = Color::yellow(),
    };
    ok_dark = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 31, 31},
        .foreground = Color::green(),
    };

    fg_dark = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 31, 31},
        .foreground = {0, 153, 153},
    };
    fg_medium = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 31, 31},
        .foreground = {0, 204, 204},
    };
    fg_light = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 31, 31},
        .foreground = Color::light_grey(),
    };

    fg_red = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 31, 31},
        .foreground = Color::red(),
    };
    fg_green = new Style{
        .font = font::fixed_8x16(),
        .background = {31, 31, 0},
        .foreground = Color::green(),
    };
    fg_yellow = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 31, 31},
        .foreground = Color::yellow(),
    };
    fg_orange = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 31, 31},
        .foreground = Color::orange(),
    };
    fg_blue = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 31, 31},
        .foreground = Color::blue(),
    };
    fg_cyan = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 31, 31},
        .foreground = Color::cyan(),
    };
    fg_darkcyan = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 31, 31},
        .foreground = Color::dark_cyan(),
    };
    fg_magenta = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 31, 31},
        .foreground = Color::magenta(),
    };

    option_active = new Style{
        .font = font::fixed_8x16(),
        .background = Color::blue(),
        .foreground = Color::white(),
    };

    status_active = new Color{0, 255, 0};  // green, the status bar icons when active

    bg_table_header = new Color{0, 205, 205};
}

ThemeDefault::ThemeDefault() {
    bg_lightest = new Style{
        .font = font::fixed_8x16(),
        .background = Color::white(),
        .foreground = Color::black(),
    };
    bg_lightest_small = new Style{
        .font = font::fixed_5x8(),
        .background = Color::white(),
        .foreground = Color::black(),
    };
    bg_light = new Style{
        .font = font::fixed_8x16(),
        .background = Color::light_grey(),
        .foreground = Color::white(),
    };
    bg_medium = new Style{
        .font = font::fixed_8x16(),
        .background = Color::grey(),
        .foreground = Color::white(),
    };
    bg_dark = new Style{
        .font = font::fixed_8x16(),
        .background = Color::dark_grey(),
        .foreground = Color::white(),
    };
    bg_darker = new Style{
        .font = font::fixed_8x16(),
        .background = Color::darker_grey(),
        .foreground = Color::white(),
    };

    bg_darkest = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = Color::white(),
    };
    bg_darkest_small = new Style{
        .font = font::fixed_5x8(),
        .background = Color::black(),
        .foreground = Color::white(),
    };

    bg_important_small = new Style{
        .font = font::fixed_5x8(),
        .background = Color::yellow(),
        .foreground = Color::black(),
    };

    error_dark = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = Color::red(),
    };
    warning_dark = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = Color::yellow(),
    };
    ok_dark = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = Color::green(),
    };

    fg_dark = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = Color::dark_grey(),
    };
    fg_medium = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = Color::grey(),
    };
    fg_light = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = Color::light_grey(),
    };

    fg_red = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = Color::red(),
    };
    fg_green = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = Color::green(),
    };
    fg_yellow = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = Color::yellow(),
    };
    fg_orange = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = Color::orange(),
    };
    fg_blue = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = Color::blue(),
    };
    fg_cyan = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = Color::cyan(),
    };
    fg_darkcyan = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = Color::dark_cyan(),
    };
    fg_magenta = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = Color::magenta(),
    };

    option_active = new Style{
        .font = font::fixed_8x16(),
        .background = Color::blue(),
        .foreground = Color::white(),
    };

    status_active = new Color{0, 255, 0};  // green, the status bar icons when active
    bg_table_header = new Color{0, 0, 255};
}

ThemeGreen::ThemeGreen() {
    bg_lightest = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 245, 29},
        .foreground = Color::black(),
    };
    bg_lightest_small = new Style{
        .font = font::fixed_5x8(),
        .background = {0, 245, 29},
        .foreground = Color::black(),
    };
    bg_light = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 212, 25},
        .foreground = Color::white(),
    };
    bg_medium = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 143, 17},
        .foreground = Color::white(),
    };
    bg_dark = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 99, 12},
        .foreground = Color::white(),
    };
    bg_darker = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 79, 9},
        .foreground = Color::white(),
    };

    bg_darkest = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 33, 4},
        .foreground = Color::white(),
    };
    bg_darkest_small = new Style{
        .font = font::fixed_5x8(),
        .background = {0, 33, 4},
        .foreground = Color::white(),
    };

    bg_important_small = new Style{
        .font = font::fixed_5x8(),
        .background = Color::yellow(),
        .foreground = {0, 33, 4},
    };

    error_dark = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 33, 4},
        .foreground = Color::red(),
    };
    warning_dark = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 33, 4},
        .foreground = Color::yellow(),
    };
    ok_dark = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 33, 4},
        .foreground = Color::green(),
    };

    fg_dark = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 33, 4},
        .foreground = {0, 99, 12},
    };
    fg_medium = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 33, 4},
        .foreground = {0, 143, 17},
    };
    fg_light = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 33, 4},
        .foreground = Color::light_grey(),
    };

    fg_red = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 33, 4},
        .foreground = Color::red(),
    };
    fg_green = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 33, 4},
        .foreground = Color::green(),
    };
    fg_yellow = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 33, 4},
        .foreground = Color::yellow(),
    };
    fg_orange = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 33, 4},
        .foreground = Color::orange(),
    };
    fg_blue = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 33, 4},
        .foreground = Color::blue(),
    };
    fg_cyan = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 33, 4},
        .foreground = Color::cyan(),
    };
    fg_darkcyan = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 33, 4},
        .foreground = Color::dark_cyan(),
    };
    fg_magenta = new Style{
        .font = font::fixed_8x16(),
        .background = {0, 33, 4},
        .foreground = Color::magenta(),
    };

    option_active = new Style{
        .font = font::fixed_8x16(),
        .background = Color::orange(),
        .foreground = Color::white(),
    };

    status_active = new Color{0, 255, 0};  // green, the status bar icons when active

    bg_table_header = new Color{0, 205, 30};
}

ThemeRed::ThemeRed() {
    bg_lightest = new Style{
        .font = font::fixed_8x16(),
        .background = {245, 29, 0},
        .foreground = Color::black(),
    };
    bg_lightest_small = new Style{
        .font = font::fixed_5x8(),
        .background = {245, 29, 0},
        .foreground = Color::black(),
    };
    bg_light = new Style{
        .font = font::fixed_8x16(),
        .background = {212, 25, 0},
        .foreground = Color::white(),
    };
    bg_medium = new Style{
        .font = font::fixed_8x16(),
        .background = {143, 17, 0},
        .foreground = Color::white(),
    };
    bg_dark = new Style{
        .font = font::fixed_8x16(),
        .background = {99, 12, 0},
        .foreground = Color::white(),
    };
    bg_darker = new Style{
        .font = font::fixed_8x16(),
        .background = {79, 9, 0},
        .foreground = Color::white(),
    };

    bg_darkest = new Style{
        .font = font::fixed_8x16(),
        .background = {33, 4, 0},
        .foreground = Color::white(),
    };
    bg_darkest_small = new Style{
        .font = font::fixed_5x8(),
        .background = {33, 4, 0},
        .foreground = Color::white(),
    };

    bg_important_small = new Style{
        .font = font::fixed_5x8(),
        .background = Color::yellow(),
        .foreground = {33, 4, 0},
    };

    error_dark = new Style{
        .font = font::fixed_8x16(),
        .background = {33, 4, 0},
        .foreground = Color::red(),
    };
    warning_dark = new Style{
        .font = font::fixed_8x16(),
        .background = {33, 4, 0},
        .foreground = Color::yellow(),
    };
    ok_dark = new Style{
        .font = font::fixed_8x16(),
        .background = {33, 4, 0},
        .foreground = Color::green(),
    };

    fg_dark = new Style{
        .font = font::fixed_8x16(),
        .background = {33, 4, 0},
        .foreground = {99, 12, 0},
    };
    fg_medium = new Style{
        .font = font::fixed_8x16(),
        .background = {33, 4, 0},
        .foreground = {143, 17, 0},
    };
    fg_light = new Style{
        .font = font::fixed_8x16(),
        .background = {33, 4, 0},
        .foreground = Color::light_grey(),
    };

    fg_red = new Style{
        .font = font::fixed_8x16(),
        .background = {33, 4, 0},
        .foreground = Color::red(),
    };
    fg_green = new Style{
        .font = font::fixed_8x16(),
        .background = {33, 4, 0},
        .foreground = Color::green(),
    };
    fg_yellow = new Style{
        .font = font::fixed_8x16(),
        .background = {33, 4, 0},
        .foreground = Color::yellow(),
    };
    fg_orange = new Style{
        .font = font::fixed_8x16(),
        .background = {33, 4, 0},
        .foreground = Color::orange(),
    };
    fg_blue = new Style{
        .font = font::fixed_8x16(),
        .background = {33, 4, 0},
        .foreground = Color::blue(),
    };
    fg_cyan = new Style{
        .font = font::fixed_8x16(),
        .background = {33, 4, 0},
        .foreground = Color::cyan(),
    };
    fg_darkcyan = new Style{
        .font = font::fixed_8x16(),
        .background = {33, 4, 0},
        .foreground = Color::dark_cyan(),
    };
    fg_magenta = new Style{
        .font = font::fixed_8x16(),
        .background = {33, 4, 0},
        .foreground = Color::magenta(),
    };

    option_active = new Style{
        .font = font::fixed_8x16(),
        .background = Color::orange(),
        .foreground = Color::white(),
    };

    status_active = new Color{0, 255, 0};  // green, the status bar icons when active

    bg_table_header = new Color{205, 30, 0};
}

ThemeDark::ThemeDark() {
    bg_lightest = new Style{
        .font = font::fixed_8x16(),
        .background = {32, 32, 32},
        .foreground = Color::white(),
    };
    bg_lightest_small = new Style{
        .font = font::fixed_5x8(),
        .background = {32, 32, 32},
        .foreground = Color::white(),
    };
    bg_light = new Style{
        .font = font::fixed_8x16(),
        .background = {24, 24, 24},
        .foreground = Color::white(),
    };
    bg_medium = new Style{
        .font = font::fixed_8x16(),
        .background = {16, 16, 16},
        .foreground = Color::white(),
    };
    bg_dark = new Style{
        .font = font::fixed_8x16(),
        .background = {8, 8, 8},
        .foreground = Color::white(),
    };
    bg_darker = new Style{
        .font = font::fixed_8x16(),
        .background = {4, 4, 4},
        .foreground = Color::white(),
    };

    bg_darkest = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = Color::white(),
    };
    bg_darkest_small = new Style{
        .font = font::fixed_5x8(),
        .background = Color::black(),
        .foreground = Color::white(),
    };

    bg_important_small = new Style{
        .font = font::fixed_5x8(),
        .background = {64, 64, 64},
        .foreground = Color::white(),
    };

    error_dark = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = Color::red(),
    };
    warning_dark = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = Color::yellow(),
    };
    ok_dark = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = Color::green(),
    };

    fg_dark = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = {96, 96, 96},
    };
    fg_medium = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = {128, 128, 128},
    };
    fg_light = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = Color::white(),
    };

    fg_red = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = Color::red(),
    };
    fg_green = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = Color::green(),
    };
    fg_yellow = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = Color::yellow(),
    };
    fg_orange = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = Color::orange(),
    };
    fg_blue = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = Color::blue(),
    };
    fg_cyan = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = Color::cyan(),
    };
    fg_darkcyan = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = Color::dark_cyan(),
    };
    fg_magenta = new Style{
        .font = font::fixed_8x16(),
        .background = Color::black(),
        .foreground = Color::magenta(),
    };

    option_active = new Style{
        .font = font::fixed_8x16(),
        .background = {64, 64, 64},
        .foreground = Color::white(),
    };

    status_active = new Color{0, 255, 0};

    bg_table_header = new Color{48, 48, 48};
}

}  // namespace ui