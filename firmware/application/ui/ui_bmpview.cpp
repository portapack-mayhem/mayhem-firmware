#include "ui_bmpview.hpp"
#include "usb_serial_asyncmsg.hpp"
#include "portapack.hpp"

bool BMPViewer::load_bmp(const std::filesystem::path& file) {
    if (!bmp.open(file, true)) return false;
    // todo calc default zoom level to fit screen, and min / max zoom too
    zoom_fit = 1;
    reset_pos();
    set_dirty();
    return true;
}

void BMPViewer::set_zoom(int8_t new_zoom) {
    if (new_zoom == 0) new_zoom = 1;
    if (new_zoom > max_zoom) new_zoom = max_zoom;
    if (new_zoom < min_zoom) new_zoom = min_zoom;
    zoom = new_zoom;
    UsbSerialAsyncmsg::asyncmsg("New zoom: ");
    UsbSerialAsyncmsg::asyncmsg(zoom);
    set_dirty();
}

// reads a lint from the bmp's bx, by coordinate to the line that's size is cnt. according to zoom
void BMPViewer::get_line(ui::Color* line, uint32_t bx, uint32_t by, uint32_t cnt) {
    if (!bmp.is_loaded()) return;
    for (uint32_t x = 0; x < cnt; x++) {
        uint32_t targetx = (zoom < 0) ? bx + x * -1 * zoom : bx + x / zoom;  // todo fix zoom out!
        if (!bmp.seek(targetx, by)) {
            line[x] = Color::white();  // can't seek there
        } else {
            bmp.read_next_px(line[x], false);
        }
    }
}

void BMPViewer::paint(Painter& painter) {
    if (!bmp.is_loaded()) {
        painter.draw_string({48, 24}, ui::Styles::white, "Can't load BMP");
        return;
    }
    // get where i can paint
    auto rect = screen_rect();
    auto d_height = rect.height();
    auto d_width = rect.width();

    // get the bmp's size
    // auto b_width = bmp.get_width();
    // auto b_height = bmp.get_real_height();

    uint32_t by = cy;  // we start to read from there
    ui::Color* line = new ui::Color[d_width];
    for (int32_t y = 0; y < d_height; y++) {
        by = cy + ((zoom < 0) ? -1 * zoom : y / (int32_t)zoom);
        get_line(line, cx, by, d_width);

        // todo optimize, not to read again and again the same..
        portapack::display.draw_pixels({rect.left(), rect.top() + y, d_width, 1}, line, d_width);
    }
    delete line;
}

int8_t BMPViewer::get_zoom() {
    return zoom;
}

BMPViewer::BMPViewer(Rect parent_rect)
    : Widget{parent_rect} {
}

BMPViewer::BMPViewer(Rect parent_rect, const std::filesystem::path& file)
    : Widget{parent_rect} {
    load_bmp(file);
}

void BMPViewer::on_focus() {}

void BMPViewer::on_blur() {
    set_dirty();
}

void BMPViewer::reset_pos() {
    if (!bmp.is_loaded()) return;
    cx = 0;
    cy = 0;
    zoom = zoom_fit;
    set_dirty();
}

bool BMPViewer::on_key(const KeyEvent key) {
    if (key == KeyEvent::Up) {
        if (cy <= 0) return false;
        // todo move up
    }
    if (key == KeyEvent::Down) {
        if (cy >= 100) return false;  // todo limit
        // todo move down
    }
    if (key == KeyEvent::Left) {
        if (cx <= 0) return false;
        // todo move left
    }
    if (key == KeyEvent::Right) {
        if (cx >= 100) return false;  // todo limit
        // todo move right
    }
    if (key == KeyEvent::Select) {
        reset_pos();
        return true;
    }
    return false;
}

bool BMPViewer::on_encoder(EncoderEvent delta) {
    if (delta > 0) {
        set_zoom(zoom + 1);  // 0 handled in set_zoom
        return true;
    }
    if (delta > 0) {
        if (zoom == 1)  // not 0, but -1
            set_zoom(-1);
        else
            set_zoom(zoom - 1);  // decrease
        return true;
    }
    return false;
}

bool BMPViewer::on_keyboard(const KeyboardEvent event) {
    if (event == '+') {
        set_zoom(zoom + 1);
        return true;
    }
    if (event == '-') {
        if (zoom == 1)  // not 0, but -1
            set_zoom(-1);
        else
            set_zoom(zoom - 1);  // decrease
        return true;
    }
    return false;
}