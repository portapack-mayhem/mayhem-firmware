#include "ui_debug_max17055.hpp"
#include "string_format.hpp"

namespace ui {

BatteryCapacityView::RegisterEntry BatteryCapacityView::get_entry(size_t index) {
    if (index < i2cdev::I2cDev_MAX17055::entries_count) {
        return i2cdev::I2cDev_MAX17055::entries[index];
    }
    return {"", 0, "", 0, false, "", false, 0, false, false, false, 0, false};
}

BatteryCapacityView::BatteryCapacityView(NavigationView& nav) {
    for (size_t i = 0; i < ENTRIES_PER_PAGE; ++i) {
        name_texts[i].set_parent_rect({UI_POS_X(0), static_cast<int>((i + 1) * 16), 8 * 8, 16});
        addr_texts[i].set_parent_rect({9 * 8, static_cast<int>((i + 1) * 16), 4 * 8, 16});
        hex_texts[i].set_parent_rect({14 * 8, static_cast<int>((i + 1) * 16), 6 * 8, 16});
        value_texts[i].set_parent_rect({21 * 8, static_cast<int>((i + 1) * 16), 10 * 8, 16});

        add_child(&name_texts[i]);
        add_child(&addr_texts[i]);
        add_child(&hex_texts[i]);
        add_child(&value_texts[i]);
    }

    add_children({&labels, &page_text, &button_done});

    button_done.on_select = [&nav](Button&) { nav.pop(); };

    auto dev = (i2cdev::I2cDev_MAX17055*)i2cdev::I2CDevManager::get_dev_by_model(I2C_DEVMDL::I2CDEVMDL_MAX17055);
    if (!dev) {  // dev not found
        nav.pop();
        return;
    }

    populate_page(0);
    update_page_text();
}

void BatteryCapacityView::focus() {
    button_done.focus();
}

bool BatteryCapacityView::on_encoder(const EncoderEvent delta) {
    int32_t new_page = current_page + delta;
    if (new_page >= 0 && new_page < ((int32_t)i2cdev::I2cDev_MAX17055::entries_count + ENTRIES_PER_PAGE - 1) / ENTRIES_PER_PAGE) {
        current_page = new_page;
        populate_page(current_page * ENTRIES_PER_PAGE);
        update_page_text();
    }
    return true;
}

void BatteryCapacityView::update_values() {
    i2cdev::I2cDev_MAX17055* dev = (i2cdev::I2cDev_MAX17055*)i2cdev::I2CDevManager::get_dev_by_model(I2C_DEVMDL::I2CDEVMDL_MAX17055);
    for (size_t i = 0; i < ENTRIES_PER_PAGE; ++i) {
        size_t entry_index = current_page * ENTRIES_PER_PAGE + i;
        if (entry_index < i2cdev::I2cDev_MAX17055::entries_count) {
            const auto entry = get_entry(entry_index);
            uint16_t raw_value = dev->read_register(entry.address);

            hex_texts[i].set("0x" + to_string_hex(raw_value, 4));

            float scaled_value;
            if (entry.is_signed) {
                int16_t signed_value = static_cast<int16_t>(raw_value);
                scaled_value = signed_value * entry.scalar;
            } else {
                scaled_value = raw_value * entry.scalar;
            }

            // Format the value with appropriate decimal places
            std::string formatted_value;
            if (entry.resolution > 0) {
                formatted_value = to_string_decimal(scaled_value, std::min(entry.resolution, 3));
            } else {
                formatted_value = to_string_dec_int(scaled_value);  // Show up to 3 decimal places
            }

            value_texts[i].set(formatted_value + " " + entry.unit);
        }
    }
}

void BatteryCapacityView::populate_page(int start_index) {
    for (size_t i = 0; i < ENTRIES_PER_PAGE; ++i) {
        size_t entry_index = start_index + i;
        if (entry_index < i2cdev::I2cDev_MAX17055::entries_count) {
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
    int total_pages = (i2cdev::I2cDev_MAX17055::entries_count + ENTRIES_PER_PAGE - 1) / ENTRIES_PER_PAGE;
    page_text.set("Page " + to_string_dec_uint(current_page + 1) + "/" + to_string_dec_uint(total_pages));
}

}  // namespace ui