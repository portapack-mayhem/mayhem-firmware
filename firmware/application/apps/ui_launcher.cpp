/*
 * Copyright (C) 2023 Kyle Reed
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

// https://github.com/tinito/Bootloader/blob/master/loader/apploader.c

#include "debug.hpp"

#include "ui_fileman.hpp"
#include "ui_launcher.hpp"

#include "file.hpp"
#include "string_format.hpp"

#include <string>
#include <vector>

using namespace portapack;
namespace fs = std::filesystem;

namespace ui {

static const fs::path ppa_ext = u".PPA";
static const fs::path app_folder = u"APPS";

AppLauncherView::AppLauncherView(NavigationView& nav)
   : nav_{nav} {

    add_children({
      &button_open,
      &text_result,
    });

    button_open.on_select = [this](Button&) {
      ensure_directory(app_folder);
      auto open_view = nav_.push<FileLoadView>(".PPA");
      open_view->push_dir(app_folder);
      open_view->on_changed = [this](fs::path app_path) {
          run_application(app_path);
      };
    };
}

AppLauncherView::AppLauncherView(NavigationView& nav, const fs::path&)
    : AppLauncherView(nav) {
}

void AppLauncherView::run_application(const fs::path& path) {

  // TODO: need to build an app loader that knows how to find the ELF sections and load
  // each section into block. Then jump into the .text section.
  // Will need to remap addresses and such or figure out how a relocatable binary works.
  //DEBUG_LOG("Loading " + path.string());

  uint32_t program = 539641712;
  auto entry = reinterpret_cast<app_start_fn>(
    reinterpret_cast<void*>(&program));

  /*
  // Ugh, failing even on 2kb files. Going to need an ELF reader.
  auto result = File::read_file(path);
  if (!result) {
    text_result.set("Failed to load " + path.string());
    return;
  }

  auto app = *std::move(result);
  auto entry = reinterpret_cast<app_start_fn>(
    reinterpret_cast<void*>(&app[0xCA])); // What's the offset?
  */

  //DEBUG_LOG(to_string_dec_uint(*(uint32_t*)entry));

  auto exit_code = entry();
  //text_result.set(to_string_dec_int(exit_code));
}

}  // namespace ui
