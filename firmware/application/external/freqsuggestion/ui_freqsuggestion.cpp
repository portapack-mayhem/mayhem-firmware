/*
 * Copyright (C) 2026
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

#include "ui_freqsuggestion.hpp"
#include "ch.h"
#include "convert.hpp"
#include "event_m0.hpp"
#include "file_reader.hpp"
#include "portapack.hpp"
#include "file_path.hpp"
#include "receiver_model.hpp"
#include "string_format.hpp"
#include "freqman_db.hpp"
#include <cstring>

using namespace portapack;
using namespace ui;

// Use existing freqman modulations from freqman_db.cpp
extern std::vector<std::pair<std::string_view, int32_t>> freqman_modulations;

namespace ui::external_app::freqsuggestion {

void FreqSuggestionView::focus() {
    field_frequency.focus();
}

std::string FreqSuggestionView::modulation_index_to_string(freqman_index_t mode) {
    // Reuse existing freqman modulation names
    if (is_valid(mode) && mode < freqman_modulations.size()) {
        return std::string{freqman_modulations[mode].first};
    }

    // Special cases for modes not in standard freqman list
    if (mode == 6) return "USB";
    if (mode == 7) return "LSB";
    if (mode == 8) return "DSB";
    if (mode == 9) return "DIGITAL";
    if (mode == 10) return "MULTI";

    return "UNKNOWN";
}

std::string FreqSuggestionView::format_bandwidth(uint32_t bw_min, uint32_t bw_max) {
    if (bw_min == bw_max) {
        if (bw_min >= 1000000) {
            return to_string_dec_uint(bw_min / 1000000, 1) + " MHz";
        } else if (bw_min >= 1000) {
            return to_string_dec_uint(bw_min / 1000, 1) + " kHz";
        } else {
            return to_string_dec_uint(bw_min, 1) + " Hz";
        }
    } else {
        std::string result;
        if (bw_min >= 1000000) {
            result = to_string_dec_uint(bw_min / 1000000, 1) + "-";
            result += to_string_dec_uint(bw_max / 1000000, 1) + " MHz";
        } else if (bw_min >= 1000) {
            result = to_string_dec_uint(bw_min / 1000, 1) + "-";
            result += to_string_dec_uint(bw_max / 1000, 1) + " kHz";
        } else {
            result = to_string_dec_uint(bw_min, 1) + "-";
            result += to_string_dec_uint(bw_max, 1) + " Hz";
        }
        return result;
    }
}

std::string FreqSuggestionView::format_frequency_range(rf::Frequency start, rf::Frequency end) {
    if (start >= 1000000000) {
        return to_string_dec_uint(start / 1000000, 1) + "-" + to_string_dec_uint(end / 1000000, 1) + " MHz";
    } else {
        return to_string_dec_uint(start / 1000, 1) + "-" + to_string_dec_uint(end / 1000, 1) + " kHz";
    }
}

FrequencyBand* FreqSuggestionView::find_band(rf::Frequency freq) {
    for (auto& band : frequency_db) {
        if (freq >= band.freq_start && freq <= band.freq_end) {
            return &band;
        }
    }
    return nullptr;
}

AntennaRecommendation FreqSuggestionView::get_antenna_recommendation(rf::Frequency freq, const std::string& antenna_name) {
    AntennaRecommendation rec;
    rec.antenna_name = antenna_name;

    // Calculate wavelength and optimal antenna properties
    double wavelength_m = 299792458.0 / static_cast<double>(freq);
    double quarter_wave_mm = (wavelength_m / 4.0) * 1000.0;

    // Basic recommendation based on frequency range
    if (freq < 50000000) {  // < 50 MHz
        rec.recommendation = "Long antenna recommended";
    } else if (freq < 150000000) {  // 50-150 MHz
        rec.recommendation = "Full-size antenna ideal";
    } else if (freq < 500000000) {  // 150-500 MHz
        rec.recommendation = "Standard antenna suitable";
    } else if (freq < 1000000000) {  // 500 MHz - 1 GHz
        rec.recommendation = "Compact antenna works";
    } else {  // > 1 GHz
        rec.recommendation = "Short antenna adequate";
    }

    rec.recommendation += " (~" + to_string_dec_uint(static_cast<uint32_t>(quarter_wave_mm), 0) + "mm λ/4)";

    return rec;
}

void FreqSuggestionView::update_suggestions() {
    console.clear(true);
    rf::Frequency freq = field_frequency.value();

    FrequencyBand* band = find_band(freq);

    if (band == nullptr) {
        text_band_name.set("Unknown band");
        text_region.set("-");
        text_usage.set("-");
        text_demod_mode.set("Try SPEC/NFM");
        text_bandwidth.set("Variable");
        text_gains.set("20 / 30");
        text_rf_amp.set("OFF");

        console.write("No specific band data.\n");
        console.write("Try spectrum mode to\n");
        console.write("identify signal type.\n");
        console.write("\n");
        console.write("General starting points:\n");
        console.write("- Voice: NFM, 12.5kHz BW\n");
        console.write("- Broadcast: WFM, 200kHz\n");
        console.write("- Digital: NFM, 25kHz\n");
        return;
    }

    // Update all fields with band information
    text_band_name.set(band->band_name.length() > 17 ? band->band_name.substr(0, 17) : band->band_name);
    text_region.set(band->region);
    text_usage.set(band->usage.length() > 17 ? band->usage.substr(0, 17) : band->usage);

    // Demod mode
    std::string demod_str = modulation_index_to_string(band->primary_mode);
    if (band->secondary_mode != 10 && band->secondary_mode != band->primary_mode) {
        demod_str += "/" + modulation_index_to_string(band->secondary_mode);
    }
    text_demod_mode.set(demod_str.length() > 17 ? demod_str.substr(0, 17) : demod_str);

    // Bandwidth
    text_bandwidth.set(format_bandwidth(band->suggested_bw_min, band->suggested_bw_max));

    // Gains
    std::string gains = to_string_dec_uint(band->suggested_lna, 2) + " / " + to_string_dec_uint(band->suggested_vga, 2);
    text_gains.set(gains);

    // RF Amp
    text_rf_amp.set(band->rf_amp_suggested ? "ON" : "OFF");

    // Detailed info in console
    console.write("Frequency Range:\n");
    console.write(format_frequency_range(band->freq_start, band->freq_end) + "\n\n");

    console.write("Description:\n");
    console.write(band->description + "\n\n");

    // Usage recommendations
    console.write("Recommended Settings:\n");
    console.write("Mode: " + modulation_index_to_string(band->primary_mode));
    if (band->secondary_mode != 10 && band->secondary_mode != band->primary_mode) {
        console.write(" or " + modulation_index_to_string(band->secondary_mode));
    }
    console.write("\n");

    console.write("BW: " + format_bandwidth(band->suggested_bw_min, band->suggested_bw_max) + "\n");
    console.write("LNA: " + to_string_dec_uint(band->suggested_lna, 2));
    console.write(", VGA: " + to_string_dec_uint(band->suggested_vga, 2));
    console.write(", AMP: " + std::string(band->rf_amp_suggested ? "ON" : "OFF") + "\n\n");

    // Antenna recommendations
    if (!antenna_names.empty()) {
        console.write("Antenna Suggestions:\n");
        for (const auto& ant_name : antenna_names) {
            auto rec = get_antenna_recommendation(freq, ant_name);
            console.write(ant_name + ": " + rec.recommendation + "\n");
        }
    }
}

FreqSuggestionView::FreqSuggestionView(NavigationView& nav)
    : nav_{nav} {
    add_children({&labels,
                  &field_frequency,
                  &text_band_name,
                  &text_region,
                  &text_usage,
                  &text_demod_mode,
                  &text_bandwidth,
                  &text_gains,
                  &text_rf_amp,
                  &labels_info,
                  &console,
                  &button_copy_settings,
                  &button_exit});

    // Load frequency database
    load_frequency_db();

    // Load antenna list
    load_antenna_list();

    // If no bands loaded, add defaults
    if (frequency_db.empty()) {
        add_default_bands();
    }

    // Set initial frequency to current receiver frequency if available
    field_frequency.set_value(receiver_model.target_frequency());

    field_frequency.on_change = [this](rf::Frequency) {
        update_suggestions();
    };

    button_copy_settings.on_select = [this, &nav](Button&) {
        // Apply suggested settings to receiver
        FrequencyBand* band = find_band(field_frequency.value());
        if (band != nullptr) {
            // Note: In a real implementation, we would apply these settings
            // to the receiver model. For now, just show confirmation.
            console.write("\nSettings would be applied\n");
            console.write("to the receiver.\n");
            console.write("(Feature in development)\n");
        }
    };

    button_exit.on_select = [this, &nav](Button&) {
        nav.pop();
    };

    update_suggestions();
}

void FreqSuggestionView::load_frequency_db() {
    File freq_file;
    // Try to load from FREQMAN directory (reuse existing directory)
    auto error = freq_file.open(u"FREQMAN/FREQSUGGESTIONS.TXT");

    if (error) {
        return;
    }

    auto reader = FileLineReader(freq_file);
    for (const auto& line : reader) {
        if (line.length() == 0 || line[0] == '#')
            continue;  // Empty or comment line

        // Format: freq_start,freq_end,band_name,region,usage,primary_mode,secondary_mode,bw_min,bw_max,lna,vga,rf_amp,description
        auto cols = split_string(line, ',');
        if (cols.size() < 13)
            continue;  // Not enough columns

        FrequencyBand band;

        // Parse frequencies
        uint64_t start_freq = 0, end_freq = 0;
        if (!parse_int(cols[0], start_freq) || !parse_int(cols[1], end_freq))
            continue;

        band.freq_start = start_freq;
        band.freq_end = end_freq;
        band.band_name = std::string{cols[2]};
        band.region = std::string{cols[3]};
        band.usage = std::string{cols[4]};

        // Parse demod modes - reuse freqman modulation indexes
        auto mode_str = std::string{cols[5]};
        if (mode_str == "AM")
            band.primary_mode = 0;  // AM index in freqman_modulations
        else if (mode_str == "NFM")
            band.primary_mode = 1;  // NFM index
        else if (mode_str == "WFM")
            band.primary_mode = 2;  // WFM index
        else if (mode_str == "USB")
            band.primary_mode = 6;  // Extended mode
        else if (mode_str == "LSB")
            band.primary_mode = 7;  // Extended mode
        else if (mode_str == "DIGITAL")
            band.primary_mode = 9;  // Extended mode
        else
            band.primary_mode = 3;  // SPEC index

        auto sec_mode_str = std::string{cols[6]};
        if (sec_mode_str == "AM")
            band.secondary_mode = 0;
        else if (sec_mode_str == "NFM")
            band.secondary_mode = 1;
        else if (sec_mode_str == "WFM")
            band.secondary_mode = 2;
        else if (sec_mode_str == "USB")
            band.secondary_mode = 6;
        else if (sec_mode_str == "LSB")
            band.secondary_mode = 7;
        else if (sec_mode_str == "DIGITAL")
            band.secondary_mode = 9;
        else
            band.secondary_mode = 10;  // MULTI

        // Parse bandwidths
        uint32_t bw_min = 0, bw_max = 0;
        if (parse_int(cols[7], bw_min) && parse_int(cols[8], bw_max)) {
            band.suggested_bw_min = bw_min;
            band.suggested_bw_max = bw_max;
        }

        // Parse gains
        uint32_t lna = 0, vga = 0;
        if (parse_int(cols[9], lna) && parse_int(cols[10], vga)) {
            band.suggested_lna = static_cast<uint8_t>(lna);
            band.suggested_vga = static_cast<uint8_t>(vga);
        }

        // Parse RF amp
        band.rf_amp_suggested = (cols[11] == "1" || cols[11] == "ON" || cols[11] == "on");

        // Description (rest of line, may contain commas)
        band.description = std::string{cols[12]};
        for (size_t i = 13; i < cols.size(); i++) {
            band.description += "," + std::string{cols[i]};
        }

        frequency_db.push_back(band);
    }
}

void FreqSuggestionView::load_antenna_list() {
    File antenna_file;
    auto error = antenna_file.open(whipcalc_dir / u"ANTENNAS.TXT");

    if (error) {
        // Add some default antennas
        antenna_names.push_back("ANT500");
        antenna_names.push_back("ANT700");
        return;
    }

    auto reader = FileLineReader(antenna_file);
    for (const auto& line : reader) {
        if (line.length() == 0 || line[0] == '#')
            continue;

        auto cols = split_string(line, ',');
        if (cols.size() >= 2) {
            antenna_names.push_back(std::string{cols[0]});
        }
    }

    if (antenna_names.empty()) {
        antenna_names.push_back("ANT500");
        antenna_names.push_back("ANT700");
    }
}

void FreqSuggestionView::add_default_bands() {
    // Add some essential default bands if database file not found

    // AM Broadcast (reusing freqman index: 0=AM, 10=MULTI)
    frequency_db.push_back({530000,   // 530 kHz
                            1700000,  // 1.7 MHz
                            "AM Broadcast",
                            "GLOBAL",
                            "AM Radio",
                            0,      // AM
                            10,     // MULTI
                            5000,   // 5 kHz min
                            10000,  // 10 kHz max
                            24,     // LNA
                            30,     // VGA
                            false,  // RF Amp
                            "Medium wave AM broadcasting"});

    // FM Broadcast (2=WFM)
    frequency_db.push_back({88000000,   // 88 MHz
                            108000000,  // 108 MHz
                            "FM Broadcast",
                            "GLOBAL",
                            "FM Radio",
                            2,       // WFM
                            10,      // MULTI
                            180000,  // 180 kHz
                            200000,  // 200 kHz
                            16,      // LNA
                            20,      // VGA
                            false,   // RF Amp
                            "VHF FM broadcasting, stereo audio"});

    // 2m Amateur Band (1=NFM, 6=USB)
    frequency_db.push_back({144000000,  // 144 MHz
                            148000000,  // 148 MHz
                            "2m Ham",
                            "GLOBAL",
                            "Amateur",
                            1,      // NFM
                            6,      // USB
                            12500,  // 12.5 kHz
                            25000,  // 25 kHz
                            24,     // LNA
                            30,     // VGA
                            false,  // RF Amp
                            "2 meter amateur radio band, voice and digital"});

    // Aircraft Band (0=AM)
    frequency_db.push_back({118000000,  // 118 MHz
                            137000000,  // 137 MHz
                            "Airband",
                            "GLOBAL",
                            "Aviation",
                            0,      // AM
                            10,     // MULTI
                            8333,   // 8.33 kHz
                            25000,  // 25 kHz
                            24,     // LNA
                            30,     // VGA
                            false,  // RF Amp
                            "Civil aviation communications"});

    // 70cm Amateur Band (1=NFM, 9=DIGITAL)
    frequency_db.push_back({420000000,  // 420 MHz
                            450000000,  // 450 MHz
                            "70cm Ham",
                            "US",
                            "Amateur",
                            1,      // NFM
                            9,      // DIGITAL
                            12500,  // 12.5 kHz
                            25000,  // 25 kHz
                            24,     // LNA
                            30,     // VGA
                            false,  // RF Amp
                            "70 centimeter amateur radio band"});

    // PMR446 (EU)
    frequency_db.push_back({446000000,  // 446.00625 MHz
                            446200000,  // 446.19375 MHz
                            "PMR446",
                            "EU",
                            "PMR Radio",
                            1,      // NFM
                            10,     // MULTI
                            12500,  // 12.5 kHz
                            12500,  // 12.5 kHz
                            24,     // LNA
                            30,     // VGA
                            false,  // RF Amp
                            "Personal Mobile Radio, license-free EU"});

    // FRS/GMRS (US)
    frequency_db.push_back({462000000,  // 462 MHz
                            467000000,  // 467 MHz
                            "FRS/GMRS",
                            "US",
                            "Two-way",
                            1,      // NFM
                            10,     // MULTI
                            12500,  // 12.5 kHz
                            25000,  // 25 kHz
                            24,     // LNA
                            30,     // VGA
                            false,  // RF Amp
                            "Family Radio Service / General Mobile Radio Service"});

    // ISM 433 MHz
    frequency_db.push_back({433050000,  // 433.05 MHz
                            434790000,  // 434.79 MHz
                            "ISM 433",
                            "EU",
                            "ISM/IoT",
                            1,       // NFM
                            9,       // DIGITAL
                            25000,   // 25 kHz
                            200000,  // 200 kHz
                            32,      // LNA
                            40,      // VGA
                            false,   // RF Amp
                            "ISM band, remote controls, IoT devices"});

    // ISM 868 MHz (EU)
    frequency_db.push_back({863000000,  // 863 MHz
                            870000000,  // 870 MHz
                            "ISM 868",
                            "EU",
                            "ISM/IoT",
                            1,       // NFM
                            9,       // DIGITAL
                            25000,   // 25 kHz
                            200000,  // 200 kHz
                            24,      // LNA
                            30,      // VGA
                            false,   // RF Amp
                            "ISM band, smart meters, LoRa, IoT"});

    // ISM 915 MHz (US)
    frequency_db.push_back({902000000,  // 902 MHz
                            928000000,  // 928 MHz
                            "ISM 915",
                            "US",
                            "ISM/IoT",
                            1,       // NFM
                            9,       // DIGITAL
                            25000,   // 25 kHz
                            500000,  // 500 kHz
                            24,      // LNA
                            30,      // VGA
                            false,   // RF Amp
                            "ISM band, LoRa, ZigBee, RFID"});

    // GPS L1 (3=SPEC)
    frequency_db.push_back({1575000000,  // 1575.42 MHz
                            1576000000,  // 1576 MHz
                            "GPS L1",
                            "GLOBAL",
                            "GNSS",
                            3,        // SPEC
                            9,        // DIGITAL
                            2000000,  // 2 MHz
                            4000000,  // 4 MHz
                            32,       // LNA
                            40,       // VGA
                            true,     // RF Amp
                            "GPS L1 C/A signal, GNSS"});
}

}  // namespace ui::external_app::freqsuggestion
