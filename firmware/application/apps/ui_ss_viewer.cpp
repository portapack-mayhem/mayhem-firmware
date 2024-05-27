/*
 * Copyright (C) 2023 Kyle Reed
 * Copyright (C) 2023 Mark Thompson
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

#include "ui_ss_viewer.hpp"

using namespace portapack;
namespace fs = std::filesystem;

namespace ui {

const std::filesystem::path splash_dot_bmp{u"/splash.bmp"};

ScreenshotViewer::ScreenshotViewer(
    NavigationView& nav,
    const std::filesystem::path& path)
    : nav_{nav},
      path_{path} {
    set_focusable(true);
}

bool ScreenshotViewer::on_key(KeyEvent) {
    nav_.pop();
    return true;
}

void ScreenshotViewer::paint(Painter& painter) {
    File file{};

    painter.fill_rectangle({0, 0, screen_width, screen_height}, Color::black());

    auto show_invalid = [&]() {
        painter.draw_string({10, 160}, *Theme::getInstance()->bg_darkest, "Not a valid screenshot.");
    };

    auto error = file.open(path_);
    if (error) {
        painter.draw_string({10, 160}, *Theme::getInstance()->bg_darkest, error->what());
        return;
    }

    // Screenshots from PNGWriter are all this size.
    if (file.size() != 232383) {
        show_invalid();
        return;
    }

    constexpr size_t read_chunk = 80;  // NB: must be a factor of pixel_width.
    constexpr size_t buffer_size = sizeof(ColorRGB888) * read_chunk;
    uint8_t buffer[buffer_size];
    std::array<Color, screen_width> pixel_data;

    // Seek past all the headers.
    file.seek(43);

    for (auto line = 0u; line < screen_height; ++line) {
        // Seek past the per-line header.
        file.seek(file.tell() + 6);

        // Per comment in PNGWriter, read in chunks of 80.
        // NB: Reading in one large chunk caused corruption so there's
        // likely a bug lurking in the SD Card/FatFs layer.
        for (auto offset = 0u; offset < screen_width; offset += read_chunk) {
            auto read = file.read(buffer, buffer_size);

            if (!read || *read != buffer_size) {
                show_invalid();
                return;
            }

            auto c8 = (ColorRGB888*)buffer;
            for (auto i = 0u; i < read_chunk; ++i) {
                pixel_data[i + offset] = Color(c8->r, c8->g, c8->b);
                ++c8;
            }
        }

        display.draw_pixels({0, (int)line, screen_width, 1}, pixel_data);
    }
}

SplashViewer::SplashViewer(
    NavigationView& nav,
    const std::filesystem::path& path)
    : nav_{nav},
      path_{path} {
    valid_image = false;
    set_focusable(true);
}

bool SplashViewer::on_key(const KeyEvent key) {
    if (valid_image && key == KeyEvent::Right) {
        delete_file(splash_dot_bmp);
        copy_file(path_, splash_dot_bmp);
    }

    nav_.pop();
    return true;
}

void SplashViewer::paint(Painter& painter) {
    painter.fill_rectangle({0, 0, screen_width, screen_height}, Color::black());

    if (!portapack::display.drawBMP2({0, 0}, path_)) {
        painter.draw_string({10, 160}, *Theme::getInstance()->bg_darkest, "Not a valid splash image.");
        return;
    }

    // Show option to set splash screen if it's not already the splash screen
    if (!path_iequal(path_, splash_dot_bmp)) {
        painter.draw_string({0, 0}, *Theme::getInstance()->bg_darkest, "*RIGHT BUTTON UPDATES SPLASH*");
        valid_image = true;
    }
}

}  // namespace ui
