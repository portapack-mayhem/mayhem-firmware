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
    static bool wait_for_button_release{false};

    if (!initialized) {
        initialized = true;
        _game.Init();
    }

    auto switches_raw = swizzled_switches() & ((1 << (int)Switch::Right) | (1 << (int)Switch::Left) | (1 << (int)Switch::Down) | (1 << (int)Switch::Up) | (1 << (int)Switch::Sel) | (1 << (int)Switch::Dfu));
    auto switches_debounced = get_switches_state().to_ulong();

    // For the Select (Start/Pause) button, wait for release to avoid repeat
    uint8_t buttons_to_wait_for = (1 << (int)Switch::Sel);
    if (wait_for_button_release) {
        if ((switches_debounced & buttons_to_wait_for) == 0)
            wait_for_button_release = false;
        switches_debounced &= ~buttons_to_wait_for;
    } else {
        if (switches_debounced & buttons_to_wait_for)
            wait_for_button_release = true;
    }

    // For the directional buttons, use the raw inputs for fastest response time
    but_RIGHT = (switches_raw & (1 << (int)Switch::Right)) != 0;
    but_LEFT = (switches_raw & (1 << (int)Switch::Left)) != 0;
    but_DOWN = (switches_raw & (1 << (int)Switch::Down)) != 0;
    but_UP = (switches_raw & (1 << (int)Switch::Up)) != 0;

    // For the pause button, use the debounced input to avoid glitches, and OR in the value to make sure that we don't clear it before it's seen
    but_A |= (switches_debounced & (1 << (int)Switch::Sel)) != 0;

    _game.Step();
}

void PacmanView::frame_sync() {
    set_dirty();
}

}  // namespace ui::external_app::pacman
