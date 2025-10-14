// ui_drone_frequency_manager.hpp
// Full frequency management UI for Enhanced Drone Analyzer
// Provides CRUD operations on frequency databases with user interface

#ifndef __UI_DRONE_FREQUENCY_MANAGER_HPP__
#define __UI_DRONE_FREQUENCY_MANAGER_HPP__

#include "ui.hpp"
#include "ui/ui_button.hpp"
#include "ui/ui_text.hpp"
#include "ui/ui_checkbox.hpp"
#include "ui/ui_number_field.hpp"
#include "ui/ui_numeric_entry.hpp"
#include "ui/ui_menu.hpp"
#include "ui/navigation.hpp"
#include "ui_drone_types.hpp"
#include "ui_drone_database.hpp"
#include <string>
#include <vector>

namespace ui::external_app::enhanced_drone_analyzer {

struct FrequencyEntryForm {
    uint32_t frequency_hz;
    DroneType drone_type;
    ThreatLevel threat_level;
    int32_t rssi_threshold_db;
    std::string name;
    uint32_t bandwidth_hz;
};

class DroneFrequencyManagerView : public View {
public:
    DroneFrequencyManagerView(NavigationView& nav);
    ~DroneFrequencyManagerView() override = default;

    void focus() override;
    std::string title() const override { return "Frequency Manager"; }
    void paint(Painter& painter) override;
    bool on_key(const KeyEvent key) override;
    bool on_touch(const TouchEvent event) override;

private:
    NavigationView& nav_;

    // Split view: left menu, right details
    MenuView menu_view_{ {0, 0, 200, screen_height}, true };
    uint32_t selected_frequency_index_ = 0;

    // Right panel controls (frequency 201-240)
    NumberField frequency_field_{ {201, 20}, 9, {0}, 6000000000, 500000000 };
    OptionsField drone_type_field_{ {201, 40}, 20 };
    OptionsField threat_level_field_{ {201, 50}, 20 };
    NumberField rssi_field_{ {201, 70}, 7, {-120}, -10, -80 };
    TextEntry text_name_{ {201, 90}, 24 };
    NumberField bandwidth_field_{ {201, 110}, 9, {0}, 100000000, 5000000 };
    Checkbox enabled_checkbox_{ {201, 130}, "" };

    // Action buttons (bottom row)
    Button button_add_{ {0, 220}, "Add Frequency" };
    Button button_update_{ {80, 220}, "Update" };
    Button button_remove_{ {136, 220}, "Remove" };
    Button button_clear_{ {184, 220}, "Clear All" };

    // Status display
    Text text_status_{ {0, 242}, 240, "Select frequency to edit..." };

    // Handle frequency selection from menu
    void on_frequency_selected(size_t index);

    // CRUD operations
    void on_add_frequency();
    void on_update_frequency();
    void on_remove_frequency();
    void on_clear_all();

    // Update UI with selected frequency data
    void update_frequency_details();

    // Validate and convert form data
    bool validate_current_form();
    FrequencyEntryForm get_form_data() const;

    // UI update helpers
    void refresh_frequency_list();
    void update_status_text(const char* text);
    void clear_form();

    // Initialize UI controls
    void initialize_menu_options();
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_DRONE_FREQUENCY_MANAGER_HPP__
