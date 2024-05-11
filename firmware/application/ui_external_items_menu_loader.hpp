
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

#ifndef __EXTERNAL_ITEMS_MENU_LOADER_H__
#define __EXTERNAL_ITEMS_MENU_LOADER_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"
#include "standalone_app.hpp"

#include "file.hpp"

#define EXT_APP_EXPECTED_CHECKSUM 0x00000000

namespace ui {

template <size_t Width, size_t Height>
class DynamicBitmap {
   public:
    static constexpr size_t buffer_size = Width * Height / (sizeof(uint8_t) * 8);  // one bit per pixel

    DynamicBitmap(const uint8_t data[buffer_size])
        : _buffer(buffer_size, 0),
          _bitmap{new Bitmap{{Width, Height}, &_buffer[0]}} {
        memcpy(&_buffer[0], data, buffer_size);
    }

    const Bitmap* bitmap() { return _bitmap.get(); }

   private:
    // Allocating both members so the class is movable without invalidation.
    std::vector<uint8_t> _buffer;
    std::unique_ptr<Bitmap> _bitmap;
};

class ExternalItemsMenuLoader {
   public:
    static std::vector<GridItem> load_external_items(app_location_t, NavigationView&);
    ExternalItemsMenuLoader() = delete;
    static bool run_external_app(ui::NavigationView&, std::filesystem::path);
    static bool run_standalone_app(ui::NavigationView&, std::filesystem::path);
    static void load_all_external_items_callback(std::function<void(AppInfoConsole&)> callback);

   private:
    static std::vector<DynamicBitmap<16, 16>> bitmaps;
};

}  // namespace ui

#endif /*__EXTERNAL_ITEMS_MENU_LOADER_H__*/
