#include "mcu_temperature.hpp"

#include "ui_painter.hpp"

#include "portapack.hpp"

using namespace portapack;
namespace ui::external_app::mcu_temperature {

void McuTemperatureWidget::paint(Painter& painter) {
    const auto logger = portapack::temperature_logger;

    const auto rect = screen_rect();
    const Color color_background{0, 0, 64};
    const Color color_foreground = Theme::getInstance()->fg_green->foreground;
    const Color color_reticle{128, 128, 128};

    const auto graph_width = static_cast<int>(logger.capacity()) * bar_width;
    const Rect graph_rect{
        rect.left() + (rect.width() - graph_width) / 2, rect.top() + 8,
        graph_width, rect.height()};
    const Rect frame_rect{
        graph_rect.left() - 1, graph_rect.top() - 1,
        graph_rect.width() + 2, graph_rect.height() + 2};
    painter.draw_rectangle(frame_rect, color_reticle);
    painter.fill_rectangle(graph_rect, color_background);

    const auto history = logger.history();
    for (size_t i = 0; i < history.size(); i++) {
        const Coord x = graph_rect.right() - (history.size() - i) * bar_width;
        const auto sample = history[i];
        const auto temp = temperature(sample);
        const auto y = screen_y(temp, graph_rect);
        const Dim bar_height = graph_rect.bottom() - y;
        painter.fill_rectangle({x, y, bar_width, bar_height}, color_foreground);
    }

    if (!history.empty()) {
        const auto sample = history.back();
        const auto temp = temperature(sample);
        const auto last_y = screen_y(temp, graph_rect);
        const Coord x = graph_rect.right() + 8;
        const Coord y = last_y - 8;

        painter.draw_string({x, y}, style(), temperature_str(temp));
    }

    const auto display_temp_max = display_temp_min + (graph_rect.height() / display_temp_scale);
    for (auto temp = display_temp_min; temp <= display_temp_max; temp += 10) {
        const int32_t tick_length = 6;
        const auto tick_x = graph_rect.left() - tick_length;
        const auto tick_y = screen_y(temp, graph_rect);
        painter.fill_rectangle({tick_x, tick_y, tick_length, 1}, color_reticle);
        const auto text_x = graph_rect.left() - temp_len * 8 - 8;
        const auto text_y = tick_y - 8;
        painter.draw_string({text_x, text_y}, style(), temperature_str(temp));
    }
}

McuTemperatureWidget::temperature_t McuTemperatureWidget::temperature(const sample_t sensor_value) const {
    // Scaling is different for MAX2837 vs MAX2839 so it's now done in the respective chip-specific module
    return sensor_value;
}

std::string McuTemperatureWidget::temperature_str(const temperature_t temperature) const {
    return to_string_dec_int(temperature, temp_len - 2) + STR_DEGREES_C;
}

Coord McuTemperatureWidget::screen_y(
    const temperature_t temperature,
    const Rect& rect) const {
    int y_raw = rect.bottom() - ((temperature - display_temp_min) * display_temp_scale);
    const auto y_limit = std::min(rect.bottom(), std::max(rect.top(), y_raw));
    return y_limit;
}

McuTemperatureView::McuTemperatureView(NavigationView& nav) {
    add_children({
        &text_title,
        &temperature_widget,
        &button_done,
    });

    button_done.on_select = [&nav](Button&) { nav.pop(); };
}

void McuTemperatureView::focus() {
    button_done.focus();
}

}  // namespace ui::external_app::mcu_temperature