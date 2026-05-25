/*
 * Copyright (C) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef METEOR_LRPT_DECODE_APP_HPP
#define METEOR_LRPT_DECODE_APP_HPP

#include <cstdint>

struct standalone_application_api_t;

void meteor_decode_initialize(const standalone_application_api_t& api);
void meteor_decode_on_event(const uint32_t& events);
void meteor_decode_shutdown();
void meteor_decode_paint();
bool meteor_decode_on_key(uint8_t key);

#endif
