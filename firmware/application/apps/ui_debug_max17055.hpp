#ifndef __UI_DEBUG_BATTERY_HPP__
#define __UI_DEBUG_BATTERY_HPP__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "battery.hpp"
#include "i2cdevmanager.hpp"
#include "i2cdev_max17055.hpp"

namespace ui {

class BatteryCapacityView : public View {
   public:
    BatteryCapacityView(NavigationView& nav);
    void focus() override;
    std::string title() const override { return "Battery Registers"; }

    bool on_encoder(const EncoderEvent delta) override;

    using RegisterEntry = i2cdev::I2cDev_MAX17055::RegisterEntry;

   private:
    static RegisterEntry get_entry(size_t index);

    Labels labels{
        {{UI_POS_X(0), UI_POS_Y(0)}, "Reg", Theme::getInstance()->fg_yellow->foreground},
        {{9 * 8, UI_POS_Y(0)}, "Addr", Theme::getInstance()->fg_yellow->foreground},
        {{14 * 8, UI_POS_Y(0)}, "Hex", Theme::getInstance()->fg_yellow->foreground},
        {{21 * 8, UI_POS_Y(0)}, "Value", Theme::getInstance()->fg_yellow->foreground},
    };
    std::array<Text, 16> name_texts = {};
    std::array<Text, 16> addr_texts = {};
    std::array<Text, 16> hex_texts = {};
    std::array<Text, 16> value_texts = {};

    Text page_text{{144, UI_POS_Y_BOTTOM(3), 80, 16}, "Page 1/1"};
    Button button_done{{16, UI_POS_Y_BOTTOM(3), 96, 24}, "Done"};

    void update_values();
    void populate_page(int start_index);
    void update_page_text();
    int current_page = 0;
    static constexpr int ENTRIES_PER_PAGE = 16;
};

}  // namespace ui

#endif  // __UI_DEBUG_BATTERY_HPP__