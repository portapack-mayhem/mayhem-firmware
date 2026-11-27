/*
 * Copyright (C) 2026 Mayhem firmware contributors
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "ui_dab.hpp"
#include "dab/dab_fec.hpp"

#include "audio.hpp"
#include "baseband_api.hpp"
#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"
#include "string_format.hpp"
#include "file_path.hpp"
#include "rtc_time.hpp"

#include <cstring>

using namespace portapack;

namespace ui {

namespace {

/* The I log record for an image load. Build it right after the load: the
 * status is global and the next load overwrites it. */
std::string image_load_record(const char* name) {
    const char* status = "?";
    switch (baseband::last_image_load_status()) {
        case baseband::ImageLoadStatus::ok:
            status = "ok";
            break;
        case baseband::ImageLoadStatus::ready_timeout:
            status = "ready_timeout";
            break;
        case baseband::ImageLoadStatus::m4_panic:
            status = "m4_panic";
            break;
    }

    return "I," + std::string{name} + "," + status + "," +
           to_string_dec_uint(baseband::last_image_ready_ms()) + ",stage" +
           to_string_dec_uint(baseband::last_image_boot_stage()) + "," +
           baseband::last_image_panic();
}

/* field_block's value for a block name (Band III then L-Band, end to end, as
 * tune_block() decodes it), or -1. */
int32_t block_value(const char* name) {
    const auto band_iii = dab::band_iii_blocks();
    for (size_t i = 0; i < band_iii.count; i++)
        if (std::strcmp(band_iii.items[i].name, name) == 0)
            return static_cast<int32_t>(i);
    const auto l_band = dab::l_band_blocks();
    for (size_t i = 0; i < l_band.count; i++)
        if (std::strcmp(l_band.items[i].name, name) == 0)
            return static_cast<int32_t>(band_iii.count + i);
    return -1;
}

}  // namespace

Text* DabView::service_row(size_t index) {
    switch (index) {
        case 0:
            return &text_service_0;
        case 1:
            return &text_service_1;
        case 2:
            return &text_service_2;
        case 3:
            return &text_service_3;
        case 4:
            return &text_service_4;
        case 5:
            return &text_service_5;
        case 6:
            return &text_service_6;
        case 7:
            return &text_service_7;
        case 8:
            return &text_service_8;
        case 9:
            return &text_service_9;
        case 10:
            return &text_service_10;
        default:
            return &text_service_11;
    }
}

DabView::DabView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_dab);
    image_log_ = image_load_record("dab");

    /* Unset, the codec keeps the previous app's rate. 48 kHz is the 24 kHz
     * AAC core doubled by dab::AudioRing. Set before start(), which unmutes. */
    audio::set_rate(audio::Rate::Hz_48000);

    audio::output::start();

    add_children({&labels,
                  &field_frequency,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &rssi,
                  &field_volume,
                  &field_block,
                  &field_service,
                  &text_dls,
                  &text_quality,
                  &check_log});

    for (size_t row = 0; row < kServiceRows; row++)
        add_child(service_row(row));

    /* A value indexes Band III then L-Band end to end, as tune_block() decodes it. */
    OptionsField::options_t options;
    const auto band_iii = dab::band_iii_blocks();
    for (size_t i = 0; i < band_iii.count; i++)
        options.emplace_back(band_iii.items[i].name, static_cast<int32_t>(i));

    const auto l_band = dab::l_band_blocks();
    for (size_t i = 0; i < l_band.count; i++)
        options.emplace_back(l_band.items[i].name, static_cast<int32_t>(band_iii.count + i));

    field_block.set_options(std::move(options));

    field_block.on_change = [this](size_t, OptionsField::value_t v) {
        this->tune_block(static_cast<size_t>(v));
    };

    field_service.on_change = [this](size_t, OptionsField::value_t v) {
        if (!rebuilding_services_)
            this->select_service(static_cast<size_t>(v));
    };

    check_log.on_select = [this](Checkbox&, bool v) { this->set_logging(v); };

    set_logging(logging_);
    check_log.set_value(logging_);

    if (logger_ && !image_log_.empty())
        logger_->write_raw(image_log_);

    /* DAB mode I's native rate. The passband sits a quarter rate off centre;
     * the wide LPF opens the analogue filter to fit it. */
    receiver_model.set_wide_lpf_enabled(true);
    receiver_model.set_sampling_rate(2048000);
    receiver_model.enable();

    /* Pinned rather than restored from rx_dab.ini, so every run starts the
     * same; at these gains a capture decodes every FIB and audio unit. */
    receiver_model.set_rf_amp(true);
    receiver_model.set_lna(24);
    receiver_model.set_vga(32);

    int32_t start = block_value(block_name_.c_str());
    if (start < 0)
        start = block_value("11C");
    if (start >= 0)
        field_block.set_by_value(start);

    refresh();
}

DabView::~DabView() {
    audio::output::stop();
    receiver_model.disable();
    receiver_model.set_wide_lpf_enabled(false);
    baseband::shutdown();
}

void DabView::focus() {
    field_block.focus();
}

void DabView::tune_block(size_t table_index) {
    const auto band_iii = dab::band_iii_blocks();
    const auto l_band = dab::l_band_blocks();

    const dab::Block* block = nullptr;
    if (table_index < band_iii.count)
        block = &band_iii.items[table_index];
    else if (table_index - band_iii.count < l_band.count)
        block = &l_band.items[table_index - band_iii.count];

    if (block == nullptr)
        return;
    block_name_ = block->name;

    /* A new block is a new multiplex: the old list is wrong, not stale. */
    database_.clear();
    parser_.reset_counters();
    fib_error_rate_ = dab::kFibsPerFrame;
    search_frames_ = 0;
    stop_audio();
    pending_sid_ = station_sid_;

    receiver_model.set_target_frequency(block->frequency_hz);

    if (logger_)
        logger_->write_raw("B," + std::string(block->name) + "," + to_string_dec_uint(block->frequency_hz) + "," +
                           to_string_dec_uint(receiver_model.rf_amp() ? 1 : 0) + "," + to_string_dec_uint(receiver_model.lna()) + "," +
                           to_string_dec_uint(receiver_model.vga()) + "," + to_string_dec_uint(receiver_model.baseband_bandwidth()));

    refresh();
}

void DabView::on_soft(uint16_t offset, const uint8_t* data, uint8_t length) {
    if (!logger_)
        return;

    std::string line = "C," + to_string_dec_uint(offset) + ",";
    for (uint8_t i = 0; i < length; i++)
        line += to_string_hex(data[i], 2);
    logger_->write_raw(line);
}

void DabView::on_fib(const uint8_t* fib, bool crc_ok) {
    if (crc_ok)
        parser_.parse_fib(fib, database_);

    /* For replay against a reference decoder: F records are CRC-valid, R
     * records failed CRC (capped at 48). */
    if (logger_ && (crc_ok || raw_fibs_logged_ < 48)) {
        if (!crc_ok)
            ++raw_fibs_logged_;

        std::string line = crc_ok ? "F," : "R,";
        for (size_t i = 0; i < dab::kFibBytes; i++)
            line += to_string_hex(fib[i], 2);
        logger_->write_raw(line);
    }

    /* After a reconfiguration the old layout decodes to noise rather than an
     * error, so audio stops and the list is rebuilt. */
    if (database_.reconfiguration_pending()) {
        database_.acknowledge_reconfiguration();

        stop_audio();
        database_.clear();
        search_frames_ = 0;
    }
}

void DabView::set_logging(bool enabled) {
    /* The constructor's check_log.set_value() fires on_select, which calls
     * this again; reopening would double the header or orphan an empty log. */
    if (enabled && logging_ && logger_)
        return;

    logging_ = enabled;

    if (!enabled) {
        logger_.reset();
        return;
    }

    logger_ = std::make_unique<LogFile>();
    if (logger_->append(logs_dir / u"DAB_" + to_string_timestamp(rtc_time::now()) + u".TXT").is_valid()) {
        logger_.reset();
        logging_ = false;
        check_log.set_value(false);
        return;
    }

    logger_->write_raw_no_newline("# PortaPack DAB receiver log\r\n");
    logger_->write_raw_no_newline("# S,sync,freq_err_hz,fib_errors,cpu_percent,decoder_bytes,services,eid,symbol_kcycles,transform_kcycles,variant,null_dip_offset,null_dip_ratio,r4_milli,const_angle,guard_milli,spec_dc,spec_used,spec_unused,raw_used,raw_unused,fft_shift,fft_shift_raw,msc_angles,prs_peak,prs_shift,msc_state,decoder_counts,hdrfail_parsecalls,rszero_lfbytes\r\n");
    logger_->write_raw_no_newline(
        "# F,<32 bytes of a CRC-valid FIB, hex>\r\n"
        "# R,<32 bytes of a FIB that failed CRC, hex>\r\n");
    logger_->write_raw_no_newline(
        "# Q,<gp_clkin>,<pll0audio>,<idivd>,<pll0audio_locked> -- raw FREQ_MON counts,\r\n"
        "#   cycles in 480 IRC cycles; codec fs = idivd * 25000 / 256\r\n");
    logger_->write_raw_no_newline(
        "# B,<block>,<frequency_hz>,<rf_amp>,<lna_db>,<vga_db>,<baseband_filter_hz>\r\n"
        "# T,<station text (DLS) as displayed>\r\n"
        "# A,retry -- the station re-sent after reports without a decoded audio unit\r\n"
        "# C,<offset>,<punctured FIC soft bits, int8 hex>\r\n"
        "# I,<image>,<ok|ready_timeout|m4_panic>,<wait_ms>,"
        "stage<0=never ran,1=reset,2=segments,3=hal,4=kernel,5=main,6=audio,7=processor,8=dispatching>,"
        "<m4 panic>\r\n");

    /* The clock actually reaching the I2S, counted by the CGU's FREQ_MON
     * against the IRC, independent of any software path. */
    const auto clocks = clock_manager.measure_audio_clock();
    logger_->write_raw("Q," + to_string_dec_uint(clocks.gp_clkin) + "," +
                       to_string_dec_uint(clocks.pll0audio) + "," +
                       to_string_dec_uint(clocks.idivd) + "," +
                       to_string_dec_uint(clocks.pll0audio_locked ? 1 : 0));
}

/* Data services (EPG, slideshow carousels) have 32-bit service IDs; audio
 * services have 16-bit ones (EN 300 401 6.3.1). */
bool DabView::is_data_service(const dab::DabService& service) {
    return service.sid > 0xFFFF;
}

/* Mirrors the checks in the baseband's on_message(): a subchannel it refuses
 * leaves the FIC locked and the engagement never releases. */
bool DabView::fits_image(const dab::DabService& service) {
    constexpr uint16_t kMaxSubchannelCu = dab::kMaxCodedBitsPerCif / dab::kBitsPerCu;
    if (service.size_cu > kMaxSubchannelCu)
        return false;
    if (!dab::subchannel_placement_ok(service.start_cu, service.size_cu))
        return false;
    dab::ProtectionProfile profile{};
    profile.kind = dab::ProtectionKind::Eep;
    profile.level = service.protection.level;
    profile.option_b = service.protection.option_b;
    const uint32_t bits = dab::eep_subchannel_bits(service.size_cu, profile);
    return bits != 0 && bits <= dab::kMaxPayloadBitsPerCif;
}

/* A row that cannot play. The station already picked stays picked: every
 * release makes the baseband re-acquire (DAB_REACQ bit 2), so scrolling past
 * a "-" row would throw a working lock away. */
void DabView::refuse(const char* why) {
    if (!engage_.wanted())
        stop_audio();
    set_message(why);
}

void DabView::stop_audio() {
    /* Back to FIC only. The baseband restarts acquisition, so half-assembled
     * PAD is meaningless. */
    audio_selected_ = false;
    engage_.drop();
    selected_sid_ = 0;
    xpad_.reset();
    dls_.reset();
    set_message("");
    baseband::set_dab_subchannel(0, 0, 0, false, false);
}

void DabView::select_service(size_t index) {
    if (index >= database_.service_count())
        return;

    const auto& service = database_.services()[index];
    pending_sid_ = 0;
    if (!service.is_playable()) {
        if (!service.has_audio_component) {
            refuse("data service, no audio");
            return;
        }
        refuse(service.has_label() ? "waiting for subchannel" : "waiting for label");
        pending_sid_ = service.sid;
        return;
    }

    /* The baseband would refuse it silently, which looks like a dead receiver. */
    if (!fits_image(service)) {
        refuse(dab::subchannel_placement_ok(service.start_cu, service.size_cu) ? "too wide for this image"
                                                                               : "subchannel placement unsupported");
        return;
    }

    /* Until ASCTy arrives is_dab_plus reads false, and a DAB+ subchannel
     * engaged as classic DAB is decoded at full cost and discarded. */
    if (service.ascty == dab::kAsctyUnknown) {
        refuse("waiting for component type");
        pending_sid_ = service.sid;
        return;
    }

    if (!service.is_dab_plus) {
        refuse("DAB (MP2) not supported");
        return;
    }

    const Target target{service.start_cu, service.size_cu, service.protection.level,
                        service.protection.option_b, service.is_dab_plus};
    /* Back onto the playing station: re-sending would flush the deinterleaver,
     * superframe and decoder (half a second of silence) and lose the DLS text. */
    if (engage_.engaged() && service.sid == selected_sid_ && target == target_) {
        set_message(dls_.text());
        return;
    }

    selected_sid_ = service.sid;
    station_sid_ = service.sid;
    audio_selected_ = true;

    xpad_.reset();
    dls_.reset();
    set_message("");

    target_ = target;
    /* On a working lock the baseband swaps the subchannel without
     * re-acquiring; otherwise manage_engagement() waits for the lock. */
    if (engage_.want() == dab::Engagement::Action::Engage)
        engage_target();
    else
        set_message("tuning...");
}

void DabView::engage_target() {
    if (message_ == "tuning..." || message_ == "signal lost, retrying")
        set_message("");
    silent_frames_ = 0;
    baseband::set_dab_subchannel(target_.start_cu, target_.size_cu, target_.level,
                                 target_.option_b, target_.dab_plus,
                                 static_cast<uint32_t>(receiver_model.target_frequency()));
}

void DabView::release_target() {
    /* A dead constellation does not recover by itself. Releasing restarts
     * acquisition (DAB_REACQ bit 2); the pick is engaged again on lock. */
    xpad_.reset();
    dls_.reset();
    set_message("signal lost, retrying");
    baseband::set_dab_subchannel(0, 0, 0, false, false);
}

void DabView::manage_engagement(bool locked, uint16_t constellation_r4) {
    switch (engage_.on_frame(locked, constellation_r4, false)) {
        case dab::Engagement::Action::Engage:
            engage_target();
            break;
        case dab::Engagement::Action::Release:
            release_target();
            break;
        default:
            break;
    }
}

void DabView::retry_if_silent(uint16_t msc_state, uint16_t decoder_counts) {
    if (!(msc_state & 1) || !engage_.engaged() || (decoder_counts & 0xFF) != 0) {
        silent_frames_ = 0;
        return;
    }
    if (++silent_frames_ < kSilentRetryFrames)
        return;

    engage_target();
    /* Restart the warm-up and the loss window with it. */
    lost_state_ = 0;
    set_message("no audio, retrying");
    if (logger_)
        logger_->write_raw("A,retry");
}

void DabView::on_pad(const uint8_t* data, size_t length) {
    xpad_.push_pad(data, length, *this);
}

void DabView::on_dls_group(const uint8_t* data, size_t length) {
    if (dls_.push_segment(data, length) && dls_.take_new_text()) {
        set_message(dls_.text());
        if (logger_)
            logger_->write_raw("T," + message_);
    }
}

void DabView::refresh_service_options() {
    const size_t count = database_.service_count();

    /* Labels (FIG 1/1) arrive after the services (FIG 0/2), so the count
     * alone would freeze the dropdown on "(no label)". */
    uint32_t signature = static_cast<uint32_t>(count);
    for (size_t i = 0; i < count; i++) {
        const char* label = database_.services()[i].label;
        for (const char* c = label; *c != '\0'; ++c)
            signature = signature * 31u + static_cast<uint8_t>(*c);

        /* Placement, ASCTy and protection arrive later still and change the mark. */
        signature = signature * 31u + database_.services()[i].size_cu;
        signature = signature * 31u + database_.services()[i].start_cu;
        signature = signature * 31u + database_.services()[i].ascty;
        signature = signature * 31u + database_.services()[i].protection.level;
        signature = signature * 31u + (database_.services()[i].protection.option_b ? 1u : 0u);
        signature = signature * 31u + (database_.services()[i].is_playable() ? 1u : 0u);
    }

    if (count == service_options_count_ && signature == service_label_signature_)
        return;

    service_label_signature_ = signature;

    OptionsField::options_t options;
    for (size_t i = 0; i < count; i++) {
        const auto& service = database_.services()[i];
        std::string name = service.has_label() ? std::string(service.label) : std::string("(no label)");
        /* Skip data services and what this image can never play (the rows
         * still show those); what is not playable yet stays, marked "-". */
        if (is_data_service(service) || (service.is_playable() && !fits_image(service)))
            continue;
        if (!service.is_playable())
            name += " -";
        else if (service.ascty != dab::kAsctyUnknown && !service.is_dab_plus)
            name += " (mp2)";
        options.emplace_back(std::move(name), static_cast<int32_t>(i));
    }

    service_options_count_ = count;
    picker_count_ = options.size();

    rebuilding_services_ = true;
    field_service.set_options(std::move(options));

    /* The pick's index may have moved as the list filled in. */
    const uint32_t shown_sid = pending_sid_ ? pending_sid_ : audio_selected_ ? selected_sid_
                                                                             : 0;
    size_t pending_index = SIZE_MAX;
    for (size_t i = 0; shown_sid != 0 && i < count; i++) {
        if (database_.services()[i].sid == shown_sid) {
            field_service.set_by_value(static_cast<int32_t>(i));
            if (pending_sid_)
                pending_index = i;
            break;
        }
    }
    rebuilding_services_ = false;

    /* The list changed, so the waiting pick may be complete now. */
    if (pending_index != SIZE_MAX) {
        const auto& service = database_.services()[pending_index];
        if (service.is_playable() && service.ascty != dab::kAsctyUnknown)
            select_service(pending_index);
    }
}

namespace {
/* round(10^(k/10)) for k = 1..48: a power ratio's dB by counting. */
constexpr uint16_t kDbSteps[] = {1, 2, 2, 3, 3, 4, 5, 6, 8, 10, 13, 16, 20, 25, 32, 40,
                                 50, 63, 79, 100, 126, 158, 200, 251, 316, 398, 501, 631, 794, 1000, 1259, 1585,
                                 1995, 2512, 3162, 3981, 5012, 6310, 7943, 10000, 12589, 15849, 19953, 25119, 31623, 39811, 50119, 63096};

uint8_t power_db(uint32_t ratio) {
    uint8_t db = 0;
    while (db < sizeof(kDbSteps) / sizeof(kDbSteps[0]) && ratio >= kDbSteps[db])
        ++db;
    return db;
}
}  // namespace

void DabView::account_quality(bool locked, uint16_t null_dip_ratio, uint16_t msc_state, uint16_t pcm_report) {
    locked_ = locked;
    /* The null carries no signal, so reference over null power is (S+N)/N.
     * Smoothed over ~16 frames: a frame whose null was missed reads ~7. */
    if (locked && null_dip_ratio) {
        const int32_t db16 = power_db(null_dip_ratio) * 16;
        snr16_ = snr16_ ? static_cast<uint16_t>(snr16_ + (db16 - snr16_) / 16) : static_cast<uint16_t>(db16);
    }
    if (!(msc_state & 1)) {
        if (warm_ && (message_ == "warming up..." || message_ == "no audio, retrying"))
            set_message("");
        lost_state_ = 0;
        warm_ = false;
        return;
    }
    if (msc_state != lost_state_) {
        lost_state_ = msc_state;
        for (auto& d : lost_dry_)
            d = 0;
        lost_bucket_ = 0;
        lost_full_ = 0;
        lost_frames_ = 0;
        warm_ = true;
        warm_frames_ = 0;
        warm_flow_ = 0;
        if (message_.empty())
            set_message("warming up...");
        return;
    }
    const uint8_t dry = static_cast<uint8_t>(pcm_report >> 8);
    if (warm_) {
        warm_flow_ = dry == 0 ? static_cast<uint8_t>(warm_flow_ + 1) : 0;
        if (warm_flow_ >= kWarmFlowFrames || ++warm_frames_ >= kWarmMaxFrames) {
            warm_ = false;
            if (message_ == "warming up..." || (warm_flow_ >= kWarmFlowFrames && message_ == "no audio, retrying"))
                set_message("");
        }
        return;
    }
    lost_dry_[lost_bucket_] = static_cast<uint16_t>(lost_dry_[lost_bucket_] + dry);
    if (++lost_frames_ >= kLostBucketFrames) {
        lost_frames_ = 0;
        lost_bucket_ = static_cast<uint8_t>((lost_bucket_ + 1) % kLostBuckets);
        lost_dry_[lost_bucket_] = 0;
        if (lost_full_ < kLostBuckets - 1)
            ++lost_full_;
    }
}

void DabView::on_status(const DabStatusMessage& m) {
    const bool locked = m.sync == DabStatusMessage::Sync::Locked;
    fib_error_rate_ = m.fib_errors;
    frequency_error_hz_ = m.frequency_error_hz;
    decoder_bytes_ = m.decoder_bytes;

    /* A 96 ms frame is 19584000 cycles at 204 MHz. Near 100% the receiver
     * cannot keep up with the samples and loses lock. */
    constexpr uint32_t cycles_per_frame = 19584000u / 100u;
    const uint32_t percent = m.frame_cycles / cycles_per_frame;
    cpu_percent_ = static_cast<uint8_t>(percent > 255 ? 255 : percent);

    manage_engagement(locked, m.constellation_r4);
    retry_if_silent(m.msc_state, m.decoder_counts);
    account_quality(locked, m.null_dip_ratio, m.msc_state, m.spectrum_raw_used);
    if (search_frames_ < 0xFFFF)
        ++search_frames_;

    if (logger_) {
        logger_->write_raw(
            "S," + to_string_dec_uint(locked ? 1 : 0) +
            "," + to_string_dec_int(frequency_error_hz_) +
            "," + to_string_dec_uint(m.fib_errors) +
            "," + to_string_dec_uint(cpu_percent_) +
            "," + to_string_dec_uint(decoder_bytes_) +
            "," + to_string_dec_uint(database_.service_count()) +
            "," + to_string_hex(database_.ensemble_id(), 4) +
            "," + to_string_dec_uint(m.symbol_cycles / 1000) +
            "," + to_string_dec_uint(m.transform_cycles / 1000) +
            "," + to_string_dec_uint(m.decode_variant) +
            "," + to_string_dec_uint(m.null_dip_offset) +
            "," + to_string_dec_uint(m.null_dip_ratio) +
            "," + to_string_dec_uint(m.constellation_r4) +
            "," + to_string_dec_int(m.const_angle) +
            "," + to_string_dec_uint(m.guard_quality) +
            "," + to_string_dec_uint(m.spectrum_dc) +
            "," + to_string_dec_uint(m.spectrum_used) +
            "," + to_string_dec_uint(m.spectrum_unused) +
            "," + to_string_dec_uint(m.spectrum_raw_used) +
            "," + to_string_dec_uint(m.spectrum_raw_unused) +
            "," + to_string_dec_uint(m.fft_shifts & 0xFF) +
            "," + to_string_dec_uint(m.fft_shifts >> 8) +
            "," + to_string_dec_uint(m.msc_angles) +
            "," + to_string_dec_uint(m.prs_peak) +
            "," + to_string_dec_int(m.prs_shift) +
            "," + to_string_dec_uint(m.msc_state) +
            "," + to_string_dec_uint(m.decoder_counts) +
            "," + to_string_dec_uint(m.header_fails) +
            "," + to_string_dec_uint(m.assembler_shape));
    }

    refresh();
}

void DabView::set_message(const std::string& text) {
    if (text == message_)
        return;
    message_ = text;
    message_offset_ = 0;
    message_hold_ = kMessageHold;
    render_message();
}

/* One line under the Rx line; text longer than it scrolls, pausing at the
 * start of each pass. Called every status (~10 a second). */
void DabView::render_message() {
    const size_t columns = static_cast<size_t>(text_dls.parent_rect().width()) / 8;
    if (message_.size() <= columns) {
        text_dls.set(message_);
        return;
    }
    const std::string loop = message_ + "   ";
    if (message_hold_ > 0) {
        --message_hold_;
    } else if (++message_tick_ >= kMessageTicksPerChar) {
        message_tick_ = 0;
        message_offset_ = (message_offset_ + 1) % loop.size();
        if (message_offset_ == 0)
            message_hold_ = kMessageHold;
    }
    text_dls.set((loop + loop).substr(message_offset_, columns));
}

void DabView::refresh() {
    render_message();
    refresh_service_options();

    /* Fixed width, field by field, so nothing moves as the numbers change:
     *   "28dB Lost:   0.3% CPU: 79%"   a station playing
     *   "28dB Lost:   warm CPU: 79%"   picked, deinterleaver still filling
     *   "28dB FIB:    9/12 CPU: 30%"   no station: FIBs failed of 12 */
    std::string quality = locked_ ? to_string_dec_uint((snr16_ + 8) / 16, 2) + "dB " : std::string("--dB ");
    if (lost_state_ & 1) {
        if (warm_) {
            quality += "Lost:   warm ";
        } else {
            uint32_t dry = 0;
            for (const auto d : lost_dry_)
                dry += d;
            const uint32_t frames = static_cast<uint32_t>(lost_full_) * kLostBucketFrames + lost_frames_;
            uint32_t permille = frames ? dry * 1000 / (frames * kTransfersPerFrame) : 0;
            if (permille > 1000)
                permille = 1000;
            quality += "Lost: " + to_string_dec_uint(permille / 10, 3) + "." + to_string_dec_uint(permille % 10) + "% ";
        }
    } else {
        quality += "FIB:   " + to_string_dec_uint(fib_error_rate_, 2) + "/12 ";
    }
    quality += "CPU:" + to_string_dec_uint(cpu_percent_, 3) + "%";
    text_quality.set(quality);

    /* Marker for the picked station, name, then "+" for DAB+ ("X" if this
     * image cannot play it) and the bit rate right-aligned at the edge:
     *   ">Station name            +  96k" */
    const size_t columns = static_cast<size_t>(service_row(0)->parent_rect().width()) / 8;
    constexpr size_t kTail = 6;
    const size_t name_width = columns > kTail ? columns - kTail : 0;
    /* Rows are the audio services only; `focus` is a database index and
     * `focus_row` its position among them. */
    uint16_t shown[dab::kMaxServices];
    size_t count = 0;
    const size_t focus = picker_count_ ? static_cast<size_t>(field_service.selected_index_value()) : SIZE_MAX;
    size_t focus_row = SIZE_MAX;
    for (size_t i = 0; i < database_.service_count() && count < dab::kMaxServices; i++) {
        if (is_data_service(database_.services()[i]))
            continue;
        if (i == focus)
            focus_row = count;
        shown[count++] = static_cast<uint16_t>(i);
    }
    if (focus_row != SIZE_MAX) {
        if (focus_row < list_top_)
            list_top_ = focus_row;
        else if (focus_row >= list_top_ + kServiceRows)
            list_top_ = focus_row - kServiceRows + 1;
    }
    if (list_top_ + kServiceRows > count)
        list_top_ = (count > kServiceRows) ? count - kServiceRows : 0;

    /* An empty list says what it is waiting for; 96 ms a report. */
    std::string waiting[2]{};
    if (count == 0) {
        const bool found = database_.has_ensemble_id();
        waiting[0] = std::string(found ? " Reading station list... " : " Searching for signal... ") +
                     to_string_dec_uint(search_frames_ * 96u / 1000u) + "s";
        if (!found && search_frames_ >= kAntennaHintFrames)
            waiting[1] = " Check antenna and block";
    }

    for (size_t row = 0; row < kServiceRows; row++) {
        if (list_top_ + row >= count) {
            service_row(row)->set(row < 2 ? waiting[row] : std::string());
            continue;
        }

        const size_t index = shown[list_top_ + row];
        const auto& service = database_.services()[index];

        std::string line = (index == focus) ? ">" : " ";
        line += service.has_label() ? std::string(service.label) : std::string("(no label)");
        line.resize(name_width, ' ');

        if (service.has_subchannel()) {
            const bool mp2 = service.ascty != dab::kAsctyUnknown && !service.is_dab_plus;
            line += (!fits_image(service) || mp2) ? "X " : service.is_dab_plus ? "+ "
                                                                               : "  ";
            line += to_string_dec_uint(service.bitrate / 1000, 3) + "k";
        } else {
            line += "     -";
        }

        service_row(row)->set(line);
    }
}

} /* namespace ui */
