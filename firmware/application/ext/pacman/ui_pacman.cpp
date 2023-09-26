#include "ui_pacman.hpp"

namespace ui::external_app::pacman {

#include "playfield.hpp"

Playfield _game;

PacmanView::PacmanView(NavigationView& nav)
    : nav_(nav) {
    add_children({&labels,
                  &button_run});

    button_run.on_select = [this](Button&) {
        ui::Painter painter;

        for (size_t i = 0; i < 10; i++) {
            painter.fill_rectangle(
                {0, 0, portapack::display.width(), portapack::display.height()},
                ui::Color::blue());

            chThdSleep(500);

            painter.fill_rectangle(
                {0, 0, portapack::display.width(), portapack::display.height()},
                ui::Color::black());

            chThdSleep(500);
        }
    };
}

void PacmanView::focus() {
    button_run.focus();
}

void PacmanView::paint(Painter& painter) {
    // View::paint(painter);
}

void PacmanView::frame_sync() {
    _game.Step();
}

}  // namespace ui::external_app::pacman
