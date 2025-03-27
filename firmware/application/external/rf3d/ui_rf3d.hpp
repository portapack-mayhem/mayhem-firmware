#ifndef __UI_RF3D_HPP__
#define __UI_RF3D_HPP__

#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "message.hpp"
#include "baseband_api.hpp"
#include "portapack.hpp"
#include "ui_record_view.hpp"

namespace ui::external_app::rf3d {

class RF3DView : public View {
public:
    RF3DView(NavigationView& nav);
    ~RF3DView();

    RF3DView(const RF3DView&) = delete;
    RF3DView& operator=(const RF3DView&) = delete;

    void focus() override;
    std::string title() const override { return "RF3D"; }

    void paint(Painter& painter) override;
    void frame_sync();

private:
    static constexpr ui::Dim header_height = 3 * 16;
    static constexpr int SCREEN_WIDTH = 240;
    static constexpr int SCREEN_HEIGHT = 320;
    static constexpr int RENDER_HEIGHT = 280;
    static constexpr int HALF_WIDTH = SCREEN_WIDTH / 2;
    static constexpr int HALF_HEIGHT = RENDER_HEIGHT / 2;
    static constexpr int MAX_RENDER_DEPTH = 12;

    NavigationView& nav_;
    bool initialized{false};
    std::vector<std::vector<uint8_t>> spectrum_data;
    uint32_t sampling_rate{3072000};
    double angle{0.0};
    bool running{false};

    RSSI rssi{{21 * 8, 0, 6 * 8, 4}};
    Channel channel{{21 * 8, 5, 6 * 8, 4}};
    Audio audio{{21 * 8, 10, 6 * 8, 4}};
    FrequencyField field_frequency{{5 * 8, 0 * 16}};
    LNAGainField field_lna{{15 * 8, 0 * 16}};
    VGAGainField field_vga{{18 * 8, 0 * 16}};
    OptionsField options_modulation{
        {0 * 8, 0 * 16},
        4,
        {
            {"AM", toUType(ReceiverModel::Mode::AMAudio)},
            {"NFM", toUType(ReceiverModel::Mode::NarrowbandFMAudio)},
            {"WFM", toUType(ReceiverModel::Mode::WidebandFMAudio)},
            {"SPEC", toUType(ReceiverModel::Mode::SpectrumAnalysis)}
        }
    };
    AudioVolumeField field_volume{{28 * 8, 0 * 16}};
    Text text_ctcss{{16 * 8, 1 * 16, 14 * 8, 1 * 16}, ""};
    RecordView record_view{
        {0 * 8, 2 * 16, 30 * 8, 1 * 16},
        u"AUD",
        u"AUDIO",
        RecordView::FileType::WAV,
        4096,
        4
    };
    const Rect options_view_rect{0 * 8, 1 * 16, 30 * 8, 1 * 16};
    std::unique_ptr<Widget> options_widget{};
    Button dummy{{240, 0, 0, 0}, ""};

    void start();
    void stop();
    void update_spectrum(const ChannelSpectrum& spectrum);
    void render_3d_waterfall(Painter& painter);
    void on_modulation_changed(ReceiverModel::Mode modulation);
    void on_show_options_frequency();
    void on_show_options_rf_gain();
    void on_show_options_modulation();
    void on_frequency_step_changed(rf::Frequency f);
    void on_reference_ppm_correction_changed(int32_t v);
    void remove_options_widget();
    void set_options_widget(std::unique_ptr<Widget> new_widget);
    void update_modulation(ReceiverModel::Mode modulation);
    void handle_coded_squelch(uint32_t value);

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) { this->frame_sync(); }
    };
    MessageHandlerRegistration message_handler_channel_spectrum{
        Message::ID::ChannelSpectrumConfig,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const ChannelSpectrumConfigMessage*>(p);
            this->channel_fifo = message.fifo;
        }
    };
    MessageHandlerRegistration message_handler_coded_squelch{
        Message::ID::CodedSquelch,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const CodedSquelchMessage*>(p);
            this->handle_coded_squelch(message.value);
        }
    };
    ChannelSpectrumFIFO* channel_fifo{nullptr};
};

} // namespace ui::external_app::rf3d

#endif