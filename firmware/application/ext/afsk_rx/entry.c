/*
 * Copyright (C) 2023 Bernd Herzog
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "external_app.h"

__attribute__((section(".external_app.app_afsk_rx"), used)) extern void app_afsk_rx(void*, void** p);
// extern uint32_t __flash_start__;

__attribute__((section(".external_app.app_afsk_rx.ExternalAppEntry_afsk_rx"))) void ExternalAppEntry_afsk_rx(void* nav, void** p) {
    app_afsk_rx(nav, p);
}

__attribute__((section(".external_app.app_afsk_rx.application_information"))) application_information_t _application_information_afsk_rx = {
    0,
    ExternalAppEntry_afsk_rx,
    /*&__flash_start__ */ (void*)0x10086000};
