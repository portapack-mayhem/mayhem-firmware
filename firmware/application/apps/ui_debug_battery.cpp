#include "ui_debug_battery.hpp"
#include "string_format.hpp"

namespace ui {

BatteryCapacityView::BatteryCapacityView(NavigationView& nav) {
    entries = {{
        {"RepCap", 0x05, 0.5, "mAh", false},
        {"QResidual", 0x0C, 0.5, "mAh", false},
        {"MixCap", 0x0F, 0.5, "mAh", false},
        {"FullCapRep", 0x10, 0.5, "mAh", false},
        {"DesignCap", 0x18, 0.5, "mAh", false},
        {"AvCap", 0x1F, 0.5, "mAh", false},
        {"FullCapNom", 0x23, 0.5, "mAh", false},
        {"dQAcc", 0x45, 16.0, "mAh", false},
        {"VFRemCap", 0x4A, 0.5, "mAh", false},
        {"QH", 0x4D, 0.5, "mAh", true},
        {"AtQResidual", 0xDC, 0.5, "mAh", false},
    }};

    for (size_t i = 0; i < entries.size(); ++i) {
        name_texts[i].set_parent_rect({0 * 8, (i + 1) * 16, 10 * 8, 16});
        name_texts[i].set(entries[i].name);
        add_child(&name_texts[i]);

        addr_texts[i].set_parent_rect({11 * 8, (i + 1) * 16, 5 * 8, 16});
        addr_texts[i].set("0x" + to_string_hex(entries[i].address, 2));
        add_child(&addr_texts[i]);

        hex_texts[i].set_parent_rect({17 * 8, (i + 1) * 16, 5 * 8, 16});
        add_child(&hex_texts[i]);

        value_texts[i].set_parent_rect({23 * 8, (i + 1) * 16, 10 * 8, 16});
        add_child(&value_texts[i]);
    }

    add_children({&labels, &button_done});

    button_done.on_select = [&nav](Button&) { nav.pop(); };

    update_values();
}

void BatteryCapacityView::focus() {
    button_done.focus();
}

void BatteryCapacityView::update_values() {
    for (size_t i = 0; i < entries.size(); ++i) {
        uint16_t raw_value = battery::BatteryManagement::read_register(entries[i].address);
        
        hex_texts[i].set("0x" + to_string_hex(raw_value, 4));

        float scaled_value;
        if (entries[i].is_signed) {
            int16_t signed_value = static_cast<int16_t>(raw_value);
            scaled_value = signed_value * entries[i].scalar;
        } else {
            scaled_value = raw_value * entries[i].scalar;
        }

        value_texts[i].set(to_string_dec_int(scaled_value, 0) + " " + entries[i].unit);
    }
}

} // namespace ui