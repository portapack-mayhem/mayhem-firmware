// ui_drone_frequency_manager.cpp
// Redesigned frequency management with Mayhem patterns
// Base class + specialized views (following Freqman/Recon architecture)

#include "ui_drone_frequency_manager.hpp"
#include "ui_drone_database.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

// BASE CLASS IMPLEMENTATION
DroneFrequencyManagerBaseView::DroneFrequencyManagerBaseView(NavigationView& nav)
    : nav_(nav), error_(ERROR_NONE)
{
    add_child(&button_exit);
    button_exit.on_select = [this, &nav](Button&) {
        nav.pop();
    };

    ensure_database_initialized();
}

void DroneFrequencyManagerBaseView::focus() {
    if (error_ == ERROR_ACCESS) {
        nav_.display_modal("Error", "Database access error", ABORT);
    } else if (error_ == ERROR_NOFILES) {
        nav_.display_modal("Error", "No database available", ABORT);
    }
    button_exit.focus();
}

void DroneFrequencyManagerBaseView::paint(Painter& painter) {
    View::paint(painter);
    // Base paint implementation - subclasses override
}

bool DroneFrequencyManagerBaseView::on_key(const KeyEvent key) {
    switch (key) {
        case KeyEvent::Back:
            nav_.pop();
            return true;
        default:
            return View::on_key(key);
    }
}

void DroneFrequencyManagerBaseView::ensure_database_initialized() {
    if (!global_frequency_db) {
        global_frequency_db = new DroneFrequencyDatabase();
    }
}

// HELPER FUNCTIONS
static const char* get_drone_type_name(DroneType type) {
    switch (type) {
        case DroneType::UNKNOWN: return "UNKNOWN";
        case DroneType::DJI_MAVIC: return "DJI MAVIC";
        case DroneType::DJI_MINI: return "DJI MINI";
        case DroneType::DJI_AIR: return "DJI AIR";
        case DroneType::DJI_PHANTOM: return "DJI PHANTOM";
        case DroneType::DJI_AVATA: return "DJI AVATA";
        case DroneType::FPV_RACING: return "FPV RACING";
        case DroneType::FPV_FREESTYLE: return "FPV FREESTYLE";
        case DroneType::ORLAN_10: return "ORLAN-10";
        case DroneType::LANCET: return "LANCET";
        case DroneType::SHAHED_136: return "SHAHED-136";
        case DroneType::BAYRAKTAR_TB2: return "BAYRAKTAR TB2";
        case DroneType::MILITARY_UAV: return "MILITARY UAV";
        case DroneType::RUSSIAN_EW_STATION: return "EW STATION";
        case DroneType::FIBER_OPTIC_DRONE: return "FIBER OPTIC";
        default: return "UNKNOWN";
    }
}

static const char* get_threat_level_name(ThreatLevel level) {
    switch (level) {
        case ThreatLevel::NONE: return "NONE";
        case ThreatLevel::LOW: return "LOW";
        case ThreatLevel::MEDIUM: return "MEDIUM";
        case ThreatLevel::HIGH: return "HIGH";
        case ThreatLevel::CRITICAL: return "CRITICAL";
        default: return "NONE";
    }
}

static std::vector<std::string> get_drone_type_options() {
    return {
        "UNKNOWN", "DJI MAVIC", "DJI MINI", "DJI AIR", "DJI PHANTOM", "DJI AVATA",
        "FPV RACING", "FPV FREESTYLE", "ORLAN-10", "LANCET", "SHAHED-136", "BAYRAKTAR TB2",
        "MILITARY UAV", "EW STATION", "FIBER OPTIC"
    };
}

static std::vector<std::string> get_threat_level_options() {
    return {"NONE", "LOW", "MEDIUM", "HIGH", "CRITICAL"};
}

DroneFrequencyManagerView::DroneFrequencyManagerView(NavigationView& nav)
    : nav_(nav)
{
    // Initialize global database access
    if (!global_frequency_db) {
        global_frequency_db = new DroneFrequencyDatabase();
    }

    initialize_menu_options();

    // Menu selection handler
    menu_view_.on_selected = [this](size_t index) {
        selected_frequency_index_ = index;
        on_frequency_selected(index);
    };

    // Button handlers
    button_add_.on_select = [this]() { on_add_frequency(); };
    button_update_.on_select = [this]() { on_update_frequency(); };
    button_remove_.on_select = [this]() { on_remove_frequency(); };
    button_clear_.on_select = [this]() { on_clear_all(); };

    // Add child views
    add_child(&menu_view_);
    add_child(&frequency_field_);
    add_child(&drone_type_field_);
    add_child(&threat_level_field_);
    add_child(&rssi_field_);
    add_child(&text_name_);
    add_child(&bandwidth_field_);
    add_child(&enabled_checkbox_);
    add_child(&button_add_);
    add_child(&button_update_);
    add_child(&button_remove_);
    add_child(&button_clear_);
    add_child(&text_status_);

    // NEW: Initialize bandwidth offset controls
    bandwidth_offset_field_.set_style(Theme::getInstance()->bg_darkest);
    // Direction options: signs are handled by the field value (±)
    offset_direction_field_.add_option(0, "Offset");
    offset_direction_field_.set_selected_index(0);  // Always "Offset"

    // Set initial values
    update_status_text("Select frequency to edit...");
    enabled_checkbox_.set_text("Enabled");
    enabled_checkbox_.set_value(true);

    refresh_frequency_list();
}

void DroneFrequencyManagerView::initialize_menu_options() {
    if (!global_frequency_db) return;

    // Clear existing menu
    menu_view_.clear();

    // Add frequency entries to menu
    for (size_t i = 0; i < global_frequency_db->size(); ++i) {
        const auto* entry = global_frequency_db->get_entry(i);
        if (entry) {
            char buffer[64];
            // Format: "433MHz Mavic HIGH"
            float freq_mhz = static_cast<float>(entry->frequency_hz) / 1000000.0f;
            snprintf(buffer, sizeof(buffer), "%.1fMHz %s %s",
                    freq_mhz,
                    get_drone_type_name(entry->drone_type),
                    get_threat_level_name(entry->threat_level));
            menu_view_.insert(i, buffer);
        }
    }

    // Set options field data
    std::vector<std::string> drone_types = get_drone_type_options();
    std::vector<std::string> threat_levels = get_threat_level_options();

    for (size_t i = 0; i < drone_types.size(); ++i) {
        drone_type_field_.add_option(i, drone_types[i]);
    }

    for (size_t i = 0; i < threat_levels.size(); ++i) {
        threat_level_field_.add_option(i, threat_levels[i]);
    }
}

void DroneFrequencyManagerView::on_frequency_selected(size_t index) {
    if (!global_frequency_db || index >= global_frequency_db->size()) return;

    const auto* entry = global_frequency_db->get_entry(index);
    if (!entry) return;

    // Update form fields with selected entry
    frequency_field_.set_value(entry->frequency_hz);
    drone_type_field_.set_selected_index(static_cast<size_t>(entry->drone_type));
    threat_level_field_.set_selected_index(static_cast<size_t>(entry->threat_level));
    rssi_field_.set_value(entry->rssi_threshold_db);
    text_name_.set(entry->name);
    bandwidth_field_.set_value(entry->bandwidth_hz);
    enabled_checkbox_.set_value(true);  // All entries are considered enabled

    char status[64];
    snprintf(status, sizeof(status), "Editing: %.1f MHz %s",
            static_cast<float>(entry->frequency_hz) / 1000000.0f,
            get_drone_type_name(entry->drone_type));
    update_status_text(status);
}

void DroneFrequencyManagerView::on_add_frequency() {
    if (!global_frequency_db) {
        update_status_text("Database not available");
        return;
    }

    if (global_frequency_db->size() >= global_frequency_db->max_size()) {
        update_status_text("Database full (64 entries max)");
        return;
    }

    if (!validate_current_form()) {
        update_status_text("Invalid frequency data");
        return;
    }

    auto form_data = get_form_data();
    DroneFrequencyEntry new_entry{
        form_data.frequency_hz,
        form_data.drone_type,
        form_data.threat_level,
        form_data.rssi_threshold_db,
        form_data.name.c_str(),
        form_data.bandwidth_hz
    };

    if (global_frequency_db->add_entry(new_entry)) {
        update_status_text("Frequency added successfully");
        refresh_frequency_list();
    } else {
        update_status_text("Failed to add frequency");
    }
}

void DroneFrequencyManagerView::on_update_frequency() {
    if (!global_frequency_db || selected_frequency_index_ >= global_frequency_db->size()) {
        update_status_text("No frequency selected");
        return;
    }

    if (!validate_current_form()) {
        update_status_text("Invalid frequency data");
        return;
    }

    auto form_data = get_form_data();
    DroneFrequencyEntry updated_entry{
        form_data.frequency_hz,
        form_data.drone_type,
        form_data.threat_level,
        form_data.rssi_threshold_db,
        form_data.name.c_str(),
        form_data.bandwidth_hz
    };

    if (global_frequency_db->update_entry(selected_frequency_index_, updated_entry)) {
        update_status_text("Frequency updated successfully");
        refresh_frequency_list();
    } else {
        update_status_text("Failed to update frequency");
    }
}

void DroneFrequencyManagerView::on_remove_frequency() {
    if (!global_frequency_db || selected_frequency_index_ >= global_frequency_db->size()) {
        update_status_text("No frequency selected");
        return;
    }

    if (global_frequency_db->remove_entry(selected_frequency_index_)) {
        update_status_text("Frequency removed successfully");
        // Reset selection if we removed the last item
        if (selected_frequency_index_ >= global_frequency_db->size()) {
            selected_frequency_index_ = global_frequency_db->size() > 0 ? global_frequency_db->size() - 1 : 0;
        }
        refresh_frequency_list();
        clear_form();
    } else {
        update_status_text("Failed to remove frequency");
    }
}

void DroneFrequencyManagerView::on_clear_all() {
    if (!global_frequency_db) return;

    // Clear confirmation dialog would be good here, but for simplicity:
    global_frequency_db->clear_database();
    update_status_text("All frequencies cleared");
    refresh_frequency_list();
    clear_form();
}

bool DroneFrequencyManagerView::validate_current_form() {
    auto freq = frequency_field_.get_value();
    if (freq < 50000000 || freq > 6000000000) return false;

    auto name = text_name_.get_value();
    if (name.empty() || name.length() > 24) return false;

    return true;
}

FrequencyEntryForm DroneFrequencyManagerView::get_form_data() const {
    return {
        static_cast<uint32_t>(frequency_field_.get_value()),
        static_cast<DroneType>(drone_type_field_.get_selected_index()),
        static_cast<ThreatLevel>(threat_level_field_.get_selected_index()),
        static_cast<int32_t>(rssi_field_.get_value()),
        text_name_.get_value(),
        static_cast<uint32_t>(bandwidth_field_.get_value())
    };
}

void DroneFrequencyManagerView::refresh_frequency_list() {
    initialize_menu_options();

    // Update selection display if still valid
    if (global_frequency_db && selected_frequency_index_ < global_frequency_db->size()) {
        menu_view_.set_selected_index(selected_frequency_index_);
    }
}

void DroneFrequencyManagerView::update_status_text(const char* text) {
    text_status_.set(text);
}

void DroneFrequencyManagerView::clear_form() {
    frequency_field_.set_value(433000000);  // Default ISM frequency
    drone_type_field_.set_selected_index(0);
    threat_level_field_.set_selected_index(0);
    rssi_field_.set_value(-80);
    text_name_.set("");
    bandwidth_field_.set_value(5000000);
    enabled_checkbox_.set_value(true);
    update_status_text("Ready to add new frequency...");
}

void DroneFrequencyManagerView::focus() {
    menu_view_.focus();
}

void DroneFrequencyManagerView::paint(Painter& painter) {
    View::paint(painter);

    // Draw divider line between menu and form
    painter.draw_vline({200, 0, screen_height}, Theme::getInstance()->bg_dark->foreground);

    // Header text
    painter.draw_string({202, 5}, Theme::getInstance()->fg_light->foreground, "Frequency (Hz)");
    painter.draw_string({202, 35}, Theme::getInstance()->fg_light->foreground, "Drone Type");
    painter.draw_string({202, 55}, Theme::getInstance()->fg_light->foreground, "Threat Level");
    painter.draw_string({202, 75}, Theme::getInstance()->fg_light->foreground, "RSSI (dB)");
    painter.draw_string({202, 95}, Theme::getInstance()->fg_light->foreground, "Name");
    painter.draw_string({202, 115}, Theme::getInstance()->fg_light->foreground, "Bandwidth (Hz)");
    painter.draw_string({202, 125}, Theme::getInstance()->fg_light->foreground, "Offset ±6MHz");
}

bool DroneFrequencyManagerView::on_key(const KeyEvent key) {
    switch (key) {
        case KeyEvent::Back:
            nav_.pop();
            return true;
        default:
            break;
    }
    return View::on_key(key);
}

bool DroneFrequencyManagerView::on_touch(const TouchEvent event) {
    return View::on_touch(event);
}

// DRONE FREQUENCY EDIT VIEW IMPLEMENTATION (Modal editing dialog)
DroneFrequencyEditView::DroneFrequencyEditView(
    NavigationView& nav, size_t edit_index)
    : nav_(nav), edit_index_(edit_index), error_(ERROR_NONE)
{
    add_children({&labels_,
                 &frequency_field_,
                 &drone_type_field_,
                 &threat_level_field_,
                 &rssi_field_,
                 &text_name_,
                 &bandwidth_field_,
                 &offset_field_,
                 &enabled_checkbox_,
                 &button_save_,
                 &button_cancel_,
                 &text_validation_});

    // Initialize form fields
    initialize_fields();

    // Populate dropdown options
    populate_options();

    // Load data if editing existing entry
    if (edit_index_ < (size_t)-1 && global_frequency_db) {
        auto* entry = global_frequency_db->get_entry(edit_index_);
        if (entry) {
            set_form_data(*entry);
        }
    }

    // Set up button callbacks
    button_save_.on_select = [this](Button&) {
        if (validate_form() && save_frequency()) {
            if (on_frequency_saved) on_frequency_saved();
            nav_.pop();
        }
    };

    button_cancel_.on_select = [this](Button&) {
        nav_.pop();
    };

    update_validation_text(true);
}

void DroneFrequencyEditView::initialize_fields() {
    frequency_field_.set_decimals(3);
    rssi_field_.set_range(-120, -10);

    enabled_checkbox_.set_text("Enabled");
    enabled_checkbox_.set_value(true);

    // Set default values for new entries
    frequency_field_.set_value(433000000);  // 433 MHz ISM
    rssi_field_.set_value(-80);
    bandwidth_field_.set_value(5000000);    // 5 MHz
    offset_field_.set_value(3000000);       // +3 MHz offset

    update_validation_text(true);
}

void DroneFrequencyEditView::populate_options() {
    // Populate drone types
    auto drone_types = get_drone_type_options();
    for (size_t i = 0; i < drone_types.size(); ++i) {
        drone_type_field_.add_option(i, drone_types[i]);
    }

    // Populate threat levels
    auto threat_levels = get_threat_level_options();
    for (size_t i = 0; i < threat_levels.size(); ++i) {
        threat_level_field_.add_option(i, threat_levels[i]);
    }
}

void DroneFrequencyEditView::focus() {
    button_cancel_.focus();
}

void DroneFrequencyEditView::paint(Painter& painter) {
    View::paint(painter);
}

bool DroneFrequencyEditView::on_key(const KeyEvent key) {
    switch (key) {
        case KeyEvent::Back:
            nav_.pop();
            return true;
        default:
            return View::on_key(key);
    }
}

bool DroneFrequencyEditView::validate_form() {
    uint64_t freq = frequency_field_.get_value();
    if (freq < 50000000 || freq > 6000000000) {
        update_validation_text(false);
        return false;
    }

    std::string name = text_name_.get_value();
    if (name.empty() || name.length() > 24) {
        update_validation_text(false);
        return false;
    }

    update_validation_text(true);
    return true;
}

bool DroneFrequencyEditView::save_frequency() {
    if (!global_frequency_db) return false;

    auto form_data = get_form_data();

    // Check database limits
    if (edit_index_ == (size_t)-1 &&
        global_frequency_db->size() >= global_frequency_db->max_size()) {
        return false;
    }

    DroneFrequencyEntry entry{
        form_data.frequency_hz,
        form_data.drone_type,
        form_data.threat_level,
        form_data.rssi_threshold_db,
        form_data.name.c_str(),
        form_data.bandwidth_hz
    };

    if (edit_index_ == (size_t)-1) {
        // Add new entry
        return global_frequency_db->add_entry(entry);
    } else {
        // Update existing entry
        return global_frequency_db->update_entry(edit_index_, entry);
    }
}

FrequencyEntryForm DroneFrequencyEditView::get_form_data() const {
    return FrequencyEntryForm{
        static_cast<uint32_t>(frequency_field_.get_value()),
        static_cast<DroneType>(drone_type_field_.get_selected_index()),
        static_cast<ThreatLevel>(threat_level_field_.get_selected_index()),
        static_cast<int32_t>(rssi_field_.get_value()),
        text_name_.get_value(),
        static_cast<uint32_t>(bandwidth_field_.get_value()),
        enabled_checkbox_.value()
    };
}

void DroneFrequencyEditView::set_form_data(const FrequencyEntryForm& data) {
    frequency_field_.set_value(data.frequency_hz);
    drone_type_field_.set_selected_index(static_cast<size_t>(data.drone_type));
    threat_level_field_.set_selected_index(static_cast<size_t>(data.threat_level));
    rssi_field_.set_value(data.rssi_threshold_db);
    text_name_.set(data.name);
    bandwidth_field_.set_value(data.bandwidth_hz);
    enabled_checkbox_.set_value(data.enabled);
}

void DroneFrequencyEditView::update_validation_text(bool valid) {
    if (valid) {
        text_validation_.set("Form is valid");
        text_validation_.set_style(Theme::getInstance()->fg_green);
    } else {
        text_validation_.set("Please check required fields");
        text_validation_.set_style(Theme::getInstance()->fg_red);
    }
}

} // namespace ui::external_app::enhanced_drone_analyzer
