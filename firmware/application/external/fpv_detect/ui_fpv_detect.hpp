// Author: berkeozkir (Berke Özkır)

#ifndef _UI_FPV_DETECT
#define _UI_FPV_DETECT

#include <array>
#include <cstdint>

#include "analog_audio_app.hpp"
#include "app_settings.hpp"
#include "audio.hpp"
#include "baseband_api.hpp"
#include "radio_state.hpp"
#include "receiver_model.hpp"
#include "string_format.hpp"
#include "ui.hpp"
#include "ui_receiver.hpp"
#include "ui_spectrum.hpp"

namespace ui::external_app::fpv_detect {

static constexpr uint8_t FPV_NUM_BANDS = 5;
static constexpr uint8_t FPV_CHANNELS_PER_BAND = 8;
static constexpr uint8_t FPV_TOTAL_CHANNELS = FPV_NUM_BANDS * FPV_CHANNELS_PER_BAND;
static constexpr uint8_t FPV_AUTO_SCAN_MODE = FPV_NUM_BANDS;

static constexpr int64_t fpv_frequencies[FPV_NUM_BANDS][FPV_CHANNELS_PER_BAND] = {
    /* Band A */ {5865000000LL, 5845000000LL, 5825000000LL, 5805000000LL, 5785000000LL, 5765000000LL, 5745000000LL, 5725000000LL},
    /* Band B */ {5733000000LL, 5752000000LL, 5771000000LL, 5790000000LL, 5809000000LL, 5828000000LL, 5847000000LL, 5866000000LL},
    /* Band E */ {5705000000LL, 5685000000LL, 5665000000LL, 5645000000LL, 5885000000LL, 5905000000LL, 5925000000LL, 5945000000LL},
    /* Band F */ {5740000000LL, 5760000000LL, 5780000000LL, 5800000000LL, 5820000000LL, 5840000000LL, 5860000000LL, 5880000000LL},
    /* Band R */ {5658000000LL, 5695000000LL, 5732000000LL, 5769000000LL, 5806000000LL, 5843000000LL, 5880000000LL, 5917000000LL},
};

static constexpr char band_labels[] = {'A', 'B', 'E', 'F', 'R'};

#define FPV_RX_BW 750000

class FpvDetectView : public View {
   public:
    explicit FpvDetectView(NavigationView& nav);
    ~FpvDetectView();

    void focus() override;
    std::string title() const override { return "FPV DETECT"; };

   private:
    enum class DetectState : uint8_t {
        Scanning,
        Candidate,
        Locked,
    };

    struct ChannelMemory {
        int16_t last_db = -120;
        int16_t peak_db = -120;
        uint8_t hits = 0;
        uint8_t confidence = 0;
    };

    NavigationView& nav_;
    RxRadioState radio_state_{};

    static int32_t clamp_value(int32_t value, int32_t low, int32_t high);
    static int32_t map_value(int32_t value, int32_t from_low, int32_t from_high, int32_t to_low, int32_t to_high);

    size_t change_mode();
    void on_statistics_update(const ChannelStatistics& statistics);
    void on_timer();

    void retune_to(uint8_t band, uint8_t ch);
    void step_scan();
    void reset_detector(bool retune_current);

    bool is_possible_analog_carrier(const ChannelStatistics& statistics) const;
    void enter_candidate(const ChannelStatistics& statistics);
    void evaluate_candidate_sample(const ChannelStatistics& statistics);
    void enter_lock();
    void update_lock(const ChannelStatistics& statistics);

    uint8_t current_channel_index() const;
    uint8_t channel_index(uint8_t band, uint8_t ch) const;
    uint8_t compute_candidate_confidence() const;
    int16_t detect_threshold_db() const;
    int16_t unlock_threshold_db() const;
    int16_t candidate_average_db() const;
    int16_t neighbor_best_db() const;

    void request_event_beep(uint32_t hz, uint32_t duration_ms);
    void update_freq_display();
    void update_state_badge();
    void update_confidence_text();
    void update_status_text();
    void update_detail_text();
    void update_stats_text(const ChannelStatistics& statistics);
    void update_visual_state();

    DetectState detect_state_ = DetectState::Scanning;

    uint8_t scan_band = 0;
    uint8_t scan_ch = 0;
    uint8_t band_mode = FPV_AUTO_SCAN_MODE;

    uint8_t candidate_band_ = 0;
    uint8_t candidate_ch_ = 0;
    uint8_t verify_samples_ = 0;
    uint8_t verify_hits_ = 0;
    uint8_t verify_misses_ = 0;
    int32_t verify_sum_db_ = 0;
    int16_t verify_peak_db_ = -120;
    uint8_t candidate_confidence_ = 0;
    uint8_t lock_hold_ = 0;
    /** During Candidate: 0=center, 1=left neighbor, 2=right neighbor. Used to
     * re-sample adjacent channels so neighbor_margin uses current measurements. */
    uint8_t neighbor_phase_ = 0;
    bool neighbors_fresh_ = false;

    uint8_t scan_dwell_frames_ = 0;
    uint32_t frame_counter_ = 0;
    uint32_t last_beep_frame_ = 0;

    int16_t last_max_db_ = -127;
    uint8_t last_min_rssi_ = 0;
    uint8_t last_avg_rssi_ = 0;
    uint8_t last_max_rssi_ = 0;

    std::array<ChannelMemory, FPV_TOTAL_CHANNELS> channel_memory_{};

    rf::Frequency freq_ = {fpv_frequencies[0][0]};
    int32_t detect_threshold_db_ = -38;
    audio::Rate audio_sampling_rate = audio::Rate::Hz_48000;

    app_settings::SettingsManager settings_{
        "rx_fpv",
        app_settings::Mode::RX,
        {
            {"detect_threshold"sv, &detect_threshold_db_},
        }};

    Labels labels{
        {{UI_POS_X(0), UI_POS_Y(0)}, "LNA:   VGA:   AMP:  ", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X_RIGHT(6), UI_POS_Y(0)}, "VOL:  ", Theme::getInstance()->fg_light->foreground},
    };

    LNAGainField field_lna{{UI_POS_X(4), UI_POS_Y(0)}};
    VGAGainField field_vga{{UI_POS_X(11), UI_POS_Y(0)}};
    RFAmpField field_rf_amp{{UI_POS_X(18), UI_POS_Y(0)}};
    AudioVolumeField field_volume{{UI_POS_X_RIGHT(2), UI_POS_Y(0)}};

    OptionsField field_band{
        {UI_POS_X(0), UI_POS_Y(1)},
        9,
        {
            {"Band A  ", 0},
            {"Band B  ", 1},
            {"Band E  ", 2},
            {"Band F  ", 3},
            {"Band R  ", 4},
            {"AutoScan", FPV_AUTO_SCAN_MODE},
        }};

    Text text_freq{{UI_POS_X_RIGHT(20), UI_POS_Y(1), UI_POS_WIDTH(20), UI_POS_DEFAULT_HEIGHT}, ""};

    Text text_state{{UI_POS_X(0), UI_POS_Y(2), UI_POS_WIDTH(12), UI_POS_DEFAULT_HEIGHT}, "SCANNING"};
    Text text_confidence{{UI_POS_X(12), UI_POS_Y(2), UI_POS_WIDTH(8), UI_POS_DEFAULT_HEIGHT}, "Conf 0%"};
    Text text_detect_label{{UI_POS_X_RIGHT(10), UI_POS_Y(2), UI_POS_WIDTH(5), UI_POS_DEFAULT_HEIGHT}, "Thr>"};

    NumberField field_detect_threshold{
        {UI_POS_X_RIGHT(5), UI_POS_Y(2)},
        4,
        {-100, 20},
        1,
        ' ',
    };

    Text freq_stats_rssi{{UI_POS_X(0), UI_POS_Y(3), UI_POS_WIDTH(15), UI_POS_DEFAULT_HEIGHT}, "RSSI 0/0/0"};
    Text freq_stats_db{{UI_POS_X_RIGHT(14), UI_POS_Y(3), UI_POS_WIDTH(14), UI_POS_DEFAULT_HEIGHT}, "PWR -120 dB"};

    Text text_status{{UI_POS_X(0), UI_POS_Y(4), UI_POS_WIDTH(30), UI_POS_DEFAULT_HEIGHT}, "SCANNING FOR ANALOG FPV"};
    Text text_detail{{UI_POS_X(0), UI_POS_Y(5), UI_POS_WIDTH(30), UI_POS_DEFAULT_HEIGHT}, "Waiting for FPV-like carrier"};

    RSSIGraph rssi_graph{{UI_POS_X(0), UI_POS_Y(6), UI_POS_WIDTH_REMAINING(5), UI_POS_HEIGHT_REMAINING(7)}};
    RSSI rssi{{UI_POS_X_RIGHT(5), UI_POS_Y(6), UI_POS_WIDTH(5), UI_POS_HEIGHT_REMAINING(7)}};

    MessageHandlerRegistration message_handler_stats{
        Message::ID::ChannelStatistics,
        [this](const Message* const p) {
            this->on_statistics_update(static_cast<const ChannelStatisticsMessage*>(p)->statistics);
        }};

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->on_timer();
        }};
};

}  // namespace ui::external_app::fpv_detect

#endif
