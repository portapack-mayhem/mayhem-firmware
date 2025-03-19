#ifndef __UI_SPI_TFT_ILI9341_H__
#define __UI_SPI_TFT_ILI9341_H__

ui::Painter painter;

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

#endif /*__UI_SPI_TFT_ILI9341_H__*/