#include "baudot.hpp"
#include <cctype>

namespace ui::external_app::rtty_rx {

constexpr uint8_t CODE_FIGS = 0x1B;
constexpr uint8_t CODE_LTRS = 0x1F;
constexpr uint8_t CODE_SPACE = 0x04;

char BaudotCoder::get_char_mapping(bool is_figures, uint8_t index) const {
    if (index >= 32) return 0;  // Safety check

    if (is_figures) {
        // ITA2 FIGURES Table
        const char figures[32] = {
            0, '3', '\n', '-', ' ', '\'', '8', '7',
            '\r', '$', '4', '\a', ',', '!', ':', '(',
            '5', '+', ')', '2', '#', '6', '0', '1',
            '9', '?', '&', 0, '.', '/', '=', 0};
        return figures[index];
    } else {
        // ITA2 LETTERS Table
        const char letters[32] = {
            0, 'E', '\n', 'A', ' ', 'S', 'I', 'U',
            '\r', 'D', 'R', 'J', 'N', 'F', 'C', 'K',
            'T', 'Z', 'L', 'W', 'H', 'Y', 'P', 'Q',
            'O', 'B', 'G', 0, 'M', 'X', 'V', 0};
        return letters[index];
    }
}

// --- DECODE ---
char BaudotCoder::decode(uint8_t baudotCode) {
    uint8_t code = baudotCode & 0x1F;
    if (code == CODE_FIGS) {
        shiftState = FIGURES;
        return 0;
    }
    if (code == CODE_LTRS) {
        shiftState = LETTERS;
        return 0;
    }
    if (usos_enabled && shiftState == FIGURES && code == CODE_SPACE) {
        shiftState = LETTERS;
        return ' ';
    }
    return get_char_mapping((shiftState == FIGURES), code);
}

void BaudotCoder::encode(const std::string& src, uint8_t* dest, uint16_t* dest_length, uint16_t dest_max_size) {
    uint16_t idx = 0;

    for (char c : src) {
        if (idx >= dest_max_size) break;

        char upper_c = std::toupper(c);
        uint8_t code = 0;
        bool found = false;
        bool target_is_figures = false;
        if (upper_c == ' ') {
            code = CODE_SPACE;
            found = true;
        } else {
            for (int i = 1; i < 32; i++) {
                if (get_char_mapping(false, i) == upper_c) {
                    code = i;
                    found = true;
                    target_is_figures = false;
                    break;
                }
            }
            if (!found) {
                for (int i = 1; i < 32; i++) {
                    if (get_char_mapping(true, i) == upper_c) {
                        code = i;
                        found = true;
                        target_is_figures = true;
                        break;
                    }
                }
            }
        }

        if (found) {
            if (target_is_figures && shiftState != FIGURES) {
                if (idx + 1 >= dest_max_size) break;
                dest[idx++] = CODE_FIGS;
                shiftState = FIGURES;
            } else if (!target_is_figures && shiftState != LETTERS && code != CODE_SPACE) {
                if (idx + 1 >= dest_max_size) break;
                dest[idx++] = CODE_LTRS;
                shiftState = LETTERS;
            }

            dest[idx++] = code;
        }
    }

    if (dest_length) {
        *dest_length = idx;
    }
}

}  // namespace ui::external_app::rtty_rx