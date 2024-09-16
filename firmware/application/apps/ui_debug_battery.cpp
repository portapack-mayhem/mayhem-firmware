#include "ui_debug_battery.hpp"
#include "string_format.hpp"

namespace ui {

BatteryCapacityView::RegisterEntry BatteryCapacityView::get_entry(size_t index) {
    static const RegisterEntry entries[] = {
        {"Status", 0x00, "", 1, false, "", false, 0, true, false, false, 0, false},
        {"VAlrtTh", 0x01, "", 1, false, "", false, 0, true, false, false, 0, false},
        {"TAlrtTh", 0x02, "", 1, false, "", false, 0, true, false, false, 0, false},
        {"SAlrtTh", 0x03, "", 1, false, "", false, 0, true, false, false, 0, false},
        {"AtRate", 0x04, "current", 0.15625, true, "mA", true, 5, true, false, false, 0, false},
        {"RepCap", 0x05, "capacity", 0.5, false, "mAh", true, 1, true, false, false, 0, false},
        {"RepSOC", 0x06, "percent", 0.00390625, false, "%", true, 6, true, false, false, 0, false},
        {"Age", 0x07, "percent", 0.00390625, false, "%", true, 6, true, false, false, 0, false},
        {"Temp", 0x08, "temperature", 0.00390625, true, "C", true, 6, true, false, false, 0, false},
        {"VCell", 0x09, "voltage", 0.000078125, false, "V", true, 9, true, false, false, 0, false},
        {"Current", 0x0A, "current", 0.15625, true, "mA", true, 5, true, false, false, 0, false},
        {"AvgCurrent", 0x0B, "current", 0.15625, true, "mA", true, 5, true, false, false, 0, false},
        {"QResidual", 0x0C, "capacity", 0.5, false, "mAh", true, 1, true, false, false, 0, false},
        {"MixSOC", 0x0D, "percent", 0.00390625, false, "%", true, 6, true, false, false, 0, false},
        {"AvSOC", 0x0E, "percent", 0.00390625, false, "%", true, 6, true, false, false, 0, false},
        {"MixCap", 0x0F, "capacity", 0.5, false, "mAh", true, 1, true, false, false, 0, false},

        {"FullCapRep", 0x10, "capacity", 0.5, false, "mAh", true, 1, true, true, false, 0, false},
        {"TTE", 0x11, "time", 0.0015625, false, "hr", true, 6, true, false, false, 0, false},
        {"QRTable00", 0x12, "model", 1, false, "", false, 0, true, true, false, 0, false},
        {"FullSocThr", 0x13, "model", 0.00390625, false, "%", true, 6, true, false, false, 0, false},
        {"RCell", 0x14, "resistance", 0.244140625, false, "mOhms", false, 6, true, false, false, 0, false},
        {"Reserved", 0x15, "", 0.244140625, false, "mOhms", false, 6, true, false, false, 0, true},
        {"AvgTA", 0x16, "temperature", 0.00390625, true, "C", true, 6, true, false, false, 0, false},
        {"Cycles", 0x17, "cycles", 0.01, false, "Cycles", false, 2, true, true, false, 0, false},
        {"DesignCap", 0x18, "capacity", 0.5, false, "mAh", true, 1, true, false, false, 0, false},
        {"AvgVCell", 0x19, "voltage", 0.000078125, false, "V", true, 9, true, false, false, 0, false},
        {"MaxMinTemp", 0x1A, "", 1, false, "", false, 0, true, false, false, 0, false},
        {"MaxMinVolt", 0x1B, "", 1, false, "", false, 0, true, false, false, 0, false},
        {"MaxMinCurr", 0x1C, "", 1, false, "", false, 0, true, false, false, 0, false},
        {"Config", 0x1D, "model", 1, false, "", false, 0, true, false, false, 0, false},
        {"IChgTerm", 0x1E, "model", 0.15625, true, "mA", true, 5, true, false, false, 0, false},
        {"AvCap", 0x1F, "capacity", 0.5, false, "mAh", true, 1, true, false, false, 0, false},

        {"TTF", 0x20, "time", 0.0015625, false, "hr", true, 6, true, false, false, 0, false},
        {"DevName", 0x21, "", 1, false, "", false, 0, true, false, false, 0, false},
        {"QRTable10", 0x22, "model", 1, false, "", false, 0, true, true, false, 0, false},
        {"FullCapNom", 0x23, "capacity", 0.5, false, "mAh", true, 1, true, true, false, 0, false},
        {"Reserved", 0x24, "", 0.00390625, true, "C", true, 6, true, false, false, 0, true},
        {"Reserved", 0x25, "", 0.00390625, true, "C", true, 6, true, false, false, 0, true},
        {"Reserved", 0x26, "", 0.00390625, true, "C", true, 6, true, false, false, 0, true},
        {"AIN", 0x27, "temperature", 0.0015259, false, "%", true, 6, true, false, false, 0, false},
        {"LearnCfg", 0x28, "model", 1, false, "", false, 0, true, false, false, 0, false},
        {"FilterCfg", 0x29, "model", 1, false, "", false, 0, true, false, false, 0, false},
        {"RelaxCfg", 0x2A, "model", 1, false, "", false, 0, true, false, false, 0, false},
        {"MiscCfg", 0x2B, "model", 1, false, "", false, 0, true, false, false, 0, false},
        {"TGain", 0x2C, "temperature", 1, false, "", false, 0, true, false, false, 0, false},
        {"TOff", 0x2D, "temperature", 1, false, "", false, 0, true, false, false, 0, false},
        {"CGain", 0x2E, "current", 0.09765625, true, "%", true, 8, true, false, false, 0, false},
        {"COff", 0x2F, "current", 0.15625, true, "mA", true, 5, true, false, false, 0, false},

        {"Reserved", 0x30, "", 0.00125, false, "V", true, 5, true, false, false, 0, true},
        {"Reserved", 0x31, "", 0.005, true, "mA", true, 3, true, false, false, 0, true},
        {"QRTable20", 0x32, "model", 1, false, "", false, 0, true, true, false, 0, false},
        {"Reserved", 0x33, "", 0.0015625, false, "hr", true, 6, true, true, false, 0, true},
        {"DieTemp", 0x34, "temperature", 0.00390625, true, "C", true, 6, true, false, false, 0, false},
        {"FullCap", 0x35, "model", 0.5, false, "mAh", true, 1, true, false, false, 0, false},
        {"Reserved", 0x36, "", 0.15625, true, "mA", true, 5, true, false, false, 0, true},
        {"Reserved", 0x37, "led", 1, false, "", false, 0, true, true, false, 0, false},
        {"RComp0", 0x38, "model", 1, false, "", false, 0, true, true, false, 0, false},
        {"TempCo", 0x39, "model", 1, false, "", false, 0, true, true, false, 0, false},
        {"VEmpty", 0x3A, "model", 0.000078125, false, "", false, 1, true, false, false, 0, false},
        {"Reserved", 0x3B, "", 0.15625, true, "mA", true, 5, true, false, false, 0, true},
        {"Reserved", 0x3C, "", 0.000976563, false, "s", true, 6, true, false, false, 0, true},
        {"FStat", 0x3D, "", 1, false, "", false, 0, true, false, false, 0, false},
        {"Timer", 0x3E, "time", 4.88E-05, false, "hr", true, 7, true, false, false, 0, false},
        {"ShdnTimer", 0x3F, "", 0.000366, false, "hr", true, 6, true, false, false, 0, false},

        {"UserMem1", 0x40, "led", 1, false, "", false, 0, true, false, false, 0, false},
        {"Reserved", 0x41, "", 1, false, "", false, 0, true, false, false, 0, true},
        {"QRTable30", 0x42, "model", 0.15625, false, "", false, 0, true, true, false, 0, false},
        {"RGain", 0x43, "", 1, false, "", false, 0, true, false, false, 0, false},
        {"Reserved", 0x44, "", 0.000078125, false, "V", true, 9, true, false, false, 0, true},
        {"dQAcc", 0x45, "capacity", 16, false, "mAh", true, 0, true, true, false, 0, false},
        {"dPAcc", 0x46, "percent", 0.0625, false, "%", true, 4, true, true, false, 0, false},
        {"Reserved", 0x47, "", 0.00390625, true, "%", true, 6, true, true, false, 0, true},
        {"Reserved", 0x48, "", 0.00390625, false, "%", true, 6, true, true, false, 0, true},
        {"ConvgCfg", 0x49, "", 1, false, "", false, 0, true, false, false, 0, false},
        {"VFRemCap", 0x4A, "capacity", 0.5, false, "mAh", true, 1, true, false, false, 0, false},
        {"Reserved", 0x4B, "", 1, false, "", false, 0, true, false, false, 0, false},
        {"Reserved", 0x4C, "", 0.5, true, "mAh", true, 1, true, false, false, 0, true},
        {"QH", 0x4D, "capacity", 0.5, true, "mAh", true, 1, true, false, false, 0, false},
        {"Reserved", 0x4E, "", 7.63E-06, false, "mAh", true, 8, true, false, false, 0, true},
        {"Reserved", 0x4F, "", 0.5, false, "mAh", true, 1, true, false, false, 0, true},

        {"Status2", 0xB0, "", 1, false, "", false, 0, true, false, false, 0, false},
        {"Power", 0xB1, "", 0.8, true, "", false, 1, true, false, false, 0, false},
        {"ID", 0xB2, "", 1, false, "", false, 0, true, false, false, 0, false},
        {"AvgPower", 0xB3, "", 0.8, true, "", false, 1, true, false, false, 0, false},
        {"IAlrtTh", 0xB4, "", 1, false, "", false, 0, true, false, false, 0, false},
        {"TTFCfg", 0xB5, "", 1, false, "", false, 0, true, false, false, 0, false},
        {"CVMixCap", 0xB6, "capacity", 0.5, false, "mAh", true, 1, true, false, false, 0, false},
        {"CVHalfTime", 0xB7, "time", 0.000195313, false, "hr", true, 6, true, false, false, 0, false},
        {"CGTempCo", 0xB8, "", 1, false, "", false, 0, true, false, false, 0, false},
        {"Curve", 0xB9, "", 1, false, "", false, 0, true, false, false, 0, false},
        {"HibCfg", 0xBA, "", 1, false, "", false, 0, true, false, false, 0, false},
        {"Config2", 0xBB, "", 1, false, "", false, 0, true, false, false, 0, false},
        {"VRipple", 0xBC, "voltage", 9.77E-06, false, "V", true, 8, true, false, false, 0, false},
        {"RippleCfg", 0xBD, "", 1, false, "", false, 0, true, false, false, 0, false},
        {"TimerH", 0xBE, "time", 3.2, false, "hr", true, 1, true, false, false, 0, false},
        {"Reserved", 0xBF, "", 0.00390625, false, "%", true, 6, true, false, false, 0, true},

        {"Rsense", 0xD0, "", 1, false, "", false, 0, true, false, false, 0, false},
        {"ScOcvLim", 0xD1, "", 1, false, "", false, 0, true, false, false, 0, false},
        {"Reserved", 0xD2, "", 1, false, "", false, 0, true, false, false, 0, true},
        {"SOCHold", 0xD3, "", 1, false, "", false, 0, true, false, false, 0, false},
        {"MaxPeakPower", 0xD4, "", 0.8, true, "", false, 0, true, false, false, 0, false},
        {"SusPeakPower", 0xD5, "", 0.8, true, "", false, 0, true, false, false, 0, false},
        {"PackResistance", 0xD6, "", 0.244141063, false, "", false, 0, true, false, false, 0, false},
        {"SysResistance", 0xD7, "", 0.244141063, false, "", false, 0, true, false, false, 0, false},
        {"MinSysVoltage", 0xD8, "", 0.000078125, false, "", false, 0, true, false, false, 0, false},
        {"MPPCurrent", 0xD9, "current", 0.15625, true, "mA", true, 5, true, false, false, 0, false},
        {"SPPCurrent", 0xDA, "current", 0.15625, true, "mA", true, 5, true, false, false, 0, false},
        {"ModelCfg", 0xDB, "", 1, false, "", false, 0, true, false, false, 0, false},
        {"AtQResidual", 0xDC, "capacity", 0.5, false, "mAh", true, 1, true, false, false, 0, false},
        {"AtTTE", 0xDD, "time", 0.0015625, false, "hr", true, 6, true, false, false, 0, false},
        {"AtAvSOC", 0xDE, "percent", 0.00390625, false, "%", true, 6, true, false, false, 0, false},
        {"AtAvCap", 0xDF, "capacity", 0.5, false, "mAh", true, 1, true, false, false, 0, false},

        {"Reserved", 0xE0, "", 1, false, "", false, 0, true, false, false, 0, true},
        {"Reserved", 0xE1, "", 1, false, "", false, 0, true, false, false, 0, true},
        {"Reserved", 0xE2, "", 1, false, "", false, 0, true, false, false, 0, true},
        {"Reserved", 0xE3, "", 1, false, "", false, 0, true, false, false, 0, true},
        {"Reserved", 0xE4, "", 1, false, "", false, 0, true, false, false, 0, true},
        {"Reserved", 0xE5, "", 1, false, "", false, 0, true, false, false, 0, true},
        {"Reserved", 0xE6, "", 1, false, "", false, 0, true, false, false, 0, true},
        {"Reserved", 0xE7, "", 1, false, "", false, 0, true, false, false, 0, true},
        {"Reserved", 0xE8, "", 1, false, "", false, 0, true, false, false, 0, true},
        {"Reserved", 0xE9, "", 1, false, "", false, 0, true, false, false, 0, true},
        {"Reserved", 0xEA, "", 1, false, "", false, 0, true, false, false, 0, true},
        {"Reserved", 0xEB, "", 1, false, "", false, 0, true, false, false, 0, true},
        {"Reserved", 0xEC, "", 1, false, "", false, 0, true, false, false, 0, true},
        {"Reserved", 0xED, "", 1, false, "", false, 0, true, false, false, 0, true},
        {"Reserved", 0xEE, "voltage", 0.000078125, false, "V", true, 9, true, false, false, 0, true},
        {"Reserved", 0xEF, "", 1, false, "", false, 0, true, false, false, 0, true},

        {"Reserved", 0xF0, "", 1, false, "", false, 0, true, false, false, 0, true},
        {"Reserved", 0xF1, "", 1, false, "", false, 0, true, false, false, 0, true},
        {"Reserved", 0xF2, "", 1, false, "", false, 0, true, false, false, 0, true},
        {"Reserved", 0xF3, "", 1, false, "", false, 0, true, false, false, 0, true},
        {"Reserved", 0xF4, "", 1, false, "", false, 0, true, false, false, 0, true},
        {"Reserved", 0xF5, "", 1, false, "", false, 0, true, false, false, 0, true},
        {"Reserved", 0xF6, "", 0.00390625, false, "%", true, 6, true, false, false, 0, true},
        {"Reserved", 0xF7, "", 0.001, true, "s", true, 3, true, false, false, 0, true},
        {"Reserved", 0xF8, "", 0.003051758, true, "C", true, 6, true, false, false, 0, true},
        {"Reserved", 0xF9, "", 0.00015625, false, "V", true, 6, true, false, false, 0, true},
        {"Reserved", 0xFA, "", 0.15625, true, "mA", true, 5, true, false, false, 0, true},
        {"Reserved", 0xFB, "", 0.000078125, false, "V", true, 9, true, false, false, 0, true},
        {"Reserved", 0xFC, "", 1, false, "", false, 0, true, false, false, 0, true},
        {"Reserved", 0xFD, "", 1, false, "", false, 0, true, false, false, 0, true},
        {"Reserved", 0xFE, "", 0.000078125, false, "V", true, 9, true, false, false, 0, true},
        {"Reserved", 0xFF, "", 0.00390625, false, "%", true, 6, true, false, false, 0, true},
    };

    if (index < sizeof(entries) / sizeof(entries[0])) {
        return entries[index];
    }
    return {"", 0, "", 0, false, "", false, 0, false, false, false, 0, false};
}

size_t BatteryCapacityView::get_entry_count() {
    return 144;  // Update this to the total number of entries
}

BatteryCapacityView::BatteryCapacityView(NavigationView& nav)
    : labels({
          {{0 * 8, 0 * 16}, "Register", Theme::getInstance()->fg_yellow->foreground},
          {{11 * 8, 0 * 16}, "Addr", Theme::getInstance()->fg_yellow->foreground},
          {{17 * 8, 0 * 16}, "Hex", Theme::getInstance()->fg_yellow->foreground},
          {{23 * 8, 0 * 16}, "Value", Theme::getInstance()->fg_yellow->foreground},
      }),
      button_prev{{16, 280, 48, 24}, "<"},
      button_next{{80, 280, 48, 24}, ">"},
      page_text{{144, 284, 80, 16}, "Page 1/1"},
      button_done{{224, 280, 64, 24}, "Done"} {
    for (size_t i = 0; i < ENTRIES_PER_PAGE; ++i) {
        name_texts[i].set_parent_rect({0 * 8, static_cast<int>((i + 1) * 16), 10 * 8, 16});
        addr_texts[i].set_parent_rect({11 * 8, static_cast<int>((i + 1) * 16), 5 * 8, 16});
        hex_texts[i].set_parent_rect({17 * 8, static_cast<int>((i + 1) * 16), 5 * 8, 16});
        value_texts[i].set_parent_rect({23 * 8, static_cast<int>((i + 1) * 16), 10 * 8, 16});

        add_child(&name_texts[i]);
        add_child(&addr_texts[i]);
        add_child(&hex_texts[i]);
        add_child(&value_texts[i]);
    }

    add_children({&labels, &button_prev, &button_next, &page_text, &button_done});

    button_done.on_select = [&nav](Button&) { nav.pop(); };

    button_prev.on_select = [this](Button&) {
        if (current_page > 0) {
            current_page--;
            populate_page(current_page * ENTRIES_PER_PAGE);
            update_page_text();
        }
    };

    button_next.on_select = [this](Button&) {
        if ((current_page + 1) * ENTRIES_PER_PAGE < get_entry_count()) {
            current_page++;
            populate_page(current_page * ENTRIES_PER_PAGE);
            update_page_text();
        }
    };

    populate_page(0);
    update_page_text();
}

// Add destructor to clean up resources
BatteryCapacityView::~BatteryCapacityView() {
    // for (size_t i = 0; i < ENTRIES_PER_PAGE; ++i) {
    //     name_texts[i].~Text();
    //     addr_texts[i].~Text();
    //     hex_texts[i].~Text();
    //     value_texts[i].~Text();
    // }
}

void BatteryCapacityView::focus() {
    button_done.focus();
}

void BatteryCapacityView::update_values() {
    for (size_t i = 0; i < ENTRIES_PER_PAGE; ++i) {
        size_t entry_index = current_page * ENTRIES_PER_PAGE + i;
        if (entry_index < get_entry_count()) {
            const auto entry = get_entry(entry_index);
            uint16_t raw_value = battery::BatteryManagement::read_register(entry.address);

            hex_texts[i].set("0x" + to_string_hex(raw_value, 4));

            float scaled_value;
            if (entry.is_signed) {
                int16_t signed_value = static_cast<int16_t>(raw_value);
                scaled_value = signed_value * entry.scalar;
            } else {
                scaled_value = raw_value * entry.scalar;
            }

            value_texts[i].set(to_string_dec_int(scaled_value, entry.resolution) + " " + entry.unit);
        }
    }
}

void BatteryCapacityView::populate_page(int start_index) {
    for (size_t i = 0; i < ENTRIES_PER_PAGE; ++i) {
        size_t entry_index = start_index + i;
        if (entry_index < get_entry_count()) {
            const auto entry = get_entry(entry_index);
            name_texts[i].set(entry.name);
            addr_texts[i].set("0x" + to_string_hex(entry.address, 2));
            name_texts[i].hidden(false);
            addr_texts[i].hidden(false);
            hex_texts[i].hidden(false);
            value_texts[i].hidden(false);
        } else {
            name_texts[i].hidden(true);
            addr_texts[i].hidden(true);
            hex_texts[i].hidden(true);
            value_texts[i].hidden(true);
        }
    }
    update_values();
}

void BatteryCapacityView::update_page_text() {
    int total_pages = (get_entry_count() + ENTRIES_PER_PAGE - 1) / ENTRIES_PER_PAGE;
    page_text.set("Page " + to_string_dec_uint(current_page + 1) + "/" + to_string_dec_uint(total_pages));
}

}  // namespace ui