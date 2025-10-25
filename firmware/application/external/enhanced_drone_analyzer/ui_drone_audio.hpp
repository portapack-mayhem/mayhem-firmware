// ui_drone_audio.hpp - Audio Manager for Enhanced Drone Analyzer
// Provides audio alert functionality for drone detection

#ifndef __UI_DRONE_AUDIO_HPP__
#define __UI_DRONE_AUDIO_HPP__

#include <cstdint>

enum class ThreatLevel;
class AudioManager {
public:
    AudioManager() = default;
    virtual ~AudioManager() = default;

    virtual bool is_audio_enabled() const { return audio_enabled_; }
    virtual void toggle_audio() { audio_enabled_ = !audio_enabled_; }
    virtual void play_detection_beep(ThreatLevel threat) { (void)threat; /* Stub */ }

    virtual uint16_t get_alert_frequency() const { return 800; }
    virtual void set_alert_frequency(uint16_t freq) { (void)freq; }
    virtual uint32_t get_alert_duration_ms() const { return 500; }
    virtual void set_alert_duration_ms(uint32_t duration) { (void)duration; }

private:
    bool audio_enabled_ = true;
};

struct DroneAudioSettings {
    bool audio_enabled = true;
    uint16_t test_threat_level = 2; // ThreatLevel::HIGH as uint
};

#endif // __UI_DRONE_AUDIO_HPP__
