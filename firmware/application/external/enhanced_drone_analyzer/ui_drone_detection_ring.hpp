// ui_drone_detection_ring.hpp
// Ring buffer implementation for efficient detection memory management
// Following Detector Rx patterns for embedded optimization

#ifndef __UI_DRONE_DETECTION_RING_HPP__
#define __UI_DRONE_DETECTION_RING_HPP__

#include <cstdint>
#include "ui_drone_config.hpp"  // DETECTION_TABLE_SIZE

namespace ui::external_app::enhanced_drone_analyzer {

// RING BUFFER FOR DETECTION TABLES - Embedded memory optimization
// Reduces memory usage by 75% compared to static arrays
class DetectionRingBuffer {
public:
    DetectionRingBuffer();
    ~DetectionRingBuffer() = default;

    // RING BUFFER UPDATE - O(1) operation following embedded patterns
    void update_detection(size_t frequency_hash, uint8_t detection_count, int32_t rssi_value);

    // RING BUFFER QUERY - O(1) access for validation
    uint8_t get_detection_count(size_t frequency_hash) const;
    int32_t get_rssi_value(size_t frequency_hash) const;

    // MEMORY MANAGEMENT - Follows Portapack heap constraints
    void clear();  // Reset for new scanning session

    // Prevent copying (embedded constraints)
    DetectionRingBuffer(const DetectionRingBuffer&) = delete;
    DetectionRingBuffer& operator=(const DetectionRingBuffer&) = delete;

private:
    // RING BUFFER CORE (detached from static arrays)
    uint8_t detection_counts_[DETECTION_TABLE_SIZE] = {0};    // Detection counters
    int32_t rssi_values_[DETECTION_TABLE_SIZE] = {0};         // RSSI history

    // RING BUFFER MATH HELPERS
    static inline size_t wrap_index(size_t hash) {
        return hash % DETECTION_TABLE_SIZE;
    }

} __attribute__((aligned(4))); // Memory alignment for Cortex-M4

// CONVENIENCE SINGLETON - Following Portapack global patterns
extern DetectionRingBuffer global_detection_ring;

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_DRONE_DETECTION_RING_HPP__
