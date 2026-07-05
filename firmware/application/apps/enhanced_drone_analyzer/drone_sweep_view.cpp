#include <cstdint>
#include <cstring>

#include "drone_sweep_view.hpp"
#include "drone_settings.hpp"
#include "settings_manager.hpp"
#include "scanner.hpp"
#include "constants.hpp"
#include "ui_receiver.hpp"
#include "file.hpp"
#include "file_path.hpp"
#include "baseband_api.hpp"

namespace drone_analyzer {

// ============================================================================
// SweepWindowGroup1View — Tab 1: Windows 1-2
// ============================================================================

SweepWindowGroup1View::SweepWindowGroup1View(NavigationView& nav, const Rect parent_rect) noexcept
    : ui::View()
    , nav_(nav) {
    set_parent_rect(parent_rect);
    add_children({
        &labels_,
        &field_sw1_start_,
        &field_sw1_end_,
        &field_sw1_step_,
        &labels_sw2_,
        &check_sw2_enabled_,
        &field_sw2_start_,
        &field_sw2_end_,
        &field_sw2_step_,
        &labels_exc1_,
        &field_sw1_exc0_,
        &field_sw1_exc1_,
        &field_sw1_exc2_,
        &field_sw1_exc3_,
        &field_sw1_exc4_,
        &labels_exc2_,
        &field_sw2_exc0_,
        &field_sw2_exc1_,
        &field_sw2_exc2_,
        &field_sw2_exc3_,
        &field_sw2_exc4_,
    });

    field_sw1_start_.on_select = [this](NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_sw1_start_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_sw1_start_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };

    field_sw1_end_.on_select = [this](NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_sw1_end_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_sw1_end_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };

    field_sw2_start_.on_select = [this](NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_sw2_start_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_sw2_start_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };

    field_sw2_end_.on_select = [this](NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_sw2_end_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_sw2_end_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };

    field_sw1_exc0_.on_select = [this](NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_sw1_exc0_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_sw1_exc0_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };

    field_sw1_exc1_.on_select = [this](NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_sw1_exc1_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_sw1_exc1_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };

    field_sw1_exc2_.on_select = [this](NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_sw1_exc2_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_sw1_exc2_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };

    field_sw2_exc0_.on_select = [this](NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_sw2_exc0_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_sw2_exc0_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };

    field_sw2_exc1_.on_select = [this](NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_sw2_exc1_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_sw2_exc1_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };

    field_sw2_exc2_.on_select = [this](NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_sw2_exc2_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_sw2_exc2_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };

    field_sw1_exc3_.on_select = [this](NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_sw1_exc3_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_sw1_exc3_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };

    field_sw1_exc4_.on_select = [this](NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_sw1_exc4_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_sw1_exc4_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };

    field_sw2_exc3_.on_select = [this](NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_sw2_exc3_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_sw2_exc3_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };

    field_sw2_exc4_.on_select = [this](NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_sw2_exc4_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_sw2_exc4_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };
}

void SweepWindowGroup1View::focus() {
    field_sw1_start_.focus();
}

// ============================================================================
// SweepWindowGroup2View — Tab 2: Windows 3-4
// ============================================================================

SweepWindowGroup2View::SweepWindowGroup2View(NavigationView& nav, const Rect parent_rect) noexcept
    : ui::View()
    , nav_(nav) {
    set_parent_rect(parent_rect);
    add_children({
        &labels_sw3_,
        &check_sw3_enabled_,
        &field_sw3_start_,
        &field_sw3_end_,
        &field_sw3_step_,
        &labels_sw4_,
        &check_sw4_enabled_,
        &field_sw4_start_,
        &field_sw4_end_,
        &field_sw4_step_,
        &labels_exc3_,
        &field_sw3_exc0_,
        &field_sw3_exc1_,
        &field_sw3_exc2_,
        &field_sw3_exc3_,
        &field_sw3_exc4_,
        &labels_exc4_,
        &field_sw4_exc0_,
        &field_sw4_exc1_,
        &field_sw4_exc2_,
        &field_sw4_exc3_,
        &field_sw4_exc4_,
    });

    field_sw3_start_.on_select = [this](NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_sw3_start_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_sw3_start_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };

    field_sw3_end_.on_select = [this](NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_sw3_end_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_sw3_end_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };

    field_sw4_start_.on_select = [this](NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_sw4_start_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_sw4_start_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };

    field_sw4_end_.on_select = [this](NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_sw4_end_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_sw4_end_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };

    field_sw3_exc0_.on_select = [this](NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_sw3_exc0_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_sw3_exc0_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };

    field_sw3_exc1_.on_select = [this](NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_sw3_exc1_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_sw3_exc1_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };

    field_sw3_exc2_.on_select = [this](NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_sw3_exc2_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_sw3_exc2_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };

    field_sw4_exc0_.on_select = [this](NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_sw4_exc0_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_sw4_exc0_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };

    field_sw4_exc1_.on_select = [this](NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_sw4_exc1_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_sw4_exc1_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };

    field_sw4_exc2_.on_select = [this](NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_sw4_exc2_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_sw4_exc2_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };

    field_sw3_exc3_.on_select = [this](NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_sw3_exc3_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_sw3_exc3_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };

    field_sw3_exc4_.on_select = [this](NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_sw3_exc4_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_sw3_exc4_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };

    field_sw4_exc3_.on_select = [this](NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_sw4_exc3_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_sw4_exc3_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };

    field_sw4_exc4_.on_select = [this](NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_sw4_exc4_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_sw4_exc4_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };
}

void SweepWindowGroup2View::focus() {
    check_sw3_enabled_.focus();
}

// ============================================================================
// DroneSweepView — Main sweep settings view with TabView
// ============================================================================

DroneSweepView::DroneSweepView(NavigationView& nav, const ScanConfig& config, DroneScanner* scanner_ptr) noexcept
    : ui::View()
    , nav_(nav)
    , scanner_ptr_(scanner_ptr)
    , original_config_(config)
    , view_group1_(nav_, Rect{0, TAB_BAR_H, screen_width, screen_height - TAB_BAR_H})
    , view_group2_(nav_, Rect{0, TAB_BAR_H, screen_width, screen_height - TAB_BAR_H})
    , tab_view_({
        {"Win 1-2", Color::white(), &view_group1_},
        {"Win 3-4", Color::white(), &view_group2_}
    }) {
    view_group2_.hidden(true);

    add_children({
        &tab_view_,
        &view_group1_,
        &view_group2_,
        &labels_exc_radius_,
        &field_exc_radius_,
        &button_defaults_,
        &button_save_,
    });

    tab_view_.set_selected(0);

    view_group1_.field_sw1_start_.set_value(static_cast<int32_t>(config.sweep_start_freq / 1000000ULL));
    view_group1_.field_sw1_end_.set_value(static_cast<int32_t>(config.sweep_end_freq / 1000000ULL));
    view_group1_.field_sw1_step_.set_value(static_cast<int32_t>(config.sweep_step_freq / 1000ULL));

    view_group1_.check_sw2_enabled_.set_value(config.sweep2_enabled);
    view_group1_.field_sw2_start_.set_value(static_cast<int32_t>(config.sweep2_start_freq / 1000000ULL));
    view_group1_.field_sw2_end_.set_value(static_cast<int32_t>(config.sweep2_end_freq / 1000000ULL));
    view_group1_.field_sw2_step_.set_value(static_cast<int32_t>(config.sweep2_step_freq / 1000ULL));

    view_group2_.check_sw3_enabled_.set_value(config.sweep3_enabled);
    view_group2_.field_sw3_start_.set_value(static_cast<int32_t>(config.sweep3_start_freq / 1000000ULL));
    view_group2_.field_sw3_end_.set_value(static_cast<int32_t>(config.sweep3_end_freq / 1000000ULL));
    view_group2_.field_sw3_step_.set_value(static_cast<int32_t>(config.sweep3_step_freq / 1000ULL));

    view_group2_.check_sw4_enabled_.set_value(config.sweep4_enabled);
    view_group2_.field_sw4_start_.set_value(static_cast<int32_t>(config.sweep4_start_freq / 1000000ULL));
    view_group2_.field_sw4_end_.set_value(static_cast<int32_t>(config.sweep4_end_freq / 1000000ULL));
    view_group2_.field_sw4_step_.set_value(static_cast<int32_t>(config.sweep4_step_freq / 1000ULL));

    ui::NumberField* exc1_fields[] = {
        &view_group1_.field_sw1_exc0_, &view_group1_.field_sw1_exc1_,
        &view_group1_.field_sw1_exc2_, &view_group1_.field_sw1_exc3_,
        &view_group1_.field_sw1_exc4_};
    ui::NumberField* exc2_fields[] = {
        &view_group1_.field_sw2_exc0_, &view_group1_.field_sw2_exc1_,
        &view_group1_.field_sw2_exc2_, &view_group1_.field_sw2_exc3_,
        &view_group1_.field_sw2_exc4_};
    ui::NumberField* exc3_fields[] = {
        &view_group2_.field_sw3_exc0_, &view_group2_.field_sw3_exc1_,
        &view_group2_.field_sw3_exc2_, &view_group2_.field_sw3_exc3_,
        &view_group2_.field_sw3_exc4_};
    ui::NumberField* exc4_fields[] = {
        &view_group2_.field_sw4_exc0_, &view_group2_.field_sw4_exc1_,
        &view_group2_.field_sw4_exc2_, &view_group2_.field_sw4_exc3_,
        &view_group2_.field_sw4_exc4_};

    for (uint8_t i = 0; i < EXCEPTIONS_PER_WINDOW; ++i) {
        exc1_fields[i]->set_value(static_cast<int32_t>(config.sweep_exceptions[0][i] / 1000000ULL));
        exc2_fields[i]->set_value(static_cast<int32_t>(config.sweep_exceptions[1][i] / 1000000ULL));
        exc3_fields[i]->set_value(static_cast<int32_t>(config.sweep_exceptions[2][i] / 1000000ULL));
        exc4_fields[i]->set_value(static_cast<int32_t>(config.sweep_exceptions[3][i] / 1000000ULL));
    }

    field_exc_radius_.set_value(static_cast<int32_t>(config.exception_radius_mhz));

    button_save_.on_select = [this](ui::Button&) {
        save_settings();
        nav_.pop();
    };

    button_defaults_.on_select = [this](ui::Button&) {
        apply_defaults();
    };
}

DroneSweepView::~DroneSweepView() noexcept {
}

void DroneSweepView::focus() {
    if (tab_view_.selected() == 0) {
        view_group1_.focus();
    } else {
        view_group2_.focus();
    }
}

void DroneSweepView::save_settings() noexcept {
    FreqHz sw1_start = static_cast<FreqHz>(view_group1_.field_sw1_start_.value()) * 1000000ULL;
    FreqHz sw1_end = static_cast<FreqHz>(view_group1_.field_sw1_end_.value()) * 1000000ULL;
    FreqHz sw1_step = static_cast<FreqHz>(view_group1_.field_sw1_step_.value()) * 1000ULL;

    bool sw2_enabled = view_group1_.check_sw2_enabled_.value();
    FreqHz sw2_start = static_cast<FreqHz>(view_group1_.field_sw2_start_.value()) * 1000000ULL;
    FreqHz sw2_end = static_cast<FreqHz>(view_group1_.field_sw2_end_.value()) * 1000000ULL;
    FreqHz sw2_step = static_cast<FreqHz>(view_group1_.field_sw2_step_.value()) * 1000ULL;

    bool sw3_enabled = view_group2_.check_sw3_enabled_.value();
    FreqHz sw3_start = static_cast<FreqHz>(view_group2_.field_sw3_start_.value()) * 1000000ULL;
    FreqHz sw3_end = static_cast<FreqHz>(view_group2_.field_sw3_end_.value()) * 1000000ULL;
    FreqHz sw3_step = static_cast<FreqHz>(view_group2_.field_sw3_step_.value()) * 1000ULL;

    bool sw4_enabled = view_group2_.check_sw4_enabled_.value();
    FreqHz sw4_start = static_cast<FreqHz>(view_group2_.field_sw4_start_.value()) * 1000000ULL;
    FreqHz sw4_end = static_cast<FreqHz>(view_group2_.field_sw4_end_.value()) * 1000000ULL;
    FreqHz sw4_step = static_cast<FreqHz>(view_group2_.field_sw4_step_.value()) * 1000ULL;

    FreqHz exc[4][EXCEPTIONS_PER_WINDOW]{};
    ui::NumberField* exc1_fields[] = {
        &view_group1_.field_sw1_exc0_, &view_group1_.field_sw1_exc1_,
        &view_group1_.field_sw1_exc2_, &view_group1_.field_sw1_exc3_,
        &view_group1_.field_sw1_exc4_};
    ui::NumberField* exc2_fields[] = {
        &view_group1_.field_sw2_exc0_, &view_group1_.field_sw2_exc1_,
        &view_group1_.field_sw2_exc2_, &view_group1_.field_sw2_exc3_,
        &view_group1_.field_sw2_exc4_};
    ui::NumberField* exc3_fields[] = {
        &view_group2_.field_sw3_exc0_, &view_group2_.field_sw3_exc1_,
        &view_group2_.field_sw3_exc2_, &view_group2_.field_sw3_exc3_,
        &view_group2_.field_sw3_exc4_};
    ui::NumberField* exc4_fields[] = {
        &view_group2_.field_sw4_exc0_, &view_group2_.field_sw4_exc1_,
        &view_group2_.field_sw4_exc2_, &view_group2_.field_sw4_exc3_,
        &view_group2_.field_sw4_exc4_};

    for (uint8_t i = 0; i < EXCEPTIONS_PER_WINDOW; ++i) {
        exc[0][i] = static_cast<FreqHz>(exc1_fields[i]->value()) * 1000000ULL;
        exc[1][i] = static_cast<FreqHz>(exc2_fields[i]->value()) * 1000000ULL;
        exc[2][i] = static_cast<FreqHz>(exc3_fields[i]->value()) * 1000000ULL;
        exc[3][i] = static_cast<FreqHz>(exc4_fields[i]->value()) * 1000000ULL;
    }

    const uint8_t exc_radius = static_cast<uint8_t>(field_exc_radius_.value());

    if (sw1_start < HARDWARE_MIN_FREQ_HZ) sw1_start = HARDWARE_MIN_FREQ_HZ;
    if (sw1_end > HARDWARE_MAX_FREQ_HZ) sw1_end = HARDWARE_MAX_FREQ_HZ;
    if (sw1_start >= sw1_end) sw1_end = sw1_start + 20000000;
    if (sw2_enabled) {
        if (sw2_start < HARDWARE_MIN_FREQ_HZ) sw2_start = HARDWARE_MIN_FREQ_HZ;
        if (sw2_end > HARDWARE_MAX_FREQ_HZ) sw2_end = HARDWARE_MAX_FREQ_HZ;
        if (sw2_start >= sw2_end) sw2_end = sw2_start + 20000000;
    }
    if (sw3_enabled) {
        if (sw3_start < HARDWARE_MIN_FREQ_HZ) sw3_start = HARDWARE_MIN_FREQ_HZ;
        if (sw3_end > HARDWARE_MAX_FREQ_HZ) sw3_end = HARDWARE_MAX_FREQ_HZ;
        if (sw3_start >= sw3_end) sw3_end = sw3_start + 20000000;
    }
    if (sw4_enabled) {
        if (sw4_start < HARDWARE_MIN_FREQ_HZ) sw4_start = HARDWARE_MIN_FREQ_HZ;
        if (sw4_end > HARDWARE_MAX_FREQ_HZ) sw4_end = HARDWARE_MAX_FREQ_HZ;
        if (sw4_start >= sw4_end) sw4_end = sw4_start + 20000000;
    }

    if (scanner_ptr_ != nullptr) {
        ScanConfig updated_config = original_config_;
        updated_config.sweep_start_freq = sw1_start;
        updated_config.sweep_end_freq = sw1_end;
        updated_config.sweep_step_freq = sw1_step;
        updated_config.sweep2_start_freq = sw2_start;
        updated_config.sweep2_end_freq = sw2_end;
        updated_config.sweep2_step_freq = sw2_step;
        updated_config.sweep2_enabled = sw2_enabled;
        updated_config.sweep3_start_freq = sw3_start;
        updated_config.sweep3_end_freq = sw3_end;
        updated_config.sweep3_step_freq = sw3_step;
        updated_config.sweep3_enabled = sw3_enabled;
        updated_config.sweep4_start_freq = sw4_start;
        updated_config.sweep4_end_freq = sw4_end;
        updated_config.sweep4_step_freq = sw4_step;
        updated_config.sweep4_enabled = sw4_enabled;
        for (uint8_t w = 0; w < 4; ++w) {
            for (uint8_t i = 0; i < EXCEPTIONS_PER_WINDOW; ++i) {
                updated_config.sweep_exceptions[w][i] = exc[w][i];
            }
        }
        updated_config.exception_radius_mhz = exc_radius;
        (void)scanner_ptr_->set_config(updated_config);
    }

    SettingsStruct current;
    (void)SettingsFileManager::load(current);

    current.sweep_start_freq = sw1_start;
    current.sweep_end_freq = sw1_end;
    current.sweep_step_freq = sw1_step;
    current.sweep2_start_freq = sw2_start;
    current.sweep2_end_freq = sw2_end;
    current.sweep2_step_freq = sw2_step;
    current.sweep2_enabled = sw2_enabled;
    current.sweep3_start_freq = sw3_start;
    current.sweep3_end_freq = sw3_end;
    current.sweep3_step_freq = sw3_step;
    current.sweep3_enabled = sw3_enabled;
    current.sweep4_start_freq = sw4_start;
    current.sweep4_end_freq = sw4_end;
    current.sweep4_step_freq = sw4_step;
    current.sweep4_enabled = sw4_enabled;
    for (uint8_t w = 0; w < 4; ++w) {
        for (uint8_t i = 0; i < EXCEPTIONS_PER_WINDOW; ++i) {
            current.sweep_exceptions[w][i] = exc[w][i];
        }
    }
    current.exception_radius_mhz = exc_radius;

    (void)SettingsFileManager::save(scanner_ptr_, current);
}

void DroneSweepView::apply_defaults() noexcept {
    SettingsStruct defaults;

    view_group1_.field_sw1_start_.set_value(static_cast<int32_t>(defaults.sweep_start_freq / 1000000ULL));
    view_group1_.field_sw1_end_.set_value(static_cast<int32_t>(defaults.sweep_end_freq / 1000000ULL));
    view_group1_.field_sw1_step_.set_value(static_cast<int32_t>(defaults.sweep_step_freq / 1000ULL));

    view_group1_.check_sw2_enabled_.set_value(defaults.sweep2_enabled);
    view_group1_.field_sw2_start_.set_value(static_cast<int32_t>(defaults.sweep2_start_freq / 1000000ULL));
    view_group1_.field_sw2_end_.set_value(static_cast<int32_t>(defaults.sweep2_end_freq / 1000000ULL));
    view_group1_.field_sw2_step_.set_value(static_cast<int32_t>(defaults.sweep2_step_freq / 1000ULL));

    view_group2_.check_sw3_enabled_.set_value(defaults.sweep3_enabled);
    view_group2_.field_sw3_start_.set_value(static_cast<int32_t>(defaults.sweep3_start_freq / 1000000ULL));
    view_group2_.field_sw3_end_.set_value(static_cast<int32_t>(defaults.sweep3_end_freq / 1000000ULL));
    view_group2_.field_sw3_step_.set_value(static_cast<int32_t>(defaults.sweep3_step_freq / 1000ULL));

    view_group2_.check_sw4_enabled_.set_value(defaults.sweep4_enabled);
    view_group2_.field_sw4_start_.set_value(static_cast<int32_t>(defaults.sweep4_start_freq / 1000000ULL));
    view_group2_.field_sw4_end_.set_value(static_cast<int32_t>(defaults.sweep4_end_freq / 1000000ULL));
    view_group2_.field_sw4_step_.set_value(static_cast<int32_t>(defaults.sweep4_step_freq / 1000ULL));

    ui::NumberField* all_exc[] = {
        &view_group1_.field_sw1_exc0_, &view_group1_.field_sw1_exc1_,
        &view_group1_.field_sw1_exc2_, &view_group1_.field_sw1_exc3_,
        &view_group1_.field_sw1_exc4_,
        &view_group1_.field_sw2_exc0_, &view_group1_.field_sw2_exc1_,
        &view_group1_.field_sw2_exc2_, &view_group1_.field_sw2_exc3_,
        &view_group1_.field_sw2_exc4_,
        &view_group2_.field_sw3_exc0_, &view_group2_.field_sw3_exc1_,
        &view_group2_.field_sw3_exc2_, &view_group2_.field_sw3_exc3_,
        &view_group2_.field_sw3_exc4_,
        &view_group2_.field_sw4_exc0_, &view_group2_.field_sw4_exc1_,
        &view_group2_.field_sw4_exc2_, &view_group2_.field_sw4_exc3_,
        &view_group2_.field_sw4_exc4_,
    };
    for (auto* f : all_exc) {
        f->set_value(0);
    }

    field_exc_radius_.set_value(static_cast<int32_t>(DEFAULT_EXCEPTION_RADIUS_MHZ));
}

} // namespace drone_analyzer