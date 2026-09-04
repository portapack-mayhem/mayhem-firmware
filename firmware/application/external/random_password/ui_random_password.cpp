/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
 * Copyright (C) 2024 zxkmm
 * Copyright (C) 2024 HTotoo
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

/* =====================================================================
 * Where the randomness comes from, what happens to it, and what is and
 * is not being claimed.
 * =====================================================================
 *
 * SOURCE
 *
 * The only entropy source is the radio, and specifically it is *not* any
 * demodulated signal. firmware/baseband/proc_entropy.cpp taps the raw
 * SGPIO/DMA I/Q buffer -- the 8-bit samples straight out of the MAX5864,
 * before any decimation -- and harvests bit 0 of I and bit 0 of Q as two
 * separate raw bit streams.
 *
 * The physical claim is the ordinary ADC-noise TRNG argument: with any gain
 * at all, the receiver's own thermal noise (LNA noise figure, kTB over a
 * multi-MHz analogue bandwidth, amplified 20-60 dB) is many LSB rms at the
 * ADC, and when the noise is much larger than one quantisation step the LSB
 * is close to a fair coin. Two consequences worth being explicit about:
 *
 *   - It does not need a signal. Dead antenna, empty band, screen room: the
 *     noise being harvested is the front end's own, so those cases are not
 *     degraded cases. They are the normal case.
 *   - A strong carrier does not erase it. The carrier correlates the LSB with
 *     its own modulation, which the estimator below sees and penalises, but
 *     thermal noise is still additive on top of it.
 *
 * Only bit 0 is transported. The upper bits carry the signal -- the part an
 * adversary can control -- and they would add no credited entropy, so leaving
 * them out keeps the pool made of exactly the bits that were tested.
 *
 * The M4 also ships its cycle-count delta between DMA buffer arrivals. The DMA
 * is clocked from the Si5351 and the core from PLL1, so that delta is a real
 * two-oscillator beat. It is mixed in UNCREDITED: its jitter magnitude has
 * never been measured on this hardware, so it must not be allowed to make a
 * dead radio look alive.
 *
 * RATE
 *
 * 2 bits/sample at 2 Msps is ~4 Mbit/s of candidate bits. The previous version
 * of this app read AFSK words at 1200 baud, where -- because proc_afskrx's
 * RS232 branch never clears its 32-bit shift register -- each message carried
 * only 8 fresh bits, about 960 bit/s. So this is roughly four thousand times
 * more raw material. That surplus is not spent on speed. It is spent on being
 * able to discount heavily, to throw away every block that fails a health test
 * without the user noticing, and to leave the M4 94% idle.
 *
 * PIPELINE, and why each stage is there
 *
 *   1. Harvest + health test + estimate (M4, proc_entropy.cpp).
 *      Health tests run on the RAW stream, before any conditioning, which is
 *      the only place a health test means anything.
 *
 *   2. Credit (M4). min(0.125, 0.5 * h_hat) bits of min-entropy per raw bit,
 *      where h_hat is a first-order estimate. Shipped as integer milli-bits
 *      because the M0 has no FPU. See BUDGET below.
 *
 *   3. Pool (M0). A running SHA-256 context, not a buffer. Every block ever
 *      received is absorbed -- raw bytes, credit, health flags, M4 jitter,
 *      plus M0-side RTC and system tick. Nothing collected is ever discarded.
 *      Being a hash context rather than a buffer is also what keeps this off
 *      the M0's 4 KB stack.
 *
 *   4. Extract (M0). When the credit reaches the budget, a *copy* of the pool
 *      is finalized to a 32-byte seed.
 *
 *      This stage is the one that is easy to mistake for decoration, so:
 *      the hash adds no entropy, but it is not optional. The pool holds (by
 *      claim) about 0.125 bits of min-entropy per bit, smeared thinly across
 *      thousands of biased, correlated bits. You cannot index a charset with
 *      a bit that is 60/40. The extractor's job is to concentrate entropy that
 *      already exists into a smaller number of near-uniform bits -- the
 *      leftover-hash-lemma role, the same role SP 800-90B gives a vetted
 *      conditioning function. Without it the entropy is present but
 *      unreachable. It also globalises: every byte ever collected influences
 *      every output character, so a localised dropout can only weaken the
 *      whole password (which the budget already accounts for) and can never
 *      produce an individually guessable character position.
 *
 *   5. Generate (M0). Hash-DRBG: SHA256(seed || counter). Characters are
 *      chosen by REJECTION SAMPLING, not modulo -- the modulo bias was only
 *      about 0.005 bits/char, a nit rather than a hole, but rejection is the
 *      same amount of code.
 *
 *   6. Carry. SHA256(seed || "carry") is folded into the next pool state, so
 *      entropy is never destroyed between passwords. The credit counter is
 *      nonetheless reset to zero, so every password waits for a fresh budget
 *      of radio rather than spinning off one seed.
 *
 * BUDGET: 256 bits, at one credited bit per byte of raw LSBs
 *
 * The physical claim above is about 1 bit per raw bit. This app credits 0.125
 * -- an 8x discount -- and full credit additionally requires the measured
 * first-order estimate to be at least 0.25 bits/bit, a source already four
 * times worse than expected. Below that the credit falls off proportionally.
 *
 * 256 bits is fixed rather than derived from password length because the most
 * this app can emit is 30 * log2(67) = 182 bits, so 256 always covers the
 * output with margin. It fills in roughly 65 ms.
 *
 * WHAT IS NOT VERIFIED
 *
 * None of the following has been measured on hardware by the author of this
 * pipeline, and no claim here should be read as if it had been:
 *
 *   - The actual noise amplitude at the ADC LSB, at any gain setting. The
 *     "many LSB rms" argument is a datasheet-and-physics argument, not a
 *     measurement. If bit 0 were in fact stuck, the repetition test would fire
 *     and the app would refuse to generate -- that is the intended failure
 *     mode, not a hypothetical one.
 *   - The magnitude of the DMA/core clock jitter. Hence uncredited.
 *   - Whether 2 Msps with the LPF wide open actually decorrelates consecutive
 *     samples as intended. The estimator's lag-1 term would partly reveal
 *     failure here; higher-order correlation it cannot see.
 *
 * The estimator is first-order over a 2048-bit window. It cannot detect
 * higher-order or long-period structure, and it is used only to pull the
 * credit DOWN below the fixed cap, never to raise it. This is also why the
 * frequency hopping is uniform and random rather than "hop until the numbers
 * look good": selecting channels by a first-order statistic would steer the
 * app toward exactly the signal an adversary would synthesise to spoof it.
 *
 * FAILURE MODES
 *
 *   - Stuck, clipped or dead front end: repetition-count fires, the block is
 *     credited zero, and after 8 consecutive bad blocks the alarm latches and
 *     generation is refused with the reason on screen. Note this is a change
 *     in kind from the old code, where a fully degenerate stream happened to
 *     stall the AFSK framer and refuse *by accident*.
 *   - Gross bias, or a period-2 pattern from a half-stuck ADC channel: the
 *     proportion and lag-1 transition tests fire. I and Q are tested as
 *     separate streams on purpose -- a DC-stuck front end gives two
 *     individually-constant channels, which interleave into a perfectly
 *     healthy-looking 0101... alternation.
 *   - Partially degraded but still plausible: this is the dangerous one and it
 *     is handled by discounting, not by detection. The credit rate drops, the
 *     progress bar visibly slows, and the numbers stay on screen. The app does
 *     not hide a weak source behind a good-looking password.
 *
 * This is not a certified TRNG and there is no warranty. What it is, is a
 * generator that states a budget, enforces it, shows its work, and refuses to
 * run when its source is measurably broken.
 * =====================================================================
 */

#include "ui_random_password.hpp"

#include "rtc_time.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"
#include "file_path.hpp"

#include <algorithm>
#include <cstring>

using namespace portapack;
using namespace ui;

namespace ui::external_app::random_password {

void RandomPasswordLogger::log_raw_data(const std::string& data) {
    log_file.write_entry(data);
}

void RandomPasswordView::focus() {
    button_refresh.focus();
}

RandomPasswordView::RandomPasswordView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&rssi,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &field_frequency,
                  &labels,
                  &text_pool,
                  &progressbar,
                  &text_health,
                  &text_generated_passwd,
                  &text_char_type_hints,
                  &field_digits,
                  &check_auto_send,
                  &check_hop,
                  &check_allow_confusable_chars,
                  &check_log,
                  &check_digits,
                  &check_punctuation,
                  &check_latin_lower,
                  &check_latin_upper,
                  &button_flood,
                  &button_send,
                  &button_refresh,
                  &button_show_qr});

    progressbar.set_max(required_mbits / 1000);

    check_digits.set_value(true);
    check_latin_lower.set_value(true);
    check_latin_upper.set_value(true);
    check_punctuation.set_value(true);
    check_hop.set_value(true);
    check_log.set_value(false);
    field_digits.set_value(16);

    /* Frequency choice is not an entropy source and is not claimed to be one.
     * This state only has to be unpredictable enough that two units powered on
     * together do not walk the band in lockstep. */
    {
        SHA256 h;
        h.init();
        const uint32_t t0 = LPC_RTC->CTIME0;
        const uint32_t t1 = LPC_RTC->CTIME1;
        const uint32_t tick = chTimeNow();
        h.update(&t0, sizeof(t0));
        h.update(&t1, sizeof(t1));
        h.update(&tick, sizeof(tick));
        h.final(hop_state_);
    }

    reset_pool(nullptr);

    check_log.on_select = [this](Checkbox&, bool v) {
        if (v) {
            nav_.display_modal(
                "Warning",
                "Sure?\n"
                "saves passwords AND the\n"
                "raw radio bytes they came\n"
                "from, plain text, to sdcard\n"
                "the raw bytes ARE the seed\n"
                "for quality checking only",
                YESNO,
                [this, v](bool c) {
                    if (c) {
                        logging = v;
                    } else {
                        check_log.set_value(false);
                        // this is needed to check back to false cuz when trigger by human, the check to true already happened
                        // this blocked interface so won't accidently saved even if user checked but selected no later here,
                        // but take care of here if in the future implemented ticking/auto/batch save etc
                    }
                });
        } else {
            logging = v;
        }
    };

    const auto regenerate = [this](Checkbox&, bool) { this->new_password(); };
    check_digits.on_select = regenerate;
    check_latin_lower.on_select = regenerate;
    check_latin_upper.on_select = regenerate;
    check_punctuation.on_select = regenerate;
    check_allow_confusable_chars.on_select = regenerate;

    field_digits.on_change = [this](int32_t) { this->new_password(); };

    button_refresh.on_select = [this](Button&) { this->new_password(); };

    button_show_qr.on_select = [this, &nav](Button&) {
        if (!password.empty())
            nav.push<QRCodeView>(password.data());
    };

    button_flood.on_select = [this](Button&) {
        flooding = !flooding;
        button_flood.set_text(flooding
                                  ? LanguageHelper::currentMessages[LANG_STOP]
                                  : LanguageHelper::currentMessages[LANG_FLOOD]);
    };

    button_send.on_select = [this](Button&) {
        if (password.empty())
            return;
        async_prev_val = portapack::async_tx_enabled;
        portapack::async_tx_enabled = true;
        UsbSerialAsyncmsg::asyncmsg(password);
        portapack::async_tx_enabled = async_prev_val;
    };

    logger = std::make_unique<RandomPasswordLogger>();
    if (logger)
        logger->append(logs_dir / u"random.TXT");

    field_frequency.updated = [this](rf::Frequency) { this->on_frequency_changed(); };

    receiver_model.enable();
    receiver_model.set_rf_amp(false);
    receiver_model.set_sampling_rate(harvest_sampling_rate);
    receiver_model.set_baseband_bandwidth(harvest_bandwidth);

    hop_frequency();
    /* hop_frequency() only flushes if the value actually changed; make sure
     * the M4 is configured and running either way. */
    on_frequency_changed();

    text_generated_passwd.set("collecting entropy...");
}

RandomPasswordView::~RandomPasswordView() {
    baseband::set_entropy_rx(harvest_sampling_rate, harvest_buffer_decimation, 0, false);
    receiver_model.disable();
    baseband::shutdown();
}

/* ---------------------------------------------------------------- pool --- */

void RandomPasswordView::reset_pool(const uint8_t* carry) {
    pool_.init();
    if (carry)
        pool_.update(carry, SHA256::digest_size);
    pool_credit_mbits_ = 0;
    mbits_since_hop_ = 0;
    log_ring_count_ = 0;
}

bool RandomPasswordView::pool_ready() const {
    return !source_failed_ && (pool_credit_mbits_ >= required_mbits);
}

void RandomPasswordView::absorb_block(const EntropyBlockMessage& message) {
    /* Absorbing costs nothing and hashing cannot reduce entropy, so even a
     * block that failed its health tests goes into the pool. What a failure
     * withholds is the *credit*, never the data. */
    pool_.update(message.data, EntropyBlockMessage::block_bytes);
    pool_.update(&message.jitter, sizeof(message.jitter));
    pool_.update(&message.credit_mbits, sizeof(message.credit_mbits));
    pool_.update(&message.h_min_mbits, sizeof(message.h_min_mbits));
    pool_.update(&message.health_flags, sizeof(message.health_flags));

    /* Uncredited auxiliaries. Free, cannot hurt, and deliberately worth zero
     * bits so they can never paper over a dead radio. */
    const uint32_t rtc0 = LPC_RTC->CTIME0;
    const uint32_t rtc1 = LPC_RTC->CTIME1;
    const uint32_t tick = chTimeNow();
    pool_.update(&rtc0, sizeof(rtc0));
    pool_.update(&rtc1, sizeof(rtc1));
    pool_.update(&tick, sizeof(tick));
}

void RandomPasswordView::on_entropy_block(const EntropyBlockMessage& message) {
    absorb_block(message);

    last_h_mbits_ = message.h_min_mbits;
    last_flags_ = message.health_flags;

    if (message.health_flags != 0) {
        consecutive_good_ = 0;
        if (++consecutive_bad_ >= health_latch_blocks) {
            /* A source that has been failing for 8 blocks running is not
             * having a bad moment. Drop the credit as well: it was collected
             * on the way into the failure. */
            source_failed_ = true;
            pool_credit_mbits_ = 0;
        }
    } else {
        consecutive_bad_ = 0;
        if (source_failed_ && (++consecutive_good_ >= health_latch_blocks))
            source_failed_ = false;

        if (!source_failed_) {
            pool_credit_mbits_ += message.credit_mbits;
            mbits_since_hop_ += message.credit_mbits;

            if (logging && log_ring_count_ < log_ring_size) {
                auto& slot = log_ring_[log_ring_count_++];
                memcpy(slot.data, message.data, EntropyBlockMessage::block_bytes);
                slot.freq_hz = static_cast<uint32_t>(receiver_model.target_frequency());
                slot.h_mbits = message.h_min_mbits;
                slot.flags = message.health_flags;
            }
        }
    }

    if (check_hop.value() && (mbits_since_hop_ >= hop_interval_mbits) && !pool_ready())
        hop_frequency();

    /* Blocks arrive at ~61 Hz. Redrawing text and the bar that often just
     * hammers the LCD for no readable benefit. */
    if ((++ui_throttle_ & 7) == 0)
        update_status_text();

    /* The first pool to fill produces a password without being asked, so the
     * app is not just sitting there looking broken on entry. One-shot rather
     * than `password.empty()`: with every charset unticked, generation legally
     * produces nothing, and testing emptiness would retry 61 times a second. */
    if (pool_ready() && (flooding || !auto_generated_)) {
        auto_generated_ = true;
        new_password();
    }
}

void RandomPasswordView::update_status_text() {
    const uint32_t bits = std::min(pool_credit_mbits_, required_mbits) / 1000;
    progressbar.set_value(bits);
    text_pool.set(to_string_dec_uint(bits) + "/" +
                  to_string_dec_uint(required_mbits / 1000) + " bit");

    /* Budget: the Text is 30 characters wide. "H 0.98  " is 8, leaving 22,
     * and the worst case below is "hlth: stuck bias per" at 20. */
    std::string status = "H " + to_string_dec_uint(last_h_mbits_ / 1000) + "." +
                         to_string_dec_uint((last_h_mbits_ / 10) % 100, 2, '0') +
                         "  ";

    if (source_failed_) {
        status += "SOURCE FAILED";
    } else if (last_flags_ != 0) {
        status += "hlth:";
        if (last_flags_ & (EntropyBlockMessage::health_repetition_i |
                           EntropyBlockMessage::health_repetition_q))
            status += " stuck";
        if (last_flags_ & (EntropyBlockMessage::health_proportion_i |
                           EntropyBlockMessage::health_proportion_q))
            status += " bias";
        if (last_flags_ & (EntropyBlockMessage::health_transition_i |
                           EntropyBlockMessage::health_transition_q))
            status += " per";
    } else {
        status += "source ok";
    }

    text_health.set(status);
}

/* ----------------------------------------------------------- frequency --- */

uint32_t RandomPasswordView::hop_random() {
    SHA256 h;
    h.init();
    h.update(hop_state_, sizeof(hop_state_));
    h.final(hop_state_);

    uint32_t v = 0;
    memcpy(&v, hop_state_, sizeof(v));
    return v;
}

void RandomPasswordView::hop_frequency() {
    /* 100 MHz - 1 GHz. The range is chosen for the antennas people actually
     * put on this thing and for front-end behaviour, NOT because some part of
     * the band is "more random" -- the noise being harvested is the
     * receiver's own. Hopping earns its place for a different reason: it
     * means no single emitter is present for the whole of a pool. */
    retune(100000000u + (hop_random() % 900000000u));
}

void RandomPasswordView::retune(uint32_t freq_hz) {
    /* Setting the field drives the model and then fires `updated`, which is
     * where the M4 flush happens -- so a manual retune by the user gets the
     * same settling treatment as an automatic hop. */
    field_frequency.set_value(freq_hz);
}

void RandomPasswordView::on_frequency_changed() {
    mbits_since_hop_ = 0;

    /* Re-send the config so the M4 drops the next few buffers. PLL settling
     * after a retune produces high-amplitude *deterministic* transients, which
     * are worse than no data at all. */
    baseband::set_entropy_rx(harvest_sampling_rate,
                             harvest_buffer_decimation,
                             harvest_settle_buffers,
                             true);
}

/* ------------------------------------------------------------ generate --- */

std::string RandomPasswordView::build_charset() const {
    std::string charset;

    if (check_digits.value()) {
        charset += "23456789";
        if (check_allow_confusable_chars.value()) charset += "01";
    }
    if (check_latin_lower.value()) {
        charset += "abcdefghijkmnpqrstuvwxyz";
        if (check_allow_confusable_chars.value()) charset += "ol";
    }
    if (check_latin_upper.value()) {
        charset += "ABCDEFGHIJKLMNPQRSTUVWXYZ";
        if (check_allow_confusable_chars.value()) charset += "O";
    }
    if (check_punctuation.value())
        charset += ".,-!?";

    return charset;
}

void RandomPasswordView::new_password() {
    if (source_failed_) {
        text_generated_passwd.set("refused: source failed");
        text_char_type_hints.set("check antenna / gain");
        return;
    }

    if (!pool_ready()) {
        text_generated_passwd.set("collecting entropy...");
        text_char_type_hints.set(
            to_string_dec_uint(pool_credit_mbits_ / 1000) + " of " +
            to_string_dec_uint(required_mbits / 1000) + " bits");
        return;
    }

    const std::string charset = build_charset();
    if (charset.empty()) {
        text_generated_passwd.set("generate failed,");
        text_char_type_hints.set("select at least 1 type");
        return;
    }

    const size_t password_length = static_cast<size_t>(field_digits.value());

    /* Finalize a COPY so the running pool keeps its history. */
    uint8_t seed[SHA256::digest_size];
    {
        SHA256 extract = pool_;
        extract.final(seed);
    }

    /* Hash-DRBG over the extracted seed. */
    uint32_t counter = 0;
    uint8_t drbg_block[SHA256::digest_size];
    size_t drbg_pos = SHA256::digest_size;

    const auto next_byte = [&]() -> uint8_t {
        if (drbg_pos >= SHA256::digest_size) {
            SHA256 h;
            h.init();
            h.update(seed, sizeof(seed));
            h.update(&counter, sizeof(counter));
            h.final(drbg_block);
            counter++;
            drbg_pos = 0;
        }
        return drbg_block[drbg_pos++];
    };

    /* Rejection sampling: discard the top partial cycle of the byte range so
     * every character is exactly uniform over the charset. */
    const uint32_t charset_size = static_cast<uint32_t>(charset.length());
    const uint32_t limit = 256u - (256u % charset_size);

    password.clear();
    std::string char_type_hints;
    for (size_t i = 0; i < password_length; i++) {
        uint32_t b;
        do {
            b = next_byte();
        } while (b >= limit);

        const char c = charset[b % charset_size];
        password += c;

        if (isdigit(c))
            char_type_hints += "1";
        else if (islower(c))
            char_type_hints += "a";
        else if (isupper(c))
            char_type_hints += "A";
        else
            char_type_hints += ",";
    }

    /* Carry: keeps the collected history across generations without letting a
     * leaked carry reveal the seed that produced this password. */
    uint8_t carry[SHA256::digest_size];
    {
        SHA256 h;
        h.init();
        h.update(seed, sizeof(seed));
        h.update("carry", 5);
        h.final(carry);
    }

    if (logger && logging) {
        std::string line = generate_log_line();
        logger->log_raw_data(line);
    }

    /* The credit resets to zero even though the state carries: every password
     * waits for its own fresh budget of radio. */
    reset_pool(carry);

    memset(seed, 0, sizeof(seed));
    memset(carry, 0, sizeof(carry));
    memset(drbg_block, 0, sizeof(drbg_block));

    text_generated_passwd.set(password);
    text_char_type_hints.set(char_type_hints);
    paint_password_hints();
    update_status_text();

    if (check_auto_send.value() || flooding) {
        async_prev_val = portapack::async_tx_enabled;
        portapack::async_tx_enabled = true;
        UsbSerialAsyncmsg::asyncmsg(password);
        portapack::async_tx_enabled = async_prev_val;
    }
}

/* ----------------------------------------------------------------- ui ---- */

bool RandomPasswordView::islower(char c) {
    return (c >= 'a' && c <= 'z');
}

bool RandomPasswordView::isupper(char c) {
    return (c >= 'A' && c <= 'Z');
}

bool RandomPasswordView::isdigit(char c) {
    return (c >= '0' && c <= '9');
}

void RandomPasswordView::paint_password_hints() {
    Painter painter;
    const int char_width = 8;
    const int char_height = 16;
    const int start_y = 6 * char_height;
    const int rect_height = 4;

    painter.fill_rectangle(
        {{0, start_y}, {screen_width, rect_height}},
        Theme::getInstance()->bg_darkest->background);

    for (size_t i = 0; i < password.length(); i++) {
        const char c = password[i];
        Color color;
        if (isdigit(c)) {
            color = Color::red();
        } else if (islower(c)) {
            color = Color::green();
        } else if (isupper(c)) {
            color = Color::blue();
        } else {
            color = Color::white();
        }

        painter.fill_rectangle(
            {{static_cast<int>(i) * char_width, start_y},
             {char_width, rect_height}},
            color);
    }
}

/* Log format, line oriented, parsed by tools/rndpw_entropy_check.py.
 *
 * The R lines are the raw harvested LSB bytes -- they ARE the seed material,
 * which is exactly why the checkbox warns and why it defaults off. They exist
 * so the source can be assessed offline instead of taken on trust. */
std::string RandomPasswordView::generate_log_line() const {
    std::string line = "\n# rndpw2 " + to_string_datetime(rtc_time::now());

    for (size_t i = 0; i < log_ring_count_; i++) {
        const auto& slot = log_ring_[i];
        line += "\nR " + to_string_dec_uint(slot.freq_hz) +
                " " + to_string_dec_uint(slot.h_mbits) +
                " " + to_string_dec_uint(slot.flags) + " ";
        for (size_t b = 0; b < EntropyBlockMessage::block_bytes; b++)
            line += to_string_hex(slot.data[b], 2);
    }

    line += "\nP " + to_string_dec_uint(pool_credit_mbits_ / 1000) +
            " " + password + "\n";
    return line;
}

}  // namespace ui::external_app::random_password
