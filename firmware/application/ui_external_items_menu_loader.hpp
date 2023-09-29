
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

namespace ui {

class ExternalItemsMenuLoader {
   public:
    static std::vector<GridItem> load_external_items(app_location_t, NavigationView&);
    ExternalItemsMenuLoader() = delete;

   private:
    static std::vector<Bitmap*> bitmaps;

    static void run_external_app(ui::NavigationView&, const TCHAR*);
};

}  // namespace ui

#endif /*__EXTERNAL_ITEMS_MENU_LOADER_H__*/
