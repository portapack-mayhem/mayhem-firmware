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
    std::string title() const override { return "Battery Registers"; }

   private:
    struct RegisterEntry {
        std::string name;
        uint8_t address;
        std::string type;
        float scalar;
        bool is_signed;
        std::string unit;
        bool abbr_units;
        int resolution;
        bool is_user;
        bool is_save_restore;
        bool is_nv;
        uint16_t por_data;
        bool is_read_only;
    };

    std::vector<RegisterEntry> entries;

    Labels labels{
        {{0 * 8, 0 * 16}, "Register", Theme::getInstance()->fg_yellow->foreground},
        {{11 * 8, 0 * 16}, "Addr", Theme::getInstance()->fg_yellow->foreground},
        {{17 * 8, 0 * 16}, "Hex", Theme::getInstance()->fg_yellow->foreground},
        {{23 * 8, 0 * 16}, "Value", Theme::getInstance()->fg_yellow->foreground},
    };

    std::vector<Text> name_texts;
    std::vector<Text> addr_texts;
    std::vector<Text> hex_texts;
    std::vector<Text> value_texts;

    Button button_done{
        {16, 280, 96, 24},  // Adjusted Y position from 264 to 280
        "Done"};

    void update_values();
    void populate_page(int start_index);
    int current_page = 0;
    static constexpr int ENTRIES_PER_PAGE = 16;
};

}  // namespace ui

#endif  // __UI_DEBUG_BATTERY_HPP__