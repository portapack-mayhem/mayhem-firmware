#ifndef DRONE_SWEEP_VIEW_HPP
#define DRONE_SWEEP_VIEW_HPP

#include <cstdint>
#include <cstddef>

#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_tabview.hpp"

#include "drone_types.hpp"
#include "constants.hpp"
#include "scanner.hpp"

namespace drone_analyzer {

class DroneScanner;

/**
 * @brief Child view for sweep windows 1-2 configuration (Tab 1)
 * @note Contains Start/End/Step for Window 1 + Enabled/Start/End/Step for Window 2
 */
class SweepWindowGroup1View : public ui::View {
public:
    SweepWindowGroup1View(NavigationView& nav, const Rect parent_rect) noexcept;

    void focus() override;

    ui::Labels labels_{
        {{UI_POS_X(0), UI_POS_Y(0)}, "-- Window 1 --", Theme::getInstance()->fg_cyan->foreground},
        {{UI_POS_X(1), UI_POS_Y(1)}, "Start(MHz):", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(1), UI_POS_Y(3)}, "End(MHz):", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(1), UI_POS_Y(5)}, "Step(kHz):", Theme::getInstance()->fg_light->foreground},
    };
    ui::NumberField field_sw1_start_{{UI_POS_X(1), UI_POS_Y(2)}, 5, {100, 7200}, 1, ' '};
    ui::NumberField field_sw1_end_{{UI_POS_X(1), UI_POS_Y(4)}, 5, {100, 7200}, 1, ' '};
    ui::NumberField field_sw1_step_{{UI_POS_X(1), UI_POS_Y(6)}, 5, {1000, 99999}, 1000, ' '};

    ui::Labels labels_sw2_{
        {{UI_POS_X(0), UI_POS_Y(8)}, "-- Window 2 --", Theme::getInstance()->fg_green->foreground},
        {{UI_POS_X(1), UI_POS_Y(10)}, "Start(MHz):", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(1), UI_POS_Y(12)}, "End(MHz):", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(1), UI_POS_Y(14)}, "Step(kHz):", Theme::getInstance()->fg_light->foreground},
    };
    ui::Checkbox check_sw2_enabled_{{UI_POS_X(1), UI_POS_Y(9)}, 8, "Enabled", false};
    ui::NumberField field_sw2_start_{{UI_POS_X(1), UI_POS_Y(11)}, 5, {100, 7200}, 1, ' '};
    ui::NumberField field_sw2_end_{{UI_POS_X(1), UI_POS_Y(13)}, 5, {100, 7200}, 1, ' '};
    ui::NumberField field_sw2_step_{{UI_POS_X(1), UI_POS_Y(15)}, 5, {1000, 99999}, 1000, ' '};

    // Exception fields — right side of Window 1 (5 slots)
    ui::Labels labels_exc1_{
        {{UI_POS_X(16), UI_POS_Y(0)}, "Exc(MHz):", Theme::getInstance()->fg_yellow->foreground},
    };
    ui::NumberField field_sw1_exc0_{{UI_POS_X(16), UI_POS_Y(1)}, 5, {0, 7200}, 1, ' '};
    ui::NumberField field_sw1_exc1_{{UI_POS_X(16), UI_POS_Y(2)}, 5, {0, 7200}, 1, ' '};
    ui::NumberField field_sw1_exc2_{{UI_POS_X(16), UI_POS_Y(3)}, 5, {0, 7200}, 1, ' '};
    ui::NumberField field_sw1_exc3_{{UI_POS_X(16), UI_POS_Y(4)}, 5, {0, 7200}, 1, ' '};
    ui::NumberField field_sw1_exc4_{{UI_POS_X(16), UI_POS_Y(5)}, 5, {0, 7200}, 1, ' '};

    // Exception fields — right side of Window 2 (5 slots)
    ui::Labels labels_exc2_{
        {{UI_POS_X(16), UI_POS_Y(8)}, "Exc(MHz):", Theme::getInstance()->fg_yellow->foreground},
    };
    ui::NumberField field_sw2_exc0_{{UI_POS_X(16), UI_POS_Y(9)}, 5, {0, 7200}, 1, ' '};
    ui::NumberField field_sw2_exc1_{{UI_POS_X(16), UI_POS_Y(10)}, 5, {0, 7200}, 1, ' '};
    ui::NumberField field_sw2_exc2_{{UI_POS_X(16), UI_POS_Y(11)}, 5, {0, 7200}, 1, ' '};
    ui::NumberField field_sw2_exc3_{{UI_POS_X(16), UI_POS_Y(12)}, 5, {0, 7200}, 1, ' '};
    ui::NumberField field_sw2_exc4_{{UI_POS_X(16), UI_POS_Y(13)}, 5, {0, 7200}, 1, ' '};
};

/**
 * @brief Child view for sweep windows 3-4 configuration (Tab 2)
 * @note Contains Enabled/Start/End/Step for Window 3 and Window 4
 * @note All disabled by default
 */
class SweepWindowGroup2View : public ui::View {
public:
    SweepWindowGroup2View(NavigationView& nav, const Rect parent_rect) noexcept;

    void focus() override;

    ui::Labels labels_sw3_{
        {{UI_POS_X(0), UI_POS_Y(0)}, "-- Window 3 --", Theme::getInstance()->fg_yellow->foreground},
        {{UI_POS_X(1), UI_POS_Y(2)}, "Start(MHz):", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(1), UI_POS_Y(4)}, "End(MHz):", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(1), UI_POS_Y(6)}, "Step(kHz):", Theme::getInstance()->fg_light->foreground},
    };
    ui::Checkbox check_sw3_enabled_{{UI_POS_X(1), UI_POS_Y(1)}, 8, "Enabled", false};
    ui::NumberField field_sw3_start_{{UI_POS_X(1), UI_POS_Y(3)}, 5, {100, 7200}, 1, ' '};
    ui::NumberField field_sw3_end_{{UI_POS_X(1), UI_POS_Y(5)}, 5, {100, 7200}, 1, ' '};
    ui::NumberField field_sw3_step_{{UI_POS_X(1), UI_POS_Y(7)}, 5, {1000, 99999}, 1000, ' '};

    ui::Labels labels_sw4_{
        {{UI_POS_X(0), UI_POS_Y(8)}, "-- Window 4 --", Theme::getInstance()->fg_orange->foreground},
        {{UI_POS_X(1), UI_POS_Y(10)}, "Start(MHz):", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(1), UI_POS_Y(12)}, "End(MHz):", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(1), UI_POS_Y(14)}, "Step(kHz):", Theme::getInstance()->fg_light->foreground},
    };
    ui::Checkbox check_sw4_enabled_{{UI_POS_X(1), UI_POS_Y(9)}, 8, "Enabled", false};
    ui::NumberField field_sw4_start_{{UI_POS_X(1), UI_POS_Y(11)}, 5, {100, 7200}, 1, ' '};
    ui::NumberField field_sw4_end_{{UI_POS_X(1), UI_POS_Y(13)}, 5, {100, 7200}, 1, ' '};
    ui::NumberField field_sw4_step_{{UI_POS_X(1), UI_POS_Y(15)}, 5, {1000, 99999}, 1000, ' '};

    // Exception fields — right side of Window 3 (5 slots)
    ui::Labels labels_exc3_{
        {{UI_POS_X(16), UI_POS_Y(0)}, "Exc(MHz):", Theme::getInstance()->fg_yellow->foreground},
    };
    ui::NumberField field_sw3_exc0_{{UI_POS_X(16), UI_POS_Y(1)}, 5, {0, 7200}, 1, ' '};
    ui::NumberField field_sw3_exc1_{{UI_POS_X(16), UI_POS_Y(2)}, 5, {0, 7200}, 1, ' '};
    ui::NumberField field_sw3_exc2_{{UI_POS_X(16), UI_POS_Y(3)}, 5, {0, 7200}, 1, ' '};
    ui::NumberField field_sw3_exc3_{{UI_POS_X(16), UI_POS_Y(4)}, 5, {0, 7200}, 1, ' '};
    ui::NumberField field_sw3_exc4_{{UI_POS_X(16), UI_POS_Y(5)}, 5, {0, 7200}, 1, ' '};

    // Exception fields — right side of Window 4 (5 slots)
    ui::Labels labels_exc4_{
        {{UI_POS_X(16), UI_POS_Y(8)}, "Exc(MHz):", Theme::getInstance()->fg_yellow->foreground},
    };
    ui::NumberField field_sw4_exc0_{{UI_POS_X(16), UI_POS_Y(9)}, 5, {0, 7200}, 1, ' '};
    ui::NumberField field_sw4_exc1_{{UI_POS_X(16), UI_POS_Y(10)}, 5, {0, 7200}, 1, ' '};
    ui::NumberField field_sw4_exc2_{{UI_POS_X(16), UI_POS_Y(11)}, 5, {0, 7200}, 1, ' '};
    ui::NumberField field_sw4_exc3_{{UI_POS_X(16), UI_POS_Y(12)}, 5, {0, 7200}, 1, ' '};
    ui::NumberField field_sw4_exc4_{{UI_POS_X(16), UI_POS_Y(13)}, 5, {0, 7200}, 1, ' '};
};

/**
 * @brief Sweep settings view — accessible via SWP button
 * @note TabView layout: Tab 1 = Win 1-2, Tab 2 = Win 3-4
 * @note Save writes sweep keys to SETTINGS/eda_settings.txt
 * @note Defaults resets all sweep windows to factory values (Win 3-4 disabled)
 */
class DroneSweepView : public ui::View {
public:
    explicit DroneSweepView(NavigationView& nav, const ScanConfig& config, DroneScanner* scanner_ptr) noexcept;

    ~DroneSweepView() noexcept override;

    DroneSweepView(const DroneSweepView&) = delete;
    DroneSweepView& operator=(const DroneSweepView&) = delete;

    void focus() override;

    std::string title() const override { return "SWP Settings"; }

private:
    NavigationView& nav_;
    DroneScanner* scanner_ptr_;
    ScanConfig original_config_;

    // Child views for tabs
    static constexpr ui::Dim TAB_BAR_H = 24;
    Rect tab_content_rect_{0, TAB_BAR_H, screen_width, screen_height - TAB_BAR_H};

    SweepWindowGroup1View view_group1_{nav_, tab_content_rect_};
    SweepWindowGroup2View view_group2_{nav_, tab_content_rect_};

    ui::TabView tab_view_{
        {"Win 1-2", Theme::getInstance()->fg_cyan->foreground, &view_group1_},
        {"Win 3-4", Theme::getInstance()->fg_yellow->foreground, &view_group2_},
    };

    // Buttons (below tab content area)
    ui::NumberField field_exc_radius_{{UI_POS_X(0), 285}, 3, {1, 100}, 1, ' '};
    ui::Labels labels_exc_radius_{
        {{UI_POS_X(4), 285}, "Exc R(MHz):", Theme::getInstance()->fg_light->foreground},
    };
    ui::Button button_defaults_{{UI_POS_X(15), 285, UI_POS_WIDTH(7), 20}, "DEFAULTS"};
    ui::Button button_save_{{UI_POS_X(22), 285, UI_POS_WIDTH(7), 20}, "SAVE"};

    void save_settings() noexcept;
    void apply_defaults() noexcept;
};

} // namespace drone_analyzer

#endif // DRONE_SWEEP_VIEW_HPP
