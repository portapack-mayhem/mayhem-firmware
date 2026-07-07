#include "ui_tetra_rx.hpp"
#include "rtc_time.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"
#include "file_path.hpp"
#include "tetra_crc.hpp"
#include "tetra_descrambler.hpp"
#include "tetra_interleave.hpp"
#include "tetra_rcpc.hpp"
#include "tetra_viterbi.hpp"
#include <cstring>

using namespace portapack;
using namespace ui;

namespace ui::external_app::tetra_rx {

template <size_t OutBytes>
bool TetraChannelDecoder::decode_block(
    const std::array<uint8_t, 63>& burst,
    size_t offset,
    size_t len_t5,
    size_t type1_bits,
    size_t type2_bits,
    size_t interleave_a,
    std::array<uint8_t, OutBytes>& out,
    uint16_t& crc,
    int& cost,
    uint32_t scramb_init) {
    std::array<uint8_t, 64> t5{};
    std::array<uint8_t, 64> t3{};
    std::array<uint8_t, 1200> mother{};
    std::array<uint8_t, 64> type2{};

    out.fill(0);

    for (size_t i = 0; i < len_t5; i++) {
        set_bit(t5, i, get_bit(burst, offset + i));
    }

    BitVector t5_bits{t5.data(), len_t5};
    Descrambler::descramble(t5_bits, scramb_init);

    BitVector t3_bits{t3.data(), len_t5};
    Interleave::block_deinterleave(t5_bits, t3_bits, len_t5, interleave_a);

    RCPC::depuncture_2_3(t3_bits, mother.data(), len_t5);
    Viterbi viterbi;
    cost = viterbi.decode_cch(mother.data(), type2_bits, type2.data(), trace_buffer);
    crc = CRC::crc16_itut_bits(type2.data(), type1_bits + 16);
    for (size_t i = 0; i < type1_bits; i++) {
        set_bit(out, i, (type2[i >> 3] >> (7 - (i & 7))) & 1);
    }
    return crc == CRC::CRC_OK;
}

TetraChannelDecoder::Result TetraChannelDecoder::decode_dnb(const TetraDnbMessage& message) {
    Result result{};
    result.inverted = message.inverted;

    if (!network_synced)
        return result;

    std::array<uint8_t, 63> buffer{};
    std::copy(message.payload.begin(), message.payload.end(), buffer.begin());

    uint32_t tmo_dnb_seed = (((uint32_t)last_mcc << 20) | ((uint32_t)last_mnc << 6) | (uint32_t)last_bcc) << 2 | 3;
    // 1. DNB Full slot (SCH/F)
    bool ok = decode_block(buffer, 0, SCH_F_LEN_BITS, SCH_F_TYPE1_BITS, SCH_F_TYPE2_BITS,
                           SCH_F_INTERLEAVE_A, result.payload, result.crc, result.cost, tmo_dnb_seed);

    // 2. SCH/HD Block 1
    if (!ok) {
        ok = decode_block(buffer, 0, BLK2_LEN_BITS, SB2_TYPE1_BITS, SB2_TYPE2_BITS,
                          SB2_INTERLEAVE_A, result.payload, result.crc, result.cost, tmo_dnb_seed);
    }

    // 3. SCH/HD Block 2
    if (!ok) {
        ok = decode_block(buffer, 216, BLK2_LEN_BITS, SB2_TYPE1_BITS, SB2_TYPE2_BITS,
                          SB2_INTERLEAVE_A, result.payload, result.crc, result.cost, tmo_dnb_seed);
    }

    if (ok) {
        result.type = Result::Type::Dnb;
        result.is_ok = true;
        parse_mac_pdu(result);
    }

    return result;
}

TetraChannelDecoder::Result TetraChannelDecoder::decode_burst(const TetraBurstMessage& message) {
    Result result{};
    result.sync_errors = message.sync_errors;
    result.inverted = message.inverted;

    // 1. SCH/S
    bool ok = decode_block(
        message.payload, BLK1_OFFSET_BITS, BLK1_LEN_BITS,
        SB1_TYPE1_BITS, SB1_TYPE2_BITS, SB1_INTERLEAVE_A,
        result.payload, result.crc, result.cost,
        Descrambler::SCRAMB_INIT_BSCH);

    if (!ok) return result;

    result.type = Result::Type::Sync;
    result.is_ok = true;
    parse_sync_pdu(result);

    uint16_t current_mcc = result.mcc;
    uint16_t current_mnc = result.mnc;
    uint8_t current_bcc = result.bcc;
    uint8_t current_enc = result.encryption;

    // 2. SCH/H
    uint16_t h_crc = 0;
    int h_cost = 0;

    ok = decode_block(
        message.payload, TMO_BLK2_OFFSET_BITS, BLK2_LEN_BITS,
        SB2_TYPE1_BITS, SB2_TYPE2_BITS, SB2_INTERLEAVE_A,
        result.payload, h_crc, h_cost,
        Descrambler::SCRAMB_INIT_BSCH);

    if (ok) {
        result.type = Result::Type::SyncFull;
        result.crc = h_crc;
        result.cost = h_cost;

        result.mcc = current_mcc;
        result.mnc = current_mnc;
        result.bcc = current_bcc;
        result.encryption = current_enc;

        parse_mac_pdu(result);
    } else {
        result.encryption = current_enc;
    }

    return result;
}

uint32_t TetraChannelDecoder::read_bits(const uint8_t* bytes, size_t bit, size_t count) const {
    uint32_t value = 0;
    for (size_t i = 0; i < count; i++) {
        value <<= 1;
        value |= (bytes[(bit + i) >> 3] >> (7 - ((bit + i) & 7))) & 1;
    }
    return value;
}

void TetraChannelDecoder::parse_sync_pdu(Result& result) {
    // Sys 4 bit, then BCC
    result.bcc = read_bits(result.payload.data(), 4, 6);
    result.timeslot = read_bits(result.payload.data(), 10, 2);
    result.frame_number = read_bits(result.payload.data(), 12, 5);
    result.encryption = read_bits(result.payload.data(), 30, 1);
    result.mcc = read_bits(result.payload.data(), 31, 10);
    result.mnc = read_bits(result.payload.data(), 41, 14);

    last_mcc = result.mcc;
    last_mnc = result.mnc;
    last_bcc = result.bcc;
    network_synced = true;
}

void TetraChannelDecoder::parse_mac_pdu(Result& result) {
    result.pdu_type = read_bits(result.payload.data(), 0, 2);

    if (result.pdu_type == 0) {  // MAC-RESOURCE
        result.encryption = read_bits(result.payload.data(), 4, 2);
    } else if (result.pdu_type == 2) {  // SYSINFO/BCAST
        uint8_t bcast_type = read_bits(result.payload.data(), 2, 2);
        if (bcast_type == 0) {
            result.la = read_bits(result.payload.data(), 82, 14);
            result.encryption = read_bits(result.payload.data(), 122, 1);
        }
    }
}

TetraRxView::TetraRxView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&rssi, &channel, &field_rf_amp, &field_lna, &field_vga,
                  &field_frequency, &text_mcc, &text_la, &text_pdu,
                  &text_mnc, &text_ts, &text_fn, &text_bcc, &text_enc, &text_debug, &console});

    field_frequency.set_step(25000);

    receiver_model.set_modulation(ReceiverModel::Mode::NarrowbandFMAudio);
    receiver_model.set_sampling_rate(3072000);
    receiver_model.set_baseband_bandwidth(1750000);
    receiver_model.set_squelch_level(0);
    receiver_model.enable();
}

void TetraRxView::focus() {
    field_frequency.focus();
}

void TetraRxView::on_data_tetra(const TetraBurstMessage& message) {
    packet_count++;
    auto result = decoder.decode_burst(message);

    if (result.type == TetraChannelDecoder::Result::Type::Sync ||
        result.type == TetraChannelDecoder::Result::Type::SyncFull) {
        valid_count++;

        text_mcc.set("MCC: " + to_string_dec_uint(result.mcc));
        text_mnc.set("MNC: " + to_string_dec_uint(result.mnc));
        text_bcc.set("BCC: " + to_string_dec_uint(result.bcc));
        text_ts.set("TS:  " + to_string_dec_uint(result.timeslot));
        text_fn.set("FN:  " + to_string_dec_uint(result.frame_number));

        std::string enc_str = (result.encryption != 0) ? "Encrypted" : "Clear";
        text_enc.set("ENC: " + enc_str);

        if (result.type == TetraChannelDecoder::Result::Type::SyncFull) {
            h_valid_count++;
            if (result.la != 0xffff) text_la.set("LA:  " + to_string_dec_uint(result.la));
            text_pdu.set("PDU: " + decoder.get_pdu_name(result.pdu_type));
        }
    }
    text_debug.set(
        "Syn: " + to_string_dec_uint(packet_count) +
        ", V: " + to_string_dec_uint(valid_count) +
        ", H: " + to_string_dec_uint(h_valid_count) +
        ", E:" + to_string_dec_uint(result.sync_errors));
}

void TetraRxView::on_data_dnb(const TetraDnbMessage& message) {
    dnb_count++;
    auto result = decoder.decode_dnb(message);

    if (result.is_ok && result.type == TetraChannelDecoder::Result::Type::Dnb) {
        if (result.la != 0xffff) text_la.set("LA:  " + to_string_dec_uint(result.la));
        text_pdu.set("PDU: " + decoder.get_pdu_name(result.pdu_type));

        std::string enc_str = (result.encryption != 0) ? "Encrypted" : "Clear";
        text_enc.set("ENC: " + enc_str);

        if (rate_limiter++ < 2) console.writeln("DNB: " + decoder.get_pdu_name(result.pdu_type));
    }
}

void TetraRxView::on_timer() {
    rate_limiter = 0;
}

TetraRxView::~TetraRxView() {
    receiver_model.disable();
    baseband::shutdown();
}

}  // namespace ui::external_app::tetra_rx