/* When to hand the baseband the picked station, and when to let go. The MSC
 * takes ~40% of the frame and acquisition cannot run under it, so a pick is a
 * standing request: engage after `streak` locked frames with r4 >= `gate`
 * outside a holdoff; release after `release_frames` below `release_r4`, then
 * hold off `holdoff` frames. Pure logic, so it can be tested off target. */
#ifndef __DAB_ENGAGE_H__
#define __DAB_ENGAGE_H__

#include <cstdint>

namespace dab {

class Engagement {
   public:
    struct Config {
        uint16_t gate;
        uint16_t streak;
        uint16_t release_r4;
        uint16_t release_frames;
        uint16_t holdoff;
    };

    enum class Action : uint8_t {
        None,
        Engage,
        Release,
    };

    explicit constexpr Engagement(const Config& config)
        : config_{config} {}

    /* A change while engaged is immediate, with a clean loss count. */
    Action want() {
        wanted_ = true;
        streak_ = 0;
        lost_ = 0;
        return engaged_ ? Action::Engage : Action::None;
    }

    /* Stop, retune, scan: the caller releases whatever the baseband holds. */
    void drop() {
        wanted_ = false;
        engaged_ = false;
        streak_ = 0;
        lost_ = 0;
    }

    Action on_frame(bool locked, uint16_t r4, bool paused) {
        if (holdoff_ > 0)
            --holdoff_;
        if (!wanted_ || paused)
            return Action::None;

        if (!engaged_) {
            if (locked && r4 >= config_.gate && holdoff_ == 0) {
                if (streak_ < 0xFFFF)
                    ++streak_;
            } else {
                streak_ = 0;
            }
            if (streak_ > config_.streak) {
                engaged_ = true;
                streak_ = 0;
                lost_ = 0;
                return Action::Engage;
            }
            return Action::None;
        }

        if (r4 < config_.release_r4) {
            if (lost_ < 0xFFFF)
                ++lost_;
        } else {
            lost_ = 0;
        }
        if (lost_ > config_.release_frames) {
            engaged_ = false;
            lost_ = 0;
            streak_ = 0;
            holdoff_ = config_.holdoff;
            ++releases_;
            return Action::Release;
        }
        return Action::None;
    }

    bool wanted() const { return wanted_; }
    bool engaged() const { return engaged_; }
    uint16_t holdoff() const { return holdoff_; }
    uint32_t releases() const { return releases_; }

   private:
    Config config_;
    bool wanted_{false};
    bool engaged_{false};
    uint16_t streak_{0};
    uint16_t lost_{0};
    uint16_t holdoff_{0};
    uint32_t releases_{0};
};

/* r4 is ~20 on a dead constellation, ~700 on a live one. Engaging inside
 * acquisition starts a limit cycle, hence the holdoff. */
constexpr Engagement::Config kEngageDefaults{30, 8, 120, 60, 64};

} /* namespace dab */

#endif /*__DAB_ENGAGE_H__*/
