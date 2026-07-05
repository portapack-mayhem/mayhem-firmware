#include "pattern_types.hpp"

namespace drone_analyzer {

SignalPattern::SignalPattern() noexcept
    : name{}
    , waveform{}
    , features{}
    , match_threshold(DEFAULT_PATTERN_SIMILARITY_THRESHOLD)
    , flags(SignalPattern::Flags::ENABLED)
    , created_time(0)
    , center_freq(0)
    , range_width(0) {
}

} // namespace drone_analyzer
