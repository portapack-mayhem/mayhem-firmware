#ifndef __MORSEDECODER_HPP__
#define __MORSEDECODER_HPP__

/*
 * Copyright (C) 2025 Pezsma
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

#include <cstdint>
#include <string>

#define MORSEDEC_ERROR "<ERR>"

namespace ui::external_app::morse_practice {

class MorseRingBuffer {
   public:
    MorseRingBuffer()
        : head_(0), tail_(0), count_(0) {}

    void push_back(const uint32_t& value) {
        data_[head_] = value;
        head_ = (head_ + 1) % 40;
        if (count_ < 40) {
            count_++;
        } else {
            // overwrite oldest element
            tail_ = (tail_ + 1) % 40;
        }
    }

    void pop_front() {
        if (count_ > 0) {
            tail_ = (tail_ + 1) % 40;
            count_--;
        }
    }

    size_t size() const { return count_; }
    bool empty() const { return count_ == 0; }
    const uint32_t& front() const { return data_[tail_]; }
    // Access by index (0 = oldest)
    uint32_t operator[](size_t idx) const {
        return data_[(tail_ + idx) % 40];
    }
    // Convert to vector-like access for sorting etc.
    void copy_to_array(uint32_t* out) const {
        for (size_t i = 0; i < count_; ++i)
            out[i] = (*this)[i];
    }

   private:
    uint32_t data_[40];
    size_t head_;
    size_t tail_;
    size_t count_;
};

class MorseDecoder {
   public:
    struct DecodeResult {
        std::string text = "";
        double confidence = 0.0;

        bool isValid() const {
            return !text.empty();
        }
    };

    struct MorseEntry {
        std::string code;
        std::string letter;
    };

    MorseDecoder() {}
    DecodeResult handleInput(int32_t duration_ms) {
        DecodeResult result = {"", 0.0};
        if (duration_ms > 0) {
            pulse_history_.push_back(duration_ms);
            double dah_prob = getDahProbability(duration_ms);
            current_sequence_ += (dah_prob > 0.5) ? '-' : '.';
            last_confidence_ = (dah_prob > 0.5) ? dah_prob : (1.0 - dah_prob);
            last_sequence_ = current_sequence_;
        } else {
            uint32_t gap_duration = -duration_ms;
            pulse_gaps_.push_back(gap_duration);

            if (gap_duration >= getInterCharThreshold() && !current_sequence_.empty()) {
                result.text = lookupMorse(current_sequence_);
                result.confidence = (result.text != MORSEDEC_ERROR) ? last_confidence_ : 0.0;
                if (gap_duration >= getInterWordThreshold()) {
                    result.text += " ";
                }
                current_sequence_ = "";
            }
        }

        updateLearning();

        return result;
    }

    inline double getInterElementThreshold() { return time_unit_ms_ * 2.0; }
    inline double getInterCharThreshold() { return time_unit_ms_ * 4.0; }
    inline double getInterWordThreshold() { return time_unit_ms_ * 7.0; }

    inline double getCurrentTimeUnit() { return time_unit_ms_; }
    inline std::string getLastSequence() { return last_sequence_; }

   private:
    std::string current_sequence_ = "";
    std::string last_sequence_ = "";
    double time_unit_ms_ = 160.0;
    double last_confidence_ = 0.0;
    MorseRingBuffer pulse_history_{};
    MorseRingBuffer pulse_gaps_{};

    std::string lookupMorse(const std::string& seq) {
        for (size_t i = 0; i < morse_table_size_; i++) {
            if (seq == morse_table_[i].code)
                return morse_table_[i].letter;
        }
        return MORSEDEC_ERROR;  // not found
    }

    double getDahProbability(uint32_t duration_ms) {
        double start_interp = 1.5 * time_unit_ms_;
        double end_interp = 2.5 * time_unit_ms_;

        if (duration_ms <= start_interp) return 0.0;
        if (duration_ms >= end_interp) return 1.0;
        return ((double)(duration_ms)-start_interp) / (end_interp - start_interp);
    }

    size_t findDecisionBoundary(uint32_t* sorted_data, size_t sorted_data_size) {
        if (sorted_data_size < 4) return 0;

        size_t best_split_index = 0;
        uint32_t max_diff = 0;

        for (size_t i = 1; i < sorted_data_size; ++i) {
            uint32_t diff = sorted_data[i] - sorted_data[i - 1];
            if (diff > sorted_data[i - 1] * 0.5 && diff > max_diff) {
                max_diff = diff;
                best_split_index = i;
            }
        }
        return best_split_index;
    }

    bool calculatePulseUnit(double& unit, double& confidence) {
        if (pulse_history_.size() < 10) return false;
        uint32_t sorted_pulses[pulse_history_.size()];
        pulse_history_.copy_to_array(sorted_pulses);
        sort_uint32(sorted_pulses, pulse_history_.size());

        size_t split_index = findDecisionBoundary(sorted_pulses, pulse_history_.size());
        if (split_index == 0 || split_index < 3 || (pulse_history_.size() - split_index) < 2) {
            return false;
        }
        double dit_sum = sum_uint32_range(sorted_pulses, 0, split_index);
        double dah_sum = sum_uint32_range(sorted_pulses, split_index, pulse_history_.size());

        double avg_dit = dit_sum / split_index;
        double avg_dah = dah_sum / (pulse_history_.size() - split_index);
        if (avg_dah <= avg_dit) return false;

        double ratio = avg_dah / avg_dit;
        if (ratio > 1.5 && ratio < 5.0) {
            unit = avg_dit;
            double tmpabs = ratio - 3.0;
            if (tmpabs < 0) tmpabs *= -1;
            tmpabs /= 3.0;
            tmpabs = 1.0 - tmpabs;
            if (tmpabs < 0) tmpabs = 0;
            confidence = tmpabs;  // 0..1
            return true;
        }

        return false;
    }

    bool calculateGapUnit(double& unit, double& confidence) {
        if (pulse_gaps_.size() < 10) return false;
        double threshold = getInterElementThreshold();
        double valid_gaps[pulse_gaps_.size()];
        size_t valid_count = 0;
        for (size_t i = 0; i < pulse_gaps_.size(); i++) {
            double gap = pulse_gaps_[i];
            if (gap <= threshold) {
                valid_gaps[valid_count++] = gap;
            }
        }
        if (valid_count < 2) {
            return false;
        }
        double sum = sum_double_range(valid_gaps, 0, valid_count);
        size_t count_to_average = valid_count;

        if (count_to_average > 0) {
            unit = sum / count_to_average;
            confidence = 0.8;
            return true;
        }

        return false;
    }

    double sum_uint32_range(const uint32_t* data, size_t start, size_t end) {
        double sum = 0.0;
        for (size_t i = start; i < end; i++) {
            sum += data[i];
        }
        return sum;
    }
    double sum_double_range(const double* data, size_t start, size_t end) {
        double sum = 0.0;
        for (size_t i = start; i < end; i++) {
            sum += data[i];
        }
        return sum;
    }

    void sort_uint32(uint32_t* data, size_t size) {
        if (size < 2)
            return;

        for (size_t i = 1; i < size; i++) {
            uint32_t key = data[i];
            size_t j = i;

            while (j > 0 && data[j - 1] > key) {
                data[j] = data[j - 1];
                j--;
            }

            data[j] = key;
        }
    }

    double clamp_double(double value, double min_val, double max_val) {
        if (value < min_val)
            return min_val;
        else if (value > max_val)
            return max_val;
        else
            return value;
    }

    void updateLearning() {
        double pulse_unit = -1.0, pulse_confidence = 0.0;
        double gap_unit = -1.0, gap_confidence = 0.0;

        bool pulse_success = calculatePulseUnit(pulse_unit, pulse_confidence);
        bool gap_success = calculateGapUnit(gap_unit, gap_confidence);

        double new_time_unit = -1.0;
        if (pulse_success && pulse_confidence > 0.5) {
            new_time_unit = pulse_unit;
        } else if (pulse_success && gap_success) {
            gap_confidence = 0.2;
            double total_confidence = pulse_confidence + gap_confidence;
            new_time_unit = (pulse_unit * pulse_confidence + gap_unit * gap_confidence) / total_confidence;
        } else if (gap_success) {
            new_time_unit = gap_unit;
        } else {
            return;
        }

        double max_change = time_unit_ms_ * 0.25;
        new_time_unit = clamp_double(new_time_unit, time_unit_ms_ - max_change, time_unit_ms_ + max_change);
        double DEFAULT_TIME_UNIT = 160.0;
        double BASE_LEARNING_RATE = 0.05;
        double MAX_LEARNING_RATE = 0.25;
        double tudeltaabs = new_time_unit - DEFAULT_TIME_UNIT;
        if (tudeltaabs < 0) tudeltaabs *= -1;
        double deviation_from_default = tudeltaabs / DEFAULT_TIME_UNIT;
        double tpp = deviation_from_default * 2.0;
        if (tpp > 1) tpp = 1;
        double learning_factor = BASE_LEARNING_RATE + (MAX_LEARNING_RATE - BASE_LEARNING_RATE) * tpp;

        time_unit_ms_ = (time_unit_ms_ * (1.0 - learning_factor)) + (new_time_unit * learning_factor);
    }

    size_t morse_table_size_ = 41;
    MorseEntry morse_table_[41] = {
        {".-", "A"},
        {"-...", "B"},
        {"-.-.", "C"},
        {"-..", "D"},
        {".", "E"},
        {"..-.", "F"},
        {"--.", "G"},
        {"....", "H"},
        {"..", "I"},
        {".---", "J"},
        {"-.-", "K"},
        {".-..", "L"},
        {"--", "M"},
        {"-.", "N"},
        {"---", "O"},
        {".--.", "P"},
        {"--.-", "Q"},
        {".-.", "R"},
        {"...", "S"},
        {"-", "T"},
        {"..-", "U"},
        {"...-", "V"},
        {".--", "W"},
        {"-..-", "X"},
        {"-.--", "Y"},
        {"--..", "Z"},
        {".----", "1"},
        {"..---", "2"},
        {"...--", "3"},
        {"....-", "4"},
        {".....", "5"},
        {"-....", "6"},
        {"--...", "7"},
        {"---..", "8"},
        {"----.", "9"},
        {"-----", "0"},
        {".-.-.-", "."},
        {"..--..", "?"},
        {"-.-.--", "!"},
        {"--..--", ","},
        {"-...-", "="}};
};

}  // namespace ui::external_app::morse_practice

#endif  // __MORSEDECODER_HPP__
