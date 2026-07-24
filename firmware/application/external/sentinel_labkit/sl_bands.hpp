/*
 * sl_bands.hpp — GNSS denylist + emitted-footprint guard (portable, header-only).
 *
 * The single hard boundary carried onto the PortaPack: a set of GNSS-protected
 * frequency spans the app must NEVER transmit into, with no override, plus the
 * emitted-footprint helper the guard is evaluated against. A direct-conversion
 * transmitter (HackRF/PortaPack) radiates across center +/- sample_rate/2 (plus LO
 * leakage, the IQ image, and DAC images) regardless of the plan bandwidth, so the
 * guard must inspect that footprint, not the plan window.
 *
 * This is a faithful C++ port of labkit/bands.py (GNSS_DENYLIST, emitted_span_mhz,
 * is_gnss, gnss_hit) — kept dependency-free so it compiles both in the Mayhem
 * firmware and in a host unit test. Frequencies in MHz throughout.
 */
#ifndef SENTINEL_LABKIT_SL_BANDS_HPP
#define SENTINEL_LABKIT_SL_BANDS_HPP

namespace sentinel_labkit {

struct FreqRange {
    float low_mhz;
    float high_mhz;
    const char* label;

    // Inclusive-overlap test (matches FreqRange.overlaps in labkit/bands.py).
    bool overlaps(float low, float high) const {
        return low <= high_mhz && high >= low_mhz;
    }
    bool contains(float low, float high) const {
        return low >= low_mhz && high <= high_mhz;
    }
};

// --- HARD GNSS DENYLIST (non-negotiable, no override) ------------------------
// Guard spans widened around each constellation's carriers to cover signal
// bandwidth and receiver front-ends. Mirrors labkit/bands.py::GNSS_DENYLIST.
static const FreqRange GNSS_DENYLIST[] = {
    {1155.0f, 1218.0f, "GNSS L5/E5/B2 (GPS L5 1176.45, Galileo E5, BeiDou B2)"},
    {1218.0f, 1310.0f, "GNSS L2/E6/B3 (GPS L2 1227.60, Galileo E6, BeiDou B3)"},
    {1358.0f, 1394.0f, "GNSS/RNSS L-band guard (GPS L3 1381.05 / L4 1379.9)"},
    {1525.0f, 1560.0f, "GNSS/SBAS + adjacent MSS guard"},
    {1559.0f, 1610.0f, "GNSS L1/E1/B1 (GPS L1 1575.42, GLONASS 1598-1606)"},
    {2483.5f, 2500.0f, "GNSS NavIC/IRNSS S-band (carrier 2492.028)"},
};
static const int GNSS_DENYLIST_COUNT =
    static_cast<int>(sizeof(GNSS_DENYLIST) / sizeof(GNSS_DENYLIST[0]));

// Reference ISM/control bands (NOT a grant of authority — the operative authorized
// list comes from the operator's FCC Part 5 experimental authorization). Mirrors
// labkit/bands.py::EXAMPLE_CONTROL_BANDS; shown in the on-device bands view.
struct NamedBand { const char* key; FreqRange range; };
static const NamedBand EXAMPLE_CONTROL_BANDS[] = {
    {"433_ism", {433.05f, 434.79f, "433 MHz ISM (EU LPD / ELRS-433)"}},
    {"915_ism", {902.0f, 928.0f, "902-928 MHz ISM (US, ELRS-900 / Crossfire)"}},
    {"868_ism", {863.0f, 870.0f, "863-870 MHz SRD (EU)"}},
    {"24_ism", {2400.0f, 2483.5f, "2.4 GHz ISM (ELRS-2.4 / OcuSync / WiFi ctl)"}},
    {"58_ism", {5725.0f, 5850.0f, "5.8 GHz ISM (video / OcuSync)"}},
};
static const int EXAMPLE_CONTROL_BANDS_COUNT =
    static_cast<int>(sizeof(EXAMPLE_CONTROL_BANDS) / sizeof(EXAMPLE_CONTROL_BANDS[0]));

// Default sample rate (Hz). A zero-IF transmitter radiates across its full sample-rate
// window; this is the reference span the guard protects when a rate is not otherwise
// known. Mirrors labkit/bands.py::DEFAULT_SAMPLE_RATE_HZ.
static const float DEFAULT_SAMPLE_RATE_HZ = 20000000.0f;

// Real occupied RF footprint of a zero-IF SDR: center +/- max(bw, sample_rate)/2.
// Mirrors labkit/bands.py::emitted_span_mhz.
inline void emitted_span_mhz(float center_mhz, float bandwidth_mhz,
                             float sample_rate_hz, float& low_out, float& high_out) {
    const float sr_mhz = sample_rate_hz / 1000000.0f;
    const float span = (bandwidth_mhz > sr_mhz) ? bandwidth_mhz : sr_mhz;
    const float half = span / 2.0f;
    low_out = center_mhz - half;
    high_out = center_mhz + half;
}

// True if [low, high] touches any GNSS-protected span. Mirrors is_gnss.
inline bool is_gnss(float low_mhz, float high_mhz) {
    for (int i = 0; i < GNSS_DENYLIST_COUNT; ++i) {
        if (GNSS_DENYLIST[i].overlaps(low_mhz, high_mhz)) return true;
    }
    return false;
}

// Label of the first GNSS span a range violates, or nullptr if clear. Mirrors
// gnss_hit (the on-device UI shows one label; the Python returns the full list).
inline const char* gnss_hit(float low_mhz, float high_mhz) {
    for (int i = 0; i < GNSS_DENYLIST_COUNT; ++i) {
        if (GNSS_DENYLIST[i].overlaps(low_mhz, high_mhz)) return GNSS_DENYLIST[i].label;
    }
    return nullptr;
}

}  // namespace sentinel_labkit

#endif  // SENTINEL_LABKIT_SL_BANDS_HPP
