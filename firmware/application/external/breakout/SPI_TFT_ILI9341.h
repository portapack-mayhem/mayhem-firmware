/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#ifndef __UI_SPI_TFT_ILI9341_H__
#define __UI_SPI_TFT_ILI9341_H__

ui::Painter painter;

static int bg_color;

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

static void claim(__FILE* x) {
    (void)x;
};

static void cls() {
    painter.fill_rectangle({0, 0, portapack::display.width(), portapack::display.height()}, Color::black());
};

static void background(int color) {
    bg_color = color;
};

static void set_orientation(int x) {
    (void)x;
};

static void set_font(unsigned char* x) {
    (void)x;
};

static void fillrect(int x1, int y1, int x2, int y2, int color) {
    painter.fill_rectangle({x1, y1, x2 - x1, y2 - y1}, pp_colors[color]);
};

static void rect(int x1, int y1, int x2, int y2, int color) {
    painter.draw_rectangle({x1, y1, x2 - x1, y2 - y1}, pp_colors[color]);
};

#endif /*__UI_SPI_TFT_ILI9341_H__*/
