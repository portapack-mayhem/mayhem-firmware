#ifndef __UI_TETRA_RX_H__
#define __UI_TETRA_RX_H__

#include "ui.hpp"
#include "ui_language.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "log_file.hpp"
#include "utility.hpp"
#include "message.hpp"
#include <array>
#include <string>

using namespace ui;

namespace ui::external_app::tetra_rx {

class TetraChannelDecoder {
   public:
    struct Result {
        enum class Type { None,
                          Sync,
                          SyncFull,
                          Dnb };
        Type type{Type::None};

        bool is_ok{false};
        uint8_t sync_errors{0};
        bool inverted{false};

        uint16_t crc{0};
        int cost{0};

        uint8_t timeslot{0xff};
        uint8_t frame_number{0xff};
        uint16_t mcc{0xffff};
        uint16_t mnc{0xffff};
        uint8_t bcc{0xff};

        uint8_t pdu_type{0xff};
        uint16_t la{0xffff};
        uint8_t encryption{0xff};

        uint8_t cmce_type{0xff};
        uint16_t call_id{0xffff};
        uint32_t calling_ssi{0};

        std::array<uint8_t, 34> payload{};
    };

    std::string get_pdu_name(uint8_t pdu_type) {
        switch (pdu_type) {
            case 0:
                return "MAC-RESOURCE";
            case 1:
                return "MAC-FRAG";
            case 2:
                return "SYSINFO/BCAST";
            case 3:
                return "MAC-U-SIGNAL";
            default:
                return "UNK_" + std::to_string(pdu_type);
        }
    }

    Result decode_burst(const TetraBurstMessage& message);
    Result decode_dnb(const TetraDnbMessage& message);

   private:
    static constexpr size_t BLK1_OFFSET_BITS = 94;
    static constexpr size_t BLK1_LEN_BITS = 120;
    static constexpr size_t TMO_BLK2_OFFSET_BITS = 282;
    static constexpr size_t BLK2_LEN_BITS = 216;
    static constexpr size_t SB1_TYPE1_BITS = 60;
    static constexpr size_t SB1_TYPE2_BITS = 80;
    static constexpr size_t SB1_INTERLEAVE_A = 11;
    static constexpr size_t SB2_TYPE1_BITS = 124;
    static constexpr size_t SB2_TYPE2_BITS = 144;
    static constexpr size_t SB2_INTERLEAVE_A = 101;

    static constexpr size_t SCH_F_LEN_BITS = 432;
    static constexpr size_t SCH_F_TYPE1_BITS = 268;
    static constexpr size_t SCH_F_TYPE2_BITS = 288;
    static constexpr size_t SCH_F_INTERLEAVE_A = 103;

    uint16_t last_mcc{0};
    uint16_t last_mnc{0};
    uint8_t last_bcc{0};
    bool network_synced{false};

    // Viterbi buffer
    uint8_t trace_buffer[9216];

    template <size_t OutBytes>
    bool decode_block(const std::array<uint8_t, 63>& burst,
                      size_t offset,
                      size_t len_t5,
                      size_t type1_bits,
                      size_t type2_bits,
                      size_t interleave_a,
                      std::array<uint8_t, OutBytes>& out,
                      uint16_t& crc,
                      int& cost,
                      uint32_t scramb_init);

    void parse_sync_pdu(Result& result);
    void parse_mac_pdu(Result& result);
    uint32_t read_bits(const uint8_t* bytes, size_t bit, size_t count) const;

    template <size_t N>
    uint8_t get_bit(const std::array<uint8_t, N>& arr, size_t bit_idx) const {
        return (arr[bit_idx / 8] >> (7 - (bit_idx % 8))) & 0x01;
    }

    template <size_t N>
    void set_bit(std::array<uint8_t, N>& arr, size_t bit_idx, uint8_t val) const {
        if (val)
            arr[bit_idx / 8] |= (1 << (7 - (bit_idx % 8)));
        else
            arr[bit_idx / 8] &= ~(1 << (7 - (bit_idx % 8)));
    }
};

class TetraRxView : public View {
   public:
    TetraRxView(NavigationView& nav);
    ~TetraRxView();
    void focus() override;
    std::string title() const override { return "Tetra RX"; };

   private:
    NavigationView& nav_;
    RxRadioState radio_state_{};
    app_settings::SettingsManager settings_{"rx_tetra", app_settings::Mode::RX};

    uint32_t packet_count{0};
    uint32_t valid_count{0};
    uint32_t h_valid_count{0};
    uint32_t dnb_count{0};

    RxFrequencyField field_frequency{{UI_POS_X(0), UI_POS_Y(0)}, nav_};
    RFAmpField field_rf_amp{{UI_POS_X(13), UI_POS_Y(0)}};
    LNAGainField field_lna{{UI_POS_X(15), UI_POS_Y(0)}};
    VGAGainField field_vga{{UI_POS_X(18), UI_POS_Y(0)}};
    RSSI rssi{{UI_POS_X_RIGHT(9), UI_POS_Y(0), UI_POS_WIDTH(9), 4}};
    Channel channel{{UI_POS_X_RIGHT(9), UI_POS_Y(0) + 5, UI_POS_WIDTH(9), 4}};

    Text text_mcc{{UI_POS_X(0), UI_POS_Y(1), UI_POS_WIDTH(15), UI_POS_HEIGHT(1)}, "MCC: ---"};
    Text text_mnc{{UI_POS_X(15), UI_POS_Y(1), UI_POS_WIDTH(15), UI_POS_HEIGHT(1)}, "MNC: ---"};
    Text text_ts{{UI_POS_X(0), UI_POS_Y(2), UI_POS_WIDTH(15), UI_POS_HEIGHT(1)}, "TS:  -"};       // time slot
    Text text_fn{{UI_POS_X(15), UI_POS_Y(2), UI_POS_WIDTH(15), UI_POS_HEIGHT(1)}, "FN:  -"};      // frame number
    Text text_bcc{{UI_POS_X(0), UI_POS_Y(3), UI_POS_WIDTH(15), UI_POS_HEIGHT(1)}, "BCC: -"};      // Base Station Color Code
    Text text_enc{{UI_POS_X(15), UI_POS_Y(3), UI_POS_WIDTH(15), UI_POS_HEIGHT(1)}, "ENC: -"};     // encoded or not
    Text text_la{{UI_POS_X(0), UI_POS_Y(4), UI_POS_WIDTH(15), UI_POS_HEIGHT(1)}, "LA:  ----"};    // Location Area
    Text text_pdu{{UI_POS_X(15), UI_POS_Y(4), UI_POS_WIDTH(15), UI_POS_HEIGHT(1)}, "PDU: ----"};  // pdu content type

    Text text_debug{{UI_POS_X(0), UI_POS_Y(5), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "Syn: 0, V: 0, H: 0, E:0"};  // sync, valid, h_valid, errors corrected in top part
    Console console{{UI_POS_X(0), UI_POS_Y(6) + 8, UI_POS_MAXWIDTH, screen_height - (UI_POS_Y(7) + 8)}};

    TetraChannelDecoder decoder{};

    void on_data_tetra(const TetraBurstMessage&);
    void on_data_dnb(const TetraDnbMessage&);

    uint8_t rate_limiter{0};  // limiter, to keep the device responsive
    void on_timer();

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->on_timer();
        }};

    MessageHandlerRegistration message_handler_packet{
        Message::ID::TetraBsch,
        [this](Message* const p) {
            this->on_data_tetra(*static_cast<const TetraBurstMessage*>(p));
        }};

    MessageHandlerRegistration message_handler_dnb{
        Message::ID::TetraDnb,
        [this](Message* const p) {
            this->on_data_dnb(*static_cast<const TetraDnbMessage*>(p));
        }};
};

}  // namespace ui::external_app::tetra_rx

#endif