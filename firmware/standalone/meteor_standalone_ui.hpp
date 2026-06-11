/*
 * Copyright (C) 2026
 *
 * Minimal LCD text/background for Meteor .ppmp apps (uses standalone API draw_bitmap).
 */
#ifndef FIRMWARE_STANDALONE_METEOR_STANDALONE_UI_HPP
#define FIRMWARE_STANDALONE_METEOR_STANDALONE_UI_HPP

#include "standalone_app.hpp"

#include <cstdint>

namespace meteor_standalone_ui {

/** CPLD key indices (see ui::KeyEvent in firmware/common/ui.hpp). */
constexpr uint8_t kKeyLeft = 1;
constexpr uint8_t kKeySelect = 4;
constexpr uint8_t kKeyBack = 6;

void bind(const standalone_application_api_t& api);
void clear_screen(uint16_t w, uint16_t h, uint16_t color = 0);
void draw_text(int x, int y, const char* text, uint16_t fg = 0xFFFF, uint16_t bg = 0);

/** True when user wants to leave the standalone app (return to Receive menu). */
bool key_exit_to_menu(uint8_t key);

/**
 * Standard Meteor offline app layout: status + two hint lines + footer navigation.
 */
void draw_app_ui(
    uint16_t screen_w,
    uint16_t screen_h,
    const char* status,
    const char* hint1,
    const char* hint2,
    const char* action_hint);

}  // namespace meteor_standalone_ui

#endif
