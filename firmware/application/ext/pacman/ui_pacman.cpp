#include "ui_pacman.hpp"
#include "irq_controls.hpp"

namespace ui::external_app::pacman {

#include "playfield.hpp"

Playfield _game;

PacmanView::PacmanView(NavigationView& nav)
    : nav_(nav) {
    add_children({&dummy});
}

void PacmanView::focus() {
    dummy.focus();
}

void PacmanView::paint(Painter& painter) {
    // View::paint(painter);

    if (!initialized) {
        initialized = true;
        _game.Init();
    }

    auto switches_debounced = get_switches_state().to_ulong();

    but_RIGHT = (switches_debounced & 0x01) == 0x01;
    but_LEFT = (switches_debounced & 0x02) == 0x02;
    but_DOWN = (switches_debounced & 0x04) == 0x04;
    but_UP = (switches_debounced & 0x08) == 0x08;
    but_A = (switches_debounced & 0x10) == 0x10;

    _game.Step();
}

void PacmanView::frame_sync() {
    set_dirty();
}

}  // namespace ui::external_app::pacman
