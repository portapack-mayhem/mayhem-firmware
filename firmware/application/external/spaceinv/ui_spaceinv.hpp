/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#ifndef __UI_SPACEINV_H__
#define __UI_SPACEINV_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "event_m0.hpp"
#include "message.hpp"
#include "irq_controls.hpp"
#include "random.hpp"
#include "lpc43xx_cpp.hpp"
#include "ui_widget.hpp"
#include "app_settings.hpp"

namespace ui::external_app::spaceinv {

using Callback = void (*)(void);

class Ticker {
   public:
    Ticker() = default;
    void attach(Callback func, double delay_sec);
    void detach();
};

void check_game_timer();
void game_timer_check();

class SpaceInvadersView : public View {
   public:
    SpaceInvadersView(NavigationView& nav);
    ~SpaceInvadersView();  // Destructor will trigger settings save
    void on_show() override;

    std::string title() const override { return "Space Invaders"; }

    void focus() override { dummy.focus(); }
    void paint(Painter& painter) override;
    void frame_sync();
    bool on_encoder(const EncoderEvent event) override;
    bool on_key(KeyEvent key) override;

    uint32_t highScore = 0;
    bool easy_mode = false;

   private:
    NavigationView& nav_;

    Button button_difficulty{
        {70, 275, 100, 20},
        "Mode: HARD"};

    app_settings::SettingsManager settings_{
        "spaceinv",
        app_settings::Mode::NO_RF,
        {{"highscore"sv, &highScore},
         {"easy_mode"sv, &easy_mode}}};

    Button dummy{
        {240, 0, 0, 0},
        ""};

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->frame_sync();
        }};
};

}  // namespace ui::external_app::spaceinv

#endif /* __UI_SPACEINV_H__ */