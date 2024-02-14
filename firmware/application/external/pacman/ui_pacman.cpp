#include "ui_pacman.hpp"
#include "irq_controls.hpp"

namespace ui::external_app::pacman {

#pragma GCC diagnostic push
// external code, so ignore warnings
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wreturn-type"
#pragma GCC diagnostic ignored "-Weffc++"
#include "playfield.hpp"
#pragma GCC diagnostic pop


PacmanView::PacmanView(NavigationView& nav)
    : nav_(nav) {
    add_children({&dummy});
}

void PacmanView::focus() {
    dummy.focus();
}

void PacmanView::paint(Painter& painter) {
    (void)painter;
    static Playfield _game;

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
