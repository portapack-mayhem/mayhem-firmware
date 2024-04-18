
#include "bmpfile.hpp"

// fix height info
uint32_t BMPFile::get_real_height() {
    if (!is_opened) return 0;
    return bmp_header.height >= 0 ? (uint32_t)bmp_header.height : (uint32_t)(-1 * bmp_header.height);
}

// get bmp width
uint32_t BMPFile::get_width() {
    if (!is_opened) return 0;
    return bmp_header.width;
}

// get if the rows are bottom up (for most bmp), or up to bottom (negative height, we use it for write)
bool BMPFile::is_bottomup() {
    return (bmp_header.height >= 0);
}

BMPFile::~BMPFile() {
    close();
}

// closes file
void BMPFile::close() {
    is_opened = false;
    bmpimage.close();
}

// creates a new bmp file. for now, hardcoded to 3 byte colour depth
bool BMPFile::create(const std::filesystem::path& file, uint32_t x, uint32_t y) {
    is_opened = false;
    is_read_ony = true;
    bmpimage.close();  // if already open, close before open a new
    if (file_exists(file)) return false;
    auto result = bmpimage.open(file, false, true);
    if (!result.value().ok()) return false;
    file_pos = 0;
    byte_per_row = (x * 3 % 4 == 0) ? x * 3 : (x * 3 + (4 - ((x * 3) % 4)));  // with padding
    bmpimage.seek(file_pos);
    bmp_header.signature = 0x4D42;
    bmp_header.planes = 1;
    bmp_header.compression = 0;
    bmp_header.bpp = 24;  // 3 byte depth
    bmp_header.width = x;
    bmp_header.height = 0;  // for now, will expand
    bmp_header.image_data = 0x36;
    bmp_header.BIH_size = 0x28;
    bmp_header.h_res = 100;
    bmp_header.v_res = 100;
    byte_per_px = 3;
    type = 1;
    bmp_header.size = sizeof(bmp_header) + get_real_height() * byte_per_row;  // with padding! --will update later with expand
    bmp_header.data_size = bmp_header.size - sizeof(bmp_header_t);
    bmp_header.colors_count = 0;
    bmp_header.icolors_count = 0;

    bmpimage.write(&bmp_header, sizeof(bmp_header_t));
    file_pos = bmp_header.image_data;
    is_opened = true;
    is_read_ony = false;
    if (!expand_y(y)) return false;  // will fill with 0, and update header data
    seek(0, 0);
    return true;
}

// opens the file and parses header data. return true on success
bool BMPFile::open(const std::filesystem::path& file, bool readonly) {
    is_opened = false;
    is_read_ony = true;
    bmpimage.close();  // if already open, close before open a new

    auto result = bmpimage.open(file, readonly, false);
    if (!result.is_valid()) return false;
    file_pos = 0;
    bmpimage.seek(file_pos);
    auto read_size = bmpimage.read(&bmp_header, sizeof(bmp_header_t));
    if (!((bmp_header.signature == 0x4D42) &&                               // "BM" Signature
          (bmp_header.planes == 1) &&                                       // Seems always to be 1
          (bmp_header.compression == 0 || bmp_header.compression == 3))) {  // No compression
        return false;
    }
    char buffer[257];
    switch (bmp_header.bpp) {
        case 16:
            file_pos = 0x36;
            memset(buffer, 0, 16);
            bmpimage.read(buffer, 16);
            byte_per_px = 2;
            if (buffer[1] == 0x7C)
                type = 3;  // A1R5G5B5
            else
                type = 0;  // R5G6B5
            break;
        case 24:
            type = 1;
            byte_per_px = 3;
            break;
        case 32:
        default:
            type = 2;
            byte_per_px = 4;
            break;
    }
    byte_per_row = (bmp_header.width * byte_per_px % 4 == 0) ? bmp_header.width * byte_per_px : (bmp_header.width * byte_per_px + (4 - ((bmp_header.width * byte_per_px) % 4)));
    file_pos = bmp_header.image_data;
    is_opened = true;
    is_read_ony = readonly;
    currx = 0;
    curry = 0;
    return true;
}

// jumps to next pixel. false on the end
bool BMPFile::advance_curr_px(uint32_t num = 1) {
    if (curry >= get_real_height()) return false;
    uint32_t rowsToAdvance = (currx + num) / bmp_header.width;
    uint32_t nx = (currx + num) % bmp_header.width;
    uint32_t ny = curry + rowsToAdvance;
    if (ny >= get_real_height()) {
        return false;
    }
    seek(nx, ny);
    return true;
}

// reads next px, then advance the pos (and seek). return false on last px
bool BMPFile::read_next_px(ui::Color& px) {
    if (!is_opened) return false;
    uint8_t buffer[4];
    auto res = bmpimage.read(buffer, byte_per_px);
    if (res.is_error()) return false;
    switch (type) {
        case 0:  // R5G6B5
        case 3:  // A1R5G5B5
            if (!type)
                px = ui::Color((uint16_t)buffer[0] | ((uint16_t)buffer[1] << 8));
            else
                px = ui::Color(((uint16_t)buffer[0] & 0x1F) | ((uint16_t)buffer[0] & 0xE0) << 1 | ((uint16_t)buffer[1] & 0x7F) << 9);
            break;
        case 1:  // 24
        default:
            px = ui::Color(buffer[2], buffer[1], buffer[0]);

            break;
        case 2:  // 32
            px = ui::Color(buffer[2], buffer[1], buffer[0]);
            break;
    }
    return advance_curr_px();
}

// writes a color data to the current position, and advances 1 px. true on success, false on error
bool BMPFile::write_next_px(ui::Color& px) {
    if (!is_opened) return false;
    if (is_read_ony) return false;
    uint8_t buffer[4];
    switch (type) {
        case 0:  // R5G6B5
        case 3:  // A1R5G5B5
            if (!type) {
                buffer[0] = (px.r() << 3) | (px.g() >> 3);  // todo test in future
                buffer[1] = (px.g() << 5) | px.b();
            } else {
                buffer[0] = (1 << 7) | (px.r() << 2) | (px.g() >> 3);  // todo test in future
                buffer[1] = (px.g() << 5) | px.b();
            }
            break;
        case 1:  // 24
        default:
            buffer[2] = px.r();
            buffer[1] = px.g();
            buffer[0] = px.b();
            break;
        case 2:  // 32
            buffer[2] = px.r();
            buffer[1] = px.g();
            buffer[0] = px.b();
            buffer[3] = 255;
            break;
    }
    auto res = bmpimage.write(buffer, byte_per_px);
    if (res.is_error()) return false;
    advance_curr_px();
    return true;
}

// positions in the file to the given pixel. 0 based indexing
bool BMPFile::seek(uint32_t x, uint32_t y) {
    if (!is_opened) return false;
    if (x >= bmp_header.width) return false;
    if (y >= get_real_height()) return false;
    file_pos = bmp_header.image_data;  // nav to start pos.
    file_pos += y * byte_per_row;
    file_pos += x * byte_per_px;
    bmpimage.seek(file_pos);
    currx = x;
    curry = y;
    return true;
}

// expands the image with a delta (y)
bool BMPFile::expand_y_delta(uint32_t delta_y) {
    return expand_y(get_real_height() + delta_y);
}

// expands the image to a new y size. also seek's t it's begining
bool BMPFile::expand_y(uint32_t new_y) {
    if (!is_opened) return false;  // not yet opened
    uint32_t old_height = get_real_height();
    if (new_y < old_height) return true;  // already bigger
    if (is_read_ony) return false;        // can't expand
    uint32_t delta = (new_y - old_height) * byte_per_row;
    bmp_header.size += delta;
    bmp_header.data_size += delta;
    bmp_header.height = -1 * new_y;  //-1*, so no bottom-up structure needed. easier to expand.
    bmpimage.seek(0);
    bmpimage.write(&bmp_header, sizeof(bmp_header));  // overwrite header
    bmpimage.seek(bmp_header.size);                   // seek to new end to expand
    seek(0, curry + 1);                               // seek back to the original position
    return true;
}