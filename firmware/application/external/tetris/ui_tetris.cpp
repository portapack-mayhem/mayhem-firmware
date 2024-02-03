#include "ui_tetris.hpp"

namespace ui::external_app::tetris {

#pragma GCC diagnostic push
// external code, so ignore warnings
#pragma GCC diagnostic ignored "-Weffc++"
#include "tetris.cpp"
#pragma GCC diagnostic pop

TetrisView::TetrisView(NavigationView& nav)
    : nav_(nav) {
    add_children({&dummy});
}

void TetrisView::paint(Painter& painter) {
    (void)painter;

    if (!initialized) {
        initialized = true;
        std::srand(LPC_RTC->CTIME0);
        main();
    }
}

void TetrisView::frame_sync() {
    check_fall_timer();
    set_dirty();
}

bool TetrisView::on_encoder(const EncoderEvent delta) {
    return check_encoder(delta);
}

bool TetrisView::on_key(const KeyEvent key) {
    return check_key(key);
}

}  // namespace ui::external_app::tetris
