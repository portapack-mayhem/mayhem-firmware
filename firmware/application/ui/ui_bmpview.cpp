#include "ui_bmpview.hpp"

void BMPView::set_zoom(int8_t new_zoom) {
    zoom = new_zoom;
    set_dirty();
}
int8_t BMPView::get_zoom() {
    return zoom;
}

BMPView::BMPView(Rect parent_rect)
    : Widget{parent_rect} {
}

void BMPView::paint(Painter& painter) {}
void BMPView::on_focus() {}

void BMPView::on_blur() {}
bool BMPView::on_key(const KeyEvent key) {}
bool BMPView::on_encoder(EncoderEvent delta) {}
bool BMPView::on_keyboard(const KeyboardEvent event) {}