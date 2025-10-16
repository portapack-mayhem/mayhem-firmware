// ui_drone_frequency_manager.cpp
// UI event handlers and CRUD operations for drone frequency management

#include "ui_drone_frequency_manager.hpp"
#include "binder.hpp"
#include "portapack.hpp"
#include "freqman_db.hpp"
#include "file_path.hpp"
#include "freqman.hpp"
#include "string_format.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

DroneFrequencyManagerView::DroneFrequencyManagerView(NavigationView& nav)
    : nav_(nav), freqman_file_("DRONES"), selected_frequency_index_(0) {

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
        &button_preset_,  // NEW: Add preset button to child components
        &button_add_,
        &button_update_,
        &button_remove_,
        &button_clear_,
        &text_status_
    });

    // Initialize UI components
    initialize_menu_options();

    // Set up button handlers (pattern from FreqMan)
    button_preset_.on_select = [this]() { on_add_preset(); };  // NEW: Preset button handler
    button_add_.on_select = [this]() { on_add_frequency(); };
    button_update_.on_select = [this]() { on_update_frequency(); };
    button_remove_.on_select = [this]() { on_remove_frequency(); };
    button_clear_.on_select = [this]() { on_clear_all(); };

    // Initialize frequency field
    frequency_field_.set_step(5000000); // 5MHz steps

    // Initialize both databases - DroneFrequencyDatabase for drone-specific data
    std::filesystem::path db_path = get_freqman_path(freqman_file_);
    if (!drone_db_.open(db_path, true)) {
        // Fallback to basic FreqmanDB if DroneFrequencyDatabase fails
        update_status_text("Using basic database mode");
    }

    // Load frequencies from database
    load_frequencies_from_database();
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
        update_status_text("Invalid data - check all fields");
        return;
    }

    auto form_data = get_form_data();

    // Create DroneFrequencyEntry for drone database
    DroneFrequencyEntry drone_entry{
        .frequency_hz = form_data.frequency_hz,
        .drone_type = form_data.drone_type,
        .threat_level = form_data.threat_level,
        .rssi_threshold_db = form_data.rssi_threshold_db,
        .bandwidth_idx = 0,  // Default - can be calculated from bandwidth_hz later
        .name_offset = 0
    };
    drone_entry.name = form_data.name;

    // Check for duplicates in drone database
    if (drone_db_.lookup_frequency(form_data.frequency_hz) != nullptr) {
        update_status_text("Frequency already exists");
        return;
    }

    // Add to drone database first
    if (drone_db_.add_drone_frequency(drone_entry, false)) {
        drone_db_.save();  // Persist changes
        refresh_frequency_list();
        clear_form();
        update_status_text("Frequency added successfully");
    } else {
        update_status_text("Failed to save frequency");
    }

    // BACKWARD COMPATIBILITY: Also update FreqmanDB for compatibility
    freqman_entry entry{
        .frequency_a = static_cast<rf::Frequency>(form_data.frequency_hz),
        .frequency_b = static_cast<rf::Frequency>(form_data.frequency_hz),  // Для single частоты
        .type = freqman_type::Single,
        .modulation = freqman_index_t(1),  // NFM по умолчанию для дронов
        .bandwidth = freqman_index_t(3),   // 25kHz по умолчанию
        .step = freqman_index_t(5),        // 25kHz шаг
        .description = form_data.name,
        .tonal = "",                       // Не используется для дронов
    };

    // Open and save to DRONES.TXT (FreqmanDB fallback)
    if (freqman_db_.open(get_freqman_path(freqman_file_), true)) {
        freqman_db_.append_entry(entry);
        freqman_db_.save();
    }
}

void DroneFrequencyManagerView::on_update_frequency() {
    // Update existing entry in FreqmanDB
    if (selected_frequency_index_ >= freqman_db_.entry_count()) {
        update_status_text("No frequency selected");
        return;
    }

    if (!validate_current_form()) {
        update_status_text("Invalid data - check all fields");
        return;
    }

    auto form_data = get_form_data();

    // Обновить существующую freqman_entry
    if (freqman_db_[selected_frequency_index_].type == freqman_type::Single) {
        freqman_db_[selected_frequency_index_].frequency_a = static_cast<rf::Frequency>(form_data.frequency_hz);
        freqman_db_[selected_frequency_index_].frequency_b = static_cast<rf::Frequency>(form_data.frequency_hz);
    }
    freqman_db_[selected_frequency_index_].description = form_data.name;

    // Сохранить изменения
    freqman_db_.save();

    refresh_frequency_list();
    update_frequency_details();
    update_status_text("Frequency updated successfully");
}

void DroneFrequencyManagerView::on_add_preset() {
    // NEW: Show preset selection menu for quick setup
    DronePresetSelector::show_preset_menu(nav_,
        [this](const DronePreset& selected_preset) {
            // Auto-populate form with preset data
            fill_form_from_preset(selected_preset);
            update_status_text(("Preset '" + selected_preset.display_name + "' loaded").c_str());
        });
}

void DroneFrequencyManagerView::fill_form_from_preset(const DronePreset& preset) {
    // Populate UI fields with preset data
    frequency_field_.set_value(preset.frequency_hz);
    drone_type_field_.set_selected_index(static_cast<size_t>(preset.drone_type));
    threat_level_field_.set_selected_index(static_cast<size_t>(preset.threat_level));
    rssi_field_.set_value(preset.rssi_threshold_db);
    text_name_.set(preset.name_template);

    // Auto-generate unique name by appending timestamp or number
    std::string unique_name = preset.name_template + " " +
                             std::to_string(freqman_db_.entry_count() + 1);
    if (unique_name.length() > 20) {
        unique_name = unique_name.substr(0, 20);  // Truncate if too long
    }
    text_name_.set(unique_name);
}

void DroneFrequencyManagerView::on_remove_frequency() {
    // Remove entry from FreqmanDB
    if (selected_frequency_index_ >= freqman_db_.entry_count()) {
        update_status_text("No frequency selected");
        return;
    }

    nav_.push<ModalMessageView>(
        "Delete", "Delete selected frequency?",
        YESNO,
        [this](bool choice) {
            if (choice) {
                // Удалить запись из FreqmanDB
                freqman_db_.erase(freqman_db_.begin() + selected_frequency_index_);
                freqman_db_.save();

                if (selected_frequency_index_ >= freqman_db_.entry_count() && freqman_db_.entry_count() > 0) {
                    selected_frequency_index_ = freqman_db_.entry_count() - 1;
                } else {
                    selected_frequency_index_ = 0;
                }

                refresh_frequency_list();
                update_frequency_details();
                update_status_text("Frequency deleted");
            }
        });
}

void DroneFrequencyManagerView::on_clear_all() {
    // Clear all entries from FreqmanDB
    if (freqman_db_.entry_count() == 0) {
        update_status_text("No frequencies to clear");
        return;
    }

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
                            // Очистить все и сохранить пустую БД
                            freqman_db_.clear();
                            freqman_db_.save();

                            selected_frequency_index_ = 0;
                            refresh_frequency_list();
                            clear_form();
                            update_status_text("All frequencies cleared");
                        }
                    });
            }
        });
}

void DroneFrequencyManagerView::update_frequency_details() {
    // Update UI fields based on selected frequency from FreqmanDB
    if (selected_frequency_index_ >= freqman_db_.entry_count()) {
        clear_form();
        return;
    }

    const freqman_entry& entry = freqman_db_[selected_frequency_index_];

    // Load frequency and description from FreqmanDB entry
    frequency_field_.set_value(entry.frequency_a);
    text_name_.set(entry.description);

    // For now, set default values for drone-specific fields
    // TODO: Later we could encode drone type/threat in description field
    drone_type_field_.set_selected_index(static_cast<size_t>(DroneType::UNKNOWN));
    threat_level_field_.set_selected_index(static_cast<size_t>(ThreatLevel::LOW));
    rssi_field_.set_value(-85);  // Default RSSI threshold for drones
    bandwidth_field_.set_value(20000000);  // Default bandwidth

    enabled_checkbox_.set_value(true);
    update_status_text("Entry loaded");
}

bool DroneFrequencyManagerView::validate_current_form() {
    // Enhanced validation based on Level/Scanner patterns
    rf::Frequency freq = frequency_field_.get_value();
    ThreatLevel threat = static_cast<ThreatLevel>(threat_level_field_.selected_index_value());
    DroneType drone_type = static_cast<DroneType>(drone_type_field_.selected_index_value());
    int32_t rssi_threshold = rssi_field_.get_value();
    std::string name = text_name_.get_string();
    uint32_t bandwidth = bandwidth_field_.get_value();

    // Validate frequency range (Portapack hardware constraints)
    if (freq < MIN_HARDWARE_FREQ || freq > MAX_HARDWARE_FREQ) {
        return false;
    }

    // Validate specific drone frequency ranges (safety constraint)
    if (freq < MIN_DRONE_FREQUENCY || freq > MAX_DRONE_FREQUENCY) {
        return false;
    }

    // Validate threat level bounds
    if (static_cast<size_t>(threat) > static_cast<size_t>(ThreatLevel::CRITICAL)) {
        return false;
    }

    // Validate drone type bounds
    if (static_cast<size_t>(drone_type) >= static_cast<size_t>(DroneType::MAX_TYPES)) {
        return false;
    }

    // Validate RSSI threshold range (realistic detection bounds)
    if (rssi_threshold < -120 || rssi_threshold > -10) {
        return false;
    }

    // Validate bandwidth within drone operation ranges
    if (bandwidth < 1000000 || bandwidth > 100000000) { // 1MHz to 100MHz
        return false;
    }

    // Validate name length (UI constraint)
    if (name.empty() || name.length() > 20) {
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
    // Following FreqManUIList pattern, recreate menu items
    // Clear existing menu items
    menu_view_.get_menu_view().clear();

    size_t entry_count = freqman_db_.entry_count();
    if (entry_count == 0) {
        update_status_text("No frequencies loaded");
        return;
    }

    // Add each frequency to the menu
    for (size_t i = 0; i < entry_count; ++i) {
        const freqman_entry& entry = freqman_db_[i];

        // Format display text: "433.92MHz: DJI Drone A"
        std::string display_text = to_string_short_freq(entry.frequency_a) + ": " + entry.description;

        // Add menu item with callback
        menu_view_.get_menu_view().add_item({
            display_text,
            Theme::getInstance()->fg_light->foreground,
            nullptr,  // No bitmap
            [this, i]() {
                selected_frequency_index_ = i;
                update_frequency_details();
            }
        });
    }

    update_status_text(to_string_dec_uint(entry_count) + " frequencies loaded");
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

void DroneFrequencyManagerView::load_frequencies_from_database() {
    // Попытаться открыть или создать файл DRONES.TXT
    if (freqman_db_.open(get_freqman_path(freqman_file_), true)) {
        // Файл открыт, обновить меню частот
        refresh_frequency_list();

        if (freqman_db_.entry_count() > 0) {
            // Выбрать первую запись для отображения в форме
            selected_frequency_index_ = 0;
            update_frequency_details();
        } else {
            // Пустая база данных - форма останется очищенной
            clear_form();
            update_status_text("No frequencies loaded - add new ones");
        }
    } else {
        // Не удалось открыть/создать файл
        clear_form();
        update_status_text("Cannot access frequency database");
    }
}

} // namespace ui::external_app::enhanced_drone_analyzer
