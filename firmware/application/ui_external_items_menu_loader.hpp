
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
#include <cstring>
#include "file.hpp"

#define EXT_APP_EXPECTED_CHECKSUM 0x00000000

namespace ui {
template <size_t Width, size_t Height>
class DynamicBitmap {
   public:
    static constexpr size_t buffer_size = Width * Height / (sizeof(uint8_t) * 8);

    // Main constructor
    DynamicBitmap(const uint8_t data[buffer_size])
        : _bitmap{{Width, Height}, _buffer.data()} {
        std::memcpy(_buffer.data(), data, buffer_size);
    }

    DynamicBitmap(const DynamicBitmap& other)
        : _buffer(other._buffer),
          _bitmap{{Width, Height}, _buffer.data()} {}

    DynamicBitmap(DynamicBitmap&& other) noexcept
        : _buffer(std::move(other._buffer)),
          _bitmap{{Width, Height}, _buffer.data()} {}

    DynamicBitmap& operator=(const DynamicBitmap& other) = delete;
    DynamicBitmap& operator=(DynamicBitmap&& other) noexcept = delete;

    // Destructor (Default is fine, no heap memory to free manually)
    ~DynamicBitmap() = default;

    const Bitmap* bitmap() const { return &_bitmap; }

   private:
    // Order matters: _buffer must be declared before _bitmap
    // so it is initialized first and its .data() pointer is valid.
    std::array<uint8_t, buffer_size> _buffer{};
    Bitmap _bitmap{};
};

class ExternalItemsMenuLoader {
   public:
    struct GridItemEx : GridItem {
        int32_t desired_position;
    };

    static std::vector<GridItemEx> load_external_items(app_location_t, NavigationView&);
    ExternalItemsMenuLoader() = delete;
    static bool run_external_app(ui::NavigationView&, std::filesystem::path);
    static bool run_standalone_app(ui::NavigationView&, std::filesystem::path);
    static bool run_module_app(ui::NavigationView&, uint8_t*, size_t);
    static void load_all_external_items_callback(std::function<void(AppInfoConsole&)> callback, bool module_included = false);
    static void unload_external_items();

   private:
    static std::vector<std::unique_ptr<DynamicBitmap<16, 16>>> bitmaps;
};

}  // namespace ui

#endif /*__EXTERNAL_ITEMS_MENU_LOADER_H__*/
