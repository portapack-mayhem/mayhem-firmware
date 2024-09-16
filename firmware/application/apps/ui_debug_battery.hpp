#ifndef __UI_DEBUG_BATTERY_HPP__
#define __UI_DEBUG_BATTERY_HPP__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "battery.hpp"

namespace ui {

class BatteryCapacityView : public View {
   public:
    BatteryCapacityView(NavigationView& nav);
    void focus() override;
    std::string title() const override { return "Battery Capacity"; }

   private:
    struct CapacityEntry {
        std::string name;
        uint8_t address;
        float scalar;
        std::string unit;
        bool is_signed;
    };

    std::array<CapacityEntry, 11> entries;

    Labels labels{
        {{0 * 8, 0 * 16}, "Capacities", Theme::getInstance()->fg_yellow->foreground},
        {{11 * 8, 0 * 16}, "Addr", Theme::getInstance()->fg_yellow->foreground},
        {{17 * 8, 0 * 16}, "Hex", Theme::getInstance()->fg_yellow->foreground},
        {{23 * 8, 0 * 16}, "Value", Theme::getInstance()->fg_yellow->foreground},
    };

    std::array<Text, 11> name_texts;
    std::array<Text, 11> addr_texts;
    std::array<Text, 11> hex_texts;
    std::array<Text, 11> value_texts;

    Button button_done{
        {72, 264, 96, 24},
        "Done"};

    void update_values();
};

}  // namespace ui

#endif  // __UI_DEBUG_BATTERY_HPP__