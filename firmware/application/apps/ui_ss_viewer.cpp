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

#include "ui_ss_viewer.hpp"
#include "screenshot_reader.hpp"


using namespace portapack;
namespace fs = std::filesystem;

namespace ui {

ScreenshotViewer::ScreenshotViewer(
    NavigationView& nav,
    const std::filesystem::path& path)
    : nav_{nav},
    path_{path} {
}

bool ScreenshotViewer::on_key(KeyEvent) {
    nav_.pop();
    return true;
}

void ScreenshotViewer::paint(Painter& painter) {
    Style style_default{
        .font = ui::font::fixed_8x16,
        .background = ui::Color::black(),
        .foreground = ui::Color::white()};
    File file{};

    painter.fill_rectangle({0,0, 240, 320}, Color::black());

    auto error = file.open(path_);
    if (error) {
        painter.draw_string({160, 10}, style_default, "Cannot open file.");
        return;
    }

    constexpr uint32_t buffer_size = 64;
    uint8_t buffer[buffer_size];
    uint32_t line = 0;
    bool valid = true;

    ScreenshotReader reader;
    reader.on_line = [&](auto line_pixels) {
        std::array<Color, 240> colors;
        for (auto i = 0u; i < line_pixels.size(); ++i) {
            auto& c8 = line_pixels[i];
            colors[i] = Color(c8.r, c8.g, c8.b);
        }
        display.draw_pixels({0, line, 240, 1}, colors.data(), 240);
    };

    while (true) {
        auto read = file.read(buffer, sizeof(buffer[0]), buffer_size);
        if (read)
            valid = reader.parse(buffer, *read);

        if (*read < buffer_size || !valid)
            break;
    }

    if (!valid) {
        painter.draw_string({160, 10}, style_default, "Not a valid screenshot.");
        return;
    }
}

}  // namespace ui