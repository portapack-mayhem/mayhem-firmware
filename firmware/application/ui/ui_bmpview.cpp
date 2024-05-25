#include "ui_bmpview.hpp"
#include "portapack.hpp"

bool BMPViewer::load_bmp(const std::filesystem::path& file) {
    if (!bmp.open(file, true)) return false;
    // calc default zoom level to fit screen, and min / max zoom too
    auto rect = screen_rect();
    auto d_height = rect.height();
    auto d_width = rect.width();
    auto b_width = bmp.get_width();
    auto b_height = bmp.get_real_height();
    // aspects
    // if image is smaller then our vp
    auto x_w = d_width / b_width;
    auto x_h = d_height / b_height;
    if (x_w < 1 && x_h < 1) {
        // not zoom in, but zoom out
        x_w = b_width / d_width;
        x_h = b_height / d_height;
        x_w = (127 < x_w) ? 127 : x_w;
        x_h = (127 < x_h) ? 127 : x_h;
        zoom_fit = (x_h > x_w) ? -1 * x_h : -1 * x_w;
    } else {
        x_w = (127 < x_w) ? 127 : x_w;
        x_h = (127 < x_h) ? 127 : x_h;
        zoom_fit = (x_h > x_w) ? x_h : x_w;
    }
    if (zoom_fit > max_zoom) zoom_fit = max_zoom;
    min_zoom = zoom_fit - 3;

    reset_pos();
    return true;
}

bool BMPViewer::move_pos(int32_t delta_x, int32_t delta_y) {
    if (!bmp.is_loaded()) return false;

    auto rect = screen_rect();
    auto d_height = rect.height();
    auto d_width = rect.width();
    auto ocx = cx;  // save old pos
    auto ocy = cy;
    // top left protection
    if (delta_x < 0 && cx <= (uint32_t)(-1 * delta_x))
        cx = 0;
    else
        cx += delta_x;

    if (delta_y < 0 && cy <= (uint32_t)(-1 * delta_y))
        cy = 0;
    else
        cy += delta_y;
    // right  bottom protection
    float zt = zoom < 0 ? -1.0f / (float)zoom : (float)zoom;
    if (zt == 0) zt = 1;
    if (cy + (uint32_t)(d_height / zt) > bmp.get_real_height()) {
        cy = (bmp.get_real_height() < (uint32_t)(d_height / zt)) ? 0 : bmp.get_real_height() - (uint32_t)(d_height / zt);
    }
    if (cx + (uint32_t)(d_width / zt) > bmp.get_width()) {
        cx = (bmp.get_width() < (uint32_t)(d_width / zt)) ? 0 : bmp.get_width() - (uint32_t)(d_width / zt);
    }
    bool ret = !(cx == ocx && ocy == cy);  // was any change?
    if (ret) set_dirty();
    return ret;
}

void BMPViewer::set_zoom(int8_t new_zoom) {
    if (!bmp.is_loaded()) return;
    if (new_zoom > max_zoom) new_zoom = max_zoom;
    if (new_zoom < min_zoom) new_zoom = min_zoom;
    if (new_zoom == 0) new_zoom = 1;
    if (new_zoom == -1) new_zoom = 1;
    zoom = new_zoom;
    auto rect = screen_rect();
    auto d_height = rect.height();
    auto d_width = rect.width();
    if (zoom < 0) {
        mvx = d_width / 3 * (-1.0 * zoom);
        mvy = d_height / 3 * (-1.0 * zoom);
    } else {
        mvx = d_width / zoom / 3;
        mvy = d_height / zoom / 3;
    }
    move_pos(0, 0);  // fix based on zoom, without real move (if not edge case)
    set_dirty();
}

// reads a lint from the bmp's bx, by coordinate to the line that's size is cnt. according to zoom
void BMPViewer::get_line(ui::Color* line, uint32_t bx, uint32_t by, uint32_t cnt) {
    if (!bmp.is_loaded()) return;
    uint32_t last_targetx = 65534;
    for (uint32_t x = 0; x < cnt; x++) {
        uint32_t targetx = (zoom < 0) ? bx + x * -1 * zoom : bx + x / zoom;  // on zoom out could probably avg the pixels, or apply some smoothing, but this is way faster.
        if (last_targetx == targetx) {
            line[x] = line[x - 1];
            continue;
        }
        last_targetx = targetx;
        if (!bmp.seek(targetx, by)) {
            line[x] = Color::white();  // can't seek there
        } else {
            bmp.read_next_px(line[x], false);
        }
    }
}

void BMPViewer::paint(Painter& painter) {
    if (!bmp.is_loaded()) {
        painter.draw_string({48, 24}, *ui::Theme::getInstance()->bg_darkest, "Can't load BMP");
        return;
    }
    // get where i can paint
    auto rect = screen_rect();
    auto d_height = rect.height();
    auto d_width = rect.width();

    uint32_t by = cy;  // we start to read from there
    uint32_t last_by = 65534;
    ui::Color* line = new ui::Color[d_width];
    for (int32_t y = 0; y < d_height; y++) {
        by = cy + ((zoom < 0) ? y * -1 * zoom : y / (int32_t)zoom);
        if (by != last_by) get_line(line, cx, by, d_width);
        last_by = by;
        portapack::display.draw_pixels({rect.left(), rect.top() + y, d_width, 1}, line, d_width);
    }
    delete line;
}

int8_t BMPViewer::get_zoom() {
    return zoom;
}

BMPViewer::BMPViewer(Rect parent_rect)
    : Widget{parent_rect} {
    set_focusable(true);
}

BMPViewer::BMPViewer(Rect parent_rect, const std::filesystem::path& file)
    : Widget{parent_rect} {
    set_focusable(true);
    load_bmp(file);
}

void BMPViewer::on_focus() {
    set_highlighted(true);
}

void BMPViewer::on_blur() {
    set_dirty();
}

void BMPViewer::reset_pos() {
    if (!bmp.is_loaded()) return;
    cx = 0;
    cy = 0;
    set_zoom(zoom_fit);
    set_dirty();
}

bool BMPViewer::on_key(const KeyEvent key) {
    if (!bmp.is_loaded()) return false;
    if (key == KeyEvent::Up) {
        return move_pos(0, -1 * mvy);
    }
    if (key == KeyEvent::Down) {
        return move_pos(0, mvy);
    }
    if (key == KeyEvent::Left) {
        return move_pos(-1 * mvx, 0);
    }
    if (key == KeyEvent::Right) {
        return move_pos(mvx, 0);
    }
    if (key == KeyEvent::Select) {
        if (!enter_pass) {
            reset_pos();
            return true;
        }
    }
    return false;
}

bool BMPViewer::on_encoder(EncoderEvent delta) {
    if (!bmp.is_loaded()) return false;
    if (delta > 0) {
        set_zoom(zoom + delta);  // 0 handled in set_zoom
        return true;
    }
    if (delta < 0) {
        if (zoom == 1)  // not 0, but -1
            set_zoom(-2);
        else
            set_zoom(zoom + delta);  // decrease
        return true;
    }
    return false;
}

// sets if the enter key should be passed to parent or handled. true = pass it, false = handle as reset pos+zoom
void BMPViewer::set_enter_pass(bool pass) {
    enter_pass = pass;
}

bool BMPViewer::get_enter_pass() {
    return enter_pass;
}

bool BMPViewer::on_keyboard(const KeyboardEvent event) {
    if (!bmp.is_loaded()) return false;
    if (event == '+') {
        set_zoom(zoom + 1);
        return true;
    }
    if (event == '-') {
        if (zoom == 1)  // not 0, but -1
            set_zoom(-2);
        else
            set_zoom(zoom - 1);  // decrease
        return true;
    }
    return false;
}