#include "ui_calculator.hpp"

namespace ui::external_app::calculator {

CalculatorView::CalculatorView(NavigationView& nav)
    : nav_(nav) {
}

void CalculatorView::focus() {
    // button_run.focus();
}

void CalculatorView::paint(Painter& painter) {
    View::paint(painter);
}

}  // namespace ui::external_app::calculator
