#include "ui_calculator.hpp"

namespace ui::external_app::calculator {

CalculatorView::CalculatorView(NavigationView& nav)
    : nav_(nav) {
}

void CalculatorView::focus() {
    // button_run.focus();
}

void CalculatorView::paint(Painter& painter) {
    // View::paint(painter);

    // ui::Painter painter;
    is_blue = !is_blue;
    painter.fill_rectangle(
        {0, 0, portapack::display.width(), portapack::display.height()},
        is_blue ? ui::Color::blue() : ui::Color::yellow());
}

void CalculatorView::frame_sync() {
    set_dirty();
}

}  // namespace ui::external_app::calculator
