// ui_drone_frequency_manager.cpp
// UI event handlers and CRUD operations for drone frequency management

#include "ui_drone_frequency_manager.hpp"
#include "binder.hpp"
#include "portapack.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

DroneFrequencyManagerView::DroneFrequencyManagerView(NavigationView& nav)
    : nav_(nav), selected_frequency_index_(0) {

    // Add all UI components
    add_children({
        &menu_view_,
        &frequency_field_,
        &drone_type_field_,
        &threat_level_field_,
        &rssi_field_,
        &text_name_,
        &bandwidth_field_,
        &bandwidth_offset_field_,
        &offset_direction_field_,
        &enabled_checkbox_,
        &button_add_,
        &button_update_,
        &button_remove_,
        &button_clear_,
        &text_status_
    });

    // Initialize UI components
    initialize_menu_options();

    // Set up button handlers (pattern from FreqMan)
    button_add_.on_select = [this]() { on_add_frequency(); };
    button_update_.on_select = [this]() { on_update_frequency(); };
    button_remove_.on_select = [this]() { on_remove_frequency(); };
    button_clear_.on_select = [this]() { on_clear_all(); };

    // Initialize frequency field
    frequency_field_.set_step(5000000); // 5MHz steps
    text_name_.set_length(24);

    // Load initial data if available
    if (menu_view_.get_menu_view().get_entry_count() > 0) {
        update_frequency_details();
    }
}

void DroneFrequencyManagerView::focus() {
    // Focus on menu first for navigation
    if (menu_view_.get_menu_view().get_entry_count() > 0) {
        button_add_.focus();
    } else {
        menu_view_.focus();
    }
}

void DroneFrequencyManagerView::paint(Painter& painter) {
    View::paint(painter);

    // Additional UI drawing similar to FreqMan
    // Draw lines or divisions if needed
}

bool DroneFrequencyManagerView::on_key(const KeyEvent key) {
    // Handle keyboard navigation (pattern from FreqMan)
    switch (key) {
        case KeyEvent::Left:
            // Navigate left
            return true;
        case KeyEvent::Right:
            // Navigate right
            return true;
        case KeyEvent::Up:
            // Move selection up
            return true;
        case KeyEvent::Down:
            // Move selection down
            return true;
        default:
            return View::on_key(key);
    }
}

bool DroneFrequencyManagerView::on_touch(const TouchEvent event) {
    // Handle touch events similar to FreqMan
    return View::on_touch(event);
}

void DroneFrequencyManagerView::on_frequency_selected(size_t index) {
    selected_frequency_index_ = index;
    update_frequency_details();
}

void DroneFrequencyManagerView::on_add_frequency() {
    // Validate form data first
    if (!validate_current_form()) {
        update_status_text("Invalid data - check frequency/threat");
        return;
    }

    auto new_entry = get_form_data();

    // Add to database (pattern from FreqMan on_add_entry)
    if (db_.add_entry(new_entry)) {
        refresh_frequency_list();
        update_status_text("Frequency added successfully");
        clear_form();
    } else {
        update_status_text("Failed to add - duplicate or DB full");
    }
}

void DroneFrequencyManagerView::on_update_frequency() {
    // Update existing entry (pattern from FreqMan)
    if (!validate_current_form()) {
        update_status_text("Invalid data - check frequency/threat");
        return;
    }

    auto updated_entry = get_form_data();

    if (db_.update_entry(selected_frequency_index_, updated_entry)) {
        refresh_frequency_list();
        update_frequency_details();
        update_status_text("Frequency updated successfully");
    } else {
        update_status_text("Failed to update frequency");
    }
}

void DroneFrequencyManagerView::on_remove_frequency() {
    // Remove entry (pattern from FreqMan on_del_entry)
    nav_.push<ModalMessageView>(
        "Delete", "Delete selected frequency?",
        YESNO,
        [this](bool choice) {
            if (choice) {
                if (db_.remove_entry(selected_frequency_index_)) {
                    refresh_frequency_list();
                    if (selected_frequency_index_ >= db_.size()) {
                        selected_frequency_index_ = 0;
                    }
                    update_frequency_details();
                    update_status_text("Frequency deleted");
                } else {
                    update_status_text("Delete failed");
                }
            }
        });
}

void DroneFrequencyManagerView::on_clear_all() {
    // Clear all entries (pattern from FreqMan)
    nav_.push<ModalMessageView>(
        "Clear All", "Delete ALL frequencies?",
        YESNO,
        [this](bool choice) {
            if (choice) {
                nav_.push<ModalMessageView>(
                    "Confirm", "This cannot be undone!",
                    YESNO,
                    [this](bool choice) {
                        if (choice) {
                            db_.clear_database();
                            refresh_frequency_list();
                            update_status_text("All frequencies cleared");
                        }
                    });
            }
        });
}

void DroneFrequencyManagerView::update_frequency_details() {
    // Update UI fields based on selected frequency
    const auto* entry = db_.get_entry(selected_frequency_index_);
    if (!entry) {
        clear_form();
        return;
    }

    // Update all UI fields
    frequency_field_.set_value(entry->frequency_hz);
    drone_type_field_.set_selected_index(static_cast<size_t>(entry->drone_type));
    threat_level_field_.set_selected_index(static_cast<size_t>(entry->threat_level));
    rssi_field_.set_value(entry->rssi_threshold_db);
    text_name_.set(entry->name);
    bandwidth_field_.set_value(entry->bandwidth_hz);
    enabled_checkbox_.set_value(true); // Assuming all entries are enabled

    update_status_text("Entry loaded");
}

bool DroneFrequencyManagerView::validate_current_form() {
    // Validate form data (pattern from FreqMan)
    rf::Frequency freq = frequency_field_.get_value();
    ThreatLevel threat = static_cast<ThreatLevel>(threat_level_field_.selected_index_value());

    if (freq < MIN_DRONE_FREQUENCY || freq > MAX_DRONE_FREQUENCY) {
        return false;
    }

    // Validate threat level is within bounds
    if (static_cast<size_t>(threat) >= static_cast<size_t>(ThreatLevel::UNKNOWN)) {
        return false;
    }

    return true;
}

FrequencyEntryForm DroneFrequencyManagerView::get_form_data() const {
    // Collect data from form fields
    FrequencyEntryForm form;
    form.frequency_hz = frequency_field_.get_value();
    form.drone_type = static_cast<DroneType>(drone_type_field_.selected_index_value());
    form.threat_level = static_cast<ThreatLevel>(threat_level_field_.selected_index_value());
    form.rssi_threshold_db = rssi_field_.get_value();
    form.name = text_name_.get_string();
    form.bandwidth_hz = bandwidth_field_.get_value();

    return form;
}

void DroneFrequencyManagerView::refresh_frequency_list() {
    // Refresh menu display (pattern from FreqMan refresh_categories)
    // This would rebuild the list view
}

void DroneFrequencyManagerView::update_status_text(const char* text) {
    text_status_.set(text);
}

void DroneFrequencyManagerView::clear_form() {
    // Clear all form fields
    frequency_field_.set_value(433920000); // Default ISM band
    drone_type_field_.set_selected_index(static_cast<size_t>(DroneType::UNKNOWN));
    threat_level_field_.set_selected_index(static_cast<size_t>(ThreatLevel::NONE));
    rssi_field_.set_value(-70);
    text_name_.set("");
    bandwidth_field_.set_value(20000000); // 20MHz default
    enabled_checkbox_.set_value(true);
}

void DroneFrequencyManagerView::initialize_menu_options() {
    // Initialize menu options (pattern from FreqMan)
    // Set up drone type options
    drone_type_field_.set_options({
        {"Unknown", static_cast<size_t>(DroneType::UNKNOWN)},
        {"DJI Mini", static_cast<size_t>(DroneType::DJI_MINI)},
        {"DJI Mavic", static_cast<size_t>(DroneType::DJI_MAVIC)},
        {"FPV Racing", static_cast<size_t>(DroneType::FPV_RACING)},
        {"Orlan-10", static_cast<size_t>(DroneType::ORLAN_10)},
        {"Lancet", static_cast<size_t>(DroneType::LANCET)},
    });

    // Set up threat level options
    threat_level_field_.set_options({
        {"None", static_cast<size_t>(ThreatLevel::NONE)},
        {"Low", static_cast<size_t>(ThreatLevel::LOW)},
        {"Medium", static_cast<size_t>(ThreatLevel::MEDIUM)},
        {"High", static_cast<size_t>(ThreatLevel::HIGH)},
        {"Critical", static_cast<size_t>(ThreatLevel::CRITICAL)},
    });

    // Set up bandwidth offset direction
    offset_direction_field_.set_options({
        {"-", 0},
        {"+", 1},
    });
}

} // namespace ui::external_app::enhanced_drone_analyzer
