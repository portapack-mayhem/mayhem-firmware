// Verifies the IEEE-754 bit-trick log2 used for RSSI in proc_lora.cpp
// (send_packet) matches true 10*log10 within a small tolerance. The firmware
// avoids log10f because it drags in ~1.7 KB of libm and the flash is full.
//
// Build:  c++ -std=c++17 -O2 -o test_lora_rssi test_lora_rssi.cpp
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cmath>

// Copy of the firmware computation (proc_lora.cpp send_packet).
static int rssi_dbm(float ref_peak_mag) {
    if (!(ref_peak_mag > 1.0f)) return 0;
    uint32_t bits;
    std::memcpy(&bits, &ref_peak_mag, sizeof(bits));
    const int exp = static_cast<int>((bits >> 23) & 0xFF) - 127;
    uint32_t mbits = (bits & 0x7FFFFF) | 0x3F800000;
    float m;
    std::memcpy(&m, &mbits, sizeof(m));
    const float log2v = static_cast<float>(exp) + (m - 1.0f);
    float dbm = 3.0103f * log2v - 95.0f;
    if (dbm > 0.0f) dbm = 0.0f;
    if (dbm < -120.0f) dbm = -120.0f;
    return static_cast<int>(static_cast<signed char>(dbm));
}

int main() {
    float worst = 0.0f;
    // Sweep peak-power values across the decodable range (~1e2..1e9).
    for (double p = 1e2; p <= 1e9; p *= 1.07) {
        const int ours = rssi_dbm(static_cast<float>(p));
        double truev = 10.0 * std::log10(p) - 95.0;
        if (truev > 0) truev = 0;
        if (truev < -120) truev = -120;
        const float err = std::fabs(ours - truev);
        if (err > worst) worst = err;
    }
    printf("worst error vs 10*log10: %.3f dB\n", worst);
    // Monotonic + range sanity at a few anchors.
    const bool mono = rssi_dbm(1e9f) > rssi_dbm(1e6f) &&
                      rssi_dbm(1e6f) > rssi_dbm(1e3f);
    const bool range = rssi_dbm(1e9f) <= 0 && rssi_dbm(1e2f) < -60;
    const bool pass = worst < 1.0f && mono && range;
    printf("monotonic=%d range=%d  ->  %s\n", mono, range, pass ? "PASS" : "FAIL");
    return pass ? 0 : 1;
}
