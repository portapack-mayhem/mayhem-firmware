// ui_drone_audio.hpp
// Audio alerts and sound generation for Enhanced Drone Analyzer
// Module 3 of 4: Handles audio feedback for drone detections

#ifndef __UI_DRONE_AUDIO_HPP__
#define __UI_DRONE_AUDIO_HPP__

#include "audio.hpp"                    // Audio output control
#include "baseband_api.hpp"             // beep API
#include "ui_drone_types.hpp"           // ThreatLevel for tone mapping

#include <memory>

namespace ui::external_app::enhanced_drone_analyzer {

class DroneAudioController {
public:
    DroneAudioController();
    ~DroneAudioController();

    // Audio state management
    void enable_audio() { audio_enabled_ = true; }
    void disable_audio() { audio_enabled_ = false; }
    bool is_audio_enabled() const { return audio_enabled_; }
    void toggle_audio() { audio_enabled_ = !audio_enabled_; }

    // Detection beep generation
    void play_detection_beep(ThreatLevel level);
    void play_sos_signal();
    void stop_audio();

    // Beep frequency mapping
    uint16_t get_beep_frequency(ThreatLevel level) const;

    // Audio status
    uint32_t get_last_beep_time() const { return last_beep_time_; }

    // Static beep method for external access
    static void request_audio_beep(uint16_t frequency, uint32_t duration_ms);

    // Prevent copying
    DroneAudioController(const DroneAudioController&) = delete;
    DroneAudioController& operator=(const DroneAudioController&) = delete;

private:
    // Audio state
    bool audio_enabled_;           // Audio alerts enabled/disabled
    uint32_t last_beep_time_;      // Last beep timestamp

    // Audio device initialization
    void initialize_audio_device();
    void cleanup_audio_device();
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_DRONE_AUDIO_HPP__
