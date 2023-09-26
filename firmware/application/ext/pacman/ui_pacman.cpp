#include "ui_pacman.hpp"

namespace ui::external_app::pacman {

PacmanView::PacmanView(NavigationView& nav)
    : nav_(nav) {
}

void PacmanView::focus() {
    // button_run.focus();
}

void PacmanView::paint(Painter& painter) {
    View::paint(painter);
}

}  // namespace ui::external_app::pacman
