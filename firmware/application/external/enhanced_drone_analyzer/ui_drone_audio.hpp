// ui_drone_audio.hpp
// Audio feedback and alert system for drone detection

#ifndef __UI_DRONE_AUDIO_HPP__
#define __UI_DRONE_AUDIO_HPP__

#include "baseband_api.hpp"
#include "audio.hpp"
#include "ui_drone_spectrum_scanner.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

class DroneAudioAlert {
private:
    bool audio_enabled_;
    uint32_t last_beep_time_;

public:
    DroneAudioAlert();
    ~DroneAudioAlert();

    // Audio control
    void enable_audio(bool enable);
    bool is_audio_enabled() const { return audio_enabled_; }

    // Alert functions
    void play_detection_beep(ThreatLevel level);
    void play_sos_signal();
    void stop_audio();

    // Static beep request (audio manager integration)
    static void request_audio_beep(uint16_t frequency, uint32_t duration_ms);

    // Frequency mapping for threat levels
    static uint16_t get_beep_frequency(ThreatLevel level);
};

class AudioManager {
public:
    static void request_beep(uint16_t frequency, uint32_t sample_rate, uint32_t duration_ms);
    static void stop_all_audio();
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_DRONE_AUDIO_HPP__
