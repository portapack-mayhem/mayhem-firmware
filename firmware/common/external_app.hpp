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

#ifndef __EXTERNAL_APPS_H__
#define __EXTERNAL_APPS_H__

#include "ch.h"
#include "ui_navigation.hpp"
#include "spi_image.hpp"
#include "standalone_app.hpp"

#define CURRENT_HEADER_VERSION 0x00000002
#define MIN_HEADER_VERSION_FOR_CHECKSUM 0x00000002

typedef void (*externalAppEntry_t)(ui::NavigationView& nav);

struct application_information_t {
    uint8_t* memory_location;
    externalAppEntry_t externalAppEntry;
    uint32_t header_version;
    uint32_t app_version;

    uint8_t app_name[16];
    uint8_t bitmap_data[32];
    uint32_t icon_color;
    app_location_t menu_location;

    portapack::spi_flash::image_tag_t m4_app_tag;
    uint32_t m4_app_offset;
};

#endif /*__EXTERNAL_APPS_H__*/
