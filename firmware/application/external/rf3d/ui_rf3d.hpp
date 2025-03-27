#ifndef __UI_RF3D_HPP__
#define __UI_RF3D_HPP__

#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "message.hpp"
#include "baseband_api.hpp"
#include "portapack.hpp"

namespace ui::external_app::rf3d {

class RF3DView : public View {
public:
    RF3DView(NavigationView& nav);
    void on_show() override;
    std::string title() const override { return "RF3D"; }
    void focus() override { dummy.focus(); }
    void paint(Painter& painter) override;
    void frame_sync();

private:
    NavigationView& nav_;
    Button dummy{{240, 0, 0, 0}, ""};
    bool initialized{false};
    std::vector<std::vector<uint8_t>> spectrum_data;
    uint32_t sampling_rate{24000};
    double angle{0.0};
    bool running{false};
    
    void start();
    void stop();
    void update_spectrum();
    void render_3d_waterfall(Painter& painter);

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->frame_sync();
        }
    };
};

} // namespace ui::external_app::rf3d

#endif /*__UI_3D_RF_HPP__*/