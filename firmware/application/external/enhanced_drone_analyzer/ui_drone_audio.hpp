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

// DEPRECATED: Old DroneAudioController - replaced by AudioManager
// Keeping for backward compatibility during transition
class DroneAudioController {
public:
    DroneAudioController();
    ~DroneAudioController();
    void enable_audio() { audio_enabled_ = true; }
    void disable_audio() { audio_enabled_ = false; }
    bool is_audio_enabled() const { return audio_enabled_; }
    void toggle_audio() { audio_enabled_ = !audio_enabled_; }
    void play_detection_beep(ThreatLevel level) { /* DEPRECATED - no-op */ }
    void play_sos_signal() { /* DEPRECATED - no-op */ }
    void stop_audio() { audio::output::stop(); }
    uint16_t get_beep_frequency(ThreatLevel level) const { return 1000; }
    uint32_t get_last_beep_time() const { return last_beep_time_; }
    static void request_audio_beep(uint16_t frequency, uint32_t duration_ms) { /* DEPRECATED */ }

    DroneAudioController(const DroneAudioController&) = delete;
    DroneAudioController& operator=(const DroneAudioController&) = delete;

private:
    bool audio_enabled_;
    uint32_t last_beep_time_;

    void initialize_audio_device() { /* DEPRECATED */ }
    void cleanup_audio_device() { audio::output::stop(); }
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_DRONE_AUDIO_HPP__
