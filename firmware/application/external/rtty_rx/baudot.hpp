#ifndef BAUDOT_HPP
#define BAUDOT_HPP

#include <cstdint>
#include <string>

namespace ui::external_app::rtty_rx {

class BaudotCoder {
   public:
    enum ShiftState { LETTERS,
                      FIGURES };

    BaudotCoder()
        : shiftState(LETTERS) {}
    char decode(uint8_t baudotCode);
    void encode(const std::string& src, uint8_t* dest, uint16_t* dest_length, uint16_t dest_max_size);
    void set_usos(bool enable) { usos_enabled = enable; }  // shift == set to letters too

   private:
    ShiftState shiftState;
    bool usos_enabled = true;
    char get_char_mapping(bool is_figures, uint8_t index) const;
};

}  // namespace ui::external_app::rtty_rx

#endif  // BAUDOT_HPP