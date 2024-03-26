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

#include "ui_spectrum_painter_image.hpp"
#include "cpld_update.hpp"
#include "bmp.hpp"
#include "baseband_api.hpp"

#include "ui_fileman.hpp"
#include "io_file.hpp"
#include "file.hpp"
#include "portapack_persistent_memory.hpp"
#include "file_path.hpp"

namespace ui::external_app::spainter {

SpectrumInputImageView::SpectrumInputImageView(NavigationView& nav) {
    hidden(true);

    add_children({&button_load_image});

    button_load_image.on_select = [this, &nav](Button&) {
        auto open_view = nav.push<FileLoadView>(".bmp");

        ensure_directory(spectrum_dir);
        open_view->push_dir(spectrum_dir);

        open_view->on_changed = [this](std::filesystem::path new_file_path) {
            this->file = new_file_path.string();
            painted = false;
            this->set_dirty();
            this->on_input_available();
        };
    };
}

SpectrumInputImageView::~SpectrumInputImageView() {
}

void SpectrumInputImageView::focus() {
    button_load_image.focus();
}

bool SpectrumInputImageView::drawBMP_scaled(const ui::Rect r, const std::string file) {
    File bmpimage;
    size_t file_pos = 0;
    uint16_t pointer = 0;
    int16_t px = 0, py, zoom_factor = 0;
    bmp_header_t bmp_header;
    char buffer[257];
    ui::Color line_buffer[240];

    auto result = bmpimage.open(file);
    if (result.is_valid())
        return false;

    bmpimage.seek(file_pos);
    auto read_size = bmpimage.read(&bmp_header, sizeof(bmp_header));
    if (!((bmp_header.signature == 0x4D42) &&                               // "BM" Signature
          (bmp_header.planes == 1) &&                                       // Seems always to be 1
          (bmp_header.compression == 0 || bmp_header.compression == 3))) {  // No compression
        return false;
    }

    switch (bmp_header.bpp) {
        case 16:
            file_pos = 0x36;
            memset(buffer, 0, 16);
            bmpimage.read(buffer, 16);
            if (buffer[1] == 0x7C)
                type = 3;  // A1R5G5B5
            else
                type = 0;  // R5G6B5
            break;
        case 24:
            type = 1;
            break;
        case 32:
        default:
            type = 2;
            break;
    }

    width = bmp_header.width;
    height = bmp_header.height;

    data_start = file_pos = bmp_header.image_data;

    while (r.width() < (width >> zoom_factor) || r.height() < (height >> zoom_factor)) {
        zoom_factor++;
    }

    py = height + 16;

    while (1) {
        while (px < width) {
            bmpimage.seek(file_pos);
            memset(buffer, 0, 257);
            read_size = bmpimage.read(buffer, 256);
            if (read_size.is_error())
                return false;  // Read error

            pointer = 0;
            while (pointer < 256) {
                if (pointer + 4 > 256)
                    break;
                switch (type) {
                    case 0:  // R5G6B5
                        if ((((1 << zoom_factor) - 1) & px) == 0x00)
                            line_buffer[px >> zoom_factor] = ui::Color(((uint16_t)buffer[pointer] & 0x1F) | ((uint16_t)buffer[pointer] & 0xE0) << 1 | ((uint16_t)buffer[pointer + 1] & 0x7F) << 9);

                        pointer += 2;
                        file_pos += 2;
                        break;

                    case 3:  // A1R5G5B5
                        if ((((1 << zoom_factor) - 1) & px) == 0x00)
                            line_buffer[px >> zoom_factor] = ui::Color((uint16_t)buffer[pointer] | ((uint16_t)buffer[pointer + 1] << 8));

                        pointer += 2;
                        file_pos += 2;
                        break;

                    case 1:  // 24
                    default:
                        if ((((1 << zoom_factor) - 1) & px) == 0x00)
                            line_buffer[px >> zoom_factor] = ui::Color(buffer[pointer + 2], buffer[pointer + 1], buffer[pointer]);
                        pointer += 3;
                        file_pos += 3;
                        break;

                    case 2:  // 32
                        if ((((1 << zoom_factor) - 1) & px) == 0x00)
                            line_buffer[px >> zoom_factor] = ui::Color(buffer[pointer + 2], buffer[pointer + 1], buffer[pointer]);
                        pointer += 4;
                        file_pos += 4;
                        break;
                }

                px++;
                if (px >= width) {
                    break;
                }
            }

            if (read_size.value() != 256)
                break;
        }

        if ((((1 << zoom_factor) - 1) & py) == 0x00)
            portapack::display.render_line({r.left(), r.top() + (py >> zoom_factor)}, px >> zoom_factor, line_buffer);

        px = 0;
        py--;

        if (read_size.value() < 256 || py < 0)
            break;
    }

    return true;
}

uint16_t SpectrumInputImageView::get_width() {
    return this->width;
}

uint16_t SpectrumInputImageView::get_height() {
    return this->height;
}

std::vector<uint8_t> SpectrumInputImageView::get_line(uint16_t y) {
    File bmpimage;
    bmpimage.open(this->file);

    // seek to line
    uint32_t line_size = width * (type == 2 ? 4 : (type == 1 ? 3 : 2));
    uint32_t line_offset = y * line_size;
    bmpimage.seek(data_start + line_offset);

    // allocate memory and read
    auto buffer = new uint8_t[line_size];
    auto bytes_read = bmpimage.read(buffer, line_size);

    // greyscale
    auto grey_buffer = new uint8_t[width];
    int pointer = 0;
    for (uint16_t px = 0; px < width; px++) {
        ui::Color color;
        switch (type) {
            case 0:  // R5G6B5
                pointer = px * 2;
                color = ui::Color(((uint16_t)buffer[pointer] & 0x1F) | ((uint16_t)buffer[pointer] & 0xE0) << 1 | ((uint16_t)buffer[pointer + 1] & 0x7F) << 9);
                break;

            case 3:  // A1R5G5B5
                pointer = px * 2;
                color = ui::Color((uint16_t)buffer[pointer] | ((uint16_t)buffer[pointer + 1] << 8));
                break;

            case 1:  // 24
            default:
                pointer = px * 3;
                color = ui::Color(buffer[pointer + 2], buffer[pointer + 1], buffer[pointer]);
                break;

            case 2:  // 32
                pointer = px * 4;
                color = ui::Color(buffer[pointer + 2], buffer[pointer + 1], buffer[pointer]);
                break;
        }

        grey_buffer[px] = color.to_greyscale();
    }

    delete[] buffer;

    std::vector<uint8_t> values(width);
    for (int i = 0; i < width; i++) {
        values[i] = grey_buffer[i];
    }

    delete[] grey_buffer;

    return values;
}

void SpectrumInputImageView::paint(Painter& painter) {
    painter.fill_rectangle(
        {{0, 40}, {240, 204}},
        style().background);

    if (!painted) {
        // This is very slow for big pictures. Do only once.
        this->drawBMP_scaled({{0, 40}, {240, 160}}, this->file);
        painted = true;
    }
}

}  // namespace ui::external_app::spainter
