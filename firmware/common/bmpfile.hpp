#ifndef __BMPFILE__H
#define __BMPFILE__H

#include <cstring>
#include <string>

#include "file.hpp"
#include "bmp.hpp"
#include "ui.hpp"

class BMPFile {
   public:
    ~BMPFile();
    bool open(const std::filesystem::path& file, bool readonly);
    bool create(const std::filesystem::path& file, uint32_t x, uint32_t y);
    void close();
    bool seek(uint32_t x, uint32_t y);
    bool expand_y(uint32_t new_y);
    bool expand_y_delta(uint32_t delta_y);
    uint32_t getbpr() { return byte_per_row; };

    bool read_next_px(ui::Color& px);
    bool write_next_px(ui::Color& px);

   private:
    bool advance_curr_px(uint32_t num);
    bool is_opened = false;
    bool is_read_ony = true;
    File bmpimage{};
    size_t file_pos = 0;
    bmp_header_t bmp_header{};
    uint8_t type = 0;
    uint8_t byte_per_px = 1;
    uint32_t byte_per_row = 0;

    uint32_t currx = 0;
    uint32_t curry = 0;

    int8_t zoom_factor = 1;  // + zoom in, - zoom out.
};

#endif