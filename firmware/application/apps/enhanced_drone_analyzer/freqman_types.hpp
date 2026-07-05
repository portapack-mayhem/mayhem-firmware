#ifndef FREQMAN_TYPES_HPP
#define FREQMAN_TYPES_HPP

#include <cstdint>
#include <cstddef>

namespace drone_analyzer {

enum class freqman_type : uint8_t {
    Single,    
    Range,     
    HamRadio,  
    Repeater,  
    Raw,       
    Unknown,
};

using freqman_index_t = uint8_t;
constexpr freqman_index_t freqman_invalid_index = static_cast<freqman_index_t>(-1);

}

#endif
