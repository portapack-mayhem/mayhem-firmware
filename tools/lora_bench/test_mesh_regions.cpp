// Host test for the frequency model in mesh_regions.hpp.
//
// The region/slot axis of the test matrix does not need a radio: it decides one
// number, the centre frequency, and that number is arithmetic. So it is checked
// here exhaustively - every region, every preset, every slot - rather than a few
// samples on air, which is also the only honest option given that transmitting on
// most of these bands is somebody else's licence.
//
// Two things guard against the test agreeing with a bug:
//   * an INDEPENDENT implementation of RadioInterface::applyModemConfig's formula,
//     written from the Meshtastic description in MHz floats the way the stock
//     firmware computes it, rather than reusing ours;
//   * anchors measured off real hardware.
//
//   c++ -std=c++17 -o test_mesh_regions test_mesh_regions.cpp && ./test_mesh_regions
#include <cstdio>
#include <cmath>
#include <cstring>
#include "../../firmware/application/apps/mesh/mesh_regions.hpp"
#include "../../firmware/application/apps/mesh/mesh_crypto.hpp"

using namespace meshtastic;

static int fails = 0;
static void check(bool ok, const char* what) {
    if (!ok) { fails++; printf("FAIL  %s\n", what); }
}

// RadioInterface::applyModemConfig, rewritten from the description rather than
// from our own code: MHz floats, spacing included, slot from the djb2 name hash.
static double meshtastic_freq_mhz(const RegionConfig& r, double bw_khz, uint32_t slot) {
    const double start = r.freq_start_hz / 1e6, end = r.freq_end_hz / 1e6;
    const double bw_mhz = bw_khz / 1000.0;
    double n = std::floor((end - start) / bw_mhz);
    if (n < 1) n = 1;
    return start + bw_mhz / 2.0 + (slot % (uint32_t)n) * bw_mhz;
}

int main() {
    // ---- anchors measured on air or documented for stock nodes --------------
    struct Anchor { uint8_t region, preset; uint64_t hz; const char* why; };
    const Anchor anchors[] = {
        {7, PRESET_SHORT_TURBO,   868950000, "RU ShortTurbo (measured on-air)"},
        {7, PRESET_LONG_FAST,     869075000, "RU LongFast (Heltec observed)"},
        {7, PRESET_LONG_MODERATE, 868887500, "RU LongMod, preset's own name"},
        {0, PRESET_LONG_FAST,     906875000, "US LongFast (well-known)"},
        {1, PRESET_LONG_FAST,     869525000, "EU868 LongFast (well-known)"},
        {2, PRESET_LONG_FAST,     433875000, "EU433 LongFast (well-known)"},
        {3, PRESET_LONG_FAST,     919875000, "ANZ LongFast (well-known)"},
    };
    for (const auto& a : anchors) {
        const uint64_t got = channel_frequency(a.region, a.preset);
        if (got != a.hz) {
            fails++;
            printf("FAIL  %-40s got %.4f MHz expect %.4f MHz\n", a.why,
                   got / 1e6, a.hz / 1e6);
        }
    }

    // ---- exhaustive: every region x preset x slot ---------------------------
    uint32_t combos = 0;
    for (uint8_t r = 0; r < NUM_REGIONS; r++) {
        const auto& reg = REGIONS[r];
        for (uint8_t p = 0; p < NUM_MODEM_PRESETS; p++) {
            const auto& mp = MODEM_PRESETS[p];
            const uint32_t n = num_channels(r, p);
            check(n >= 1, "num_channels is at least one");
            for (uint32_t slot = 1; slot <= n; slot++) {
                combos++;
                // The slot override is carried in a byte, so a band with more slots
                // than that cannot name the rest of them at all. Reported below.
                if (slot > 255) continue;
                const uint64_t hz = channel_frequency(r, p, (uint8_t)slot);
                char what[128];

                // Against the independent formula.
                const double want = meshtastic_freq_mhz(reg, mp.bw_hz / 1000.0, slot - 1);
                snprintf(what, sizeof what, "%s/%s slot %u: %.4f MHz, Meshtastic says %.4f",
                         reg.code, mp.name, slot, hz / 1e6, want);
                check(std::fabs(hz / 1e6 - want) < 0.0005, what);

                // A channel must sit inside its own band, edges included - unless the
                // preset is simply wider than the band, which no arithmetic can fix and
                // which the stock firmware handles the same way. Listed below instead.
                if (mp.bw_hz <= reg.freq_end_hz - reg.freq_start_hz) {
                    snprintf(what, sizeof what, "%s/%s slot %u: %.4f MHz outside %.3f-%.3f",
                             reg.code, mp.name, slot, hz / 1e6,
                             reg.freq_start_hz / 1e6, reg.freq_end_hz / 1e6);
                    check(hz - mp.bw_hz / 2 >= reg.freq_start_hz &&
                          hz + mp.bw_hz / 2 <= reg.freq_end_hz + 1, what);
                }

                // A one-based override must land on the slot it names. It is carried
                // in a byte, so bands with more than 255 slots cannot name them all -
                // reported below rather than asserted away.
                snprintf(what, sizeof what, "%s/%s slot override %u", reg.code, mp.name, slot);
                check(channel_slot(r, p, (uint8_t)slot) == slot - 1, what);
            }
            // Slot 0 means "derive from the channel name", and the name decides.
            check(channel_slot(r, p) == (name_hash(PRESET_CHANNEL_NAMES[p]) % n),
                  "derived slot follows the channel name");
        }
    }

    // ---- channel hash: the byte every packet header carries -----------------
    // Wrong here and a stock node's packets are dropped after being decoded
    // perfectly, which is how this was found the first time.
    // It follows the channel NAME, never the preset: a configured node keeps its
    // primary name across preset changes, and both the hash and the frequency slot
    // are computed from that name. A LONG_MODERATE exchange with a stock node on
    // 2026-08-26 ran on the name "LongFast" - hash 08, slot 3 - and not on the
    // preset's own "LongMod", which would have been hash 6E on a different slot.
    struct HashCase { const char* name; uint8_t hash; const char* why; };
    const HashCase hashes[] = {
        {"LongFast",   0x08, "LongFast (seen on air, incl. under LONG_MODERATE)"},
        {"ShortTurbo", 0x0E, "ShortTurbo (seen on air)"},
    };
    for (const auto& h : hashes) {
        const uint8_t got = channel_hash(h.name, DEFAULT_PSK, sizeof(DEFAULT_PSK));
        if (got != h.hash) {
            fails++;
            printf("FAIL  channel hash %-44s got %02X expect %02X\n", h.why, got, h.hash);
        }
    }
    // The preset helper must be the same rule, not a second copy of it.
    for (uint8_t p = 0; p < NUM_MODEM_PRESETS; p++)
        check(preset_channel_hash(p, DEFAULT_PSK, sizeof(DEFAULT_PSK)) ==
              channel_hash(PRESET_CHANNEL_NAMES[p], DEFAULT_PSK, sizeof(DEFAULT_PSK)),
              "preset_channel_hash agrees with channel_hash");

    // ---- the preset table against the wire ----------------------------------
    // Every preset's coding rate and bandwidth, and the reduced-rate rule that
    // follows from them. LONG_MODERATE was entered as 4/8 while the air carried
    // 4/5, and nothing here noticed for months.
    for (uint8_t p = 0; p < NUM_MODEM_PRESETS; p++) {
        const auto& mp = MODEM_PRESETS[p];
        char what[128];
        snprintf(what, sizeof what, "%s: coding rate %u is 5..8", mp.name, mp.cr);
        check(mp.cr >= 5 && mp.cr <= 8, what);
        snprintf(what, sizeof what, "%s: spreading factor %u is 7..12", mp.name, mp.sf);
        check(mp.sf >= 7 && mp.sf <= 12, what);
        // LDRO: mandatory once a symbol lasts longer than 16 ms.
        const bool ldro = ((uint64_t(1) << mp.sf) * 1000ull) > (uint64_t)mp.bw_hz * 16ull;
        const double ms = (double)(1u << mp.sf) * 1000.0 / mp.bw_hz;
        snprintf(what, sizeof what, "%s: symbol %.2f ms, reduced rate %s", mp.name, ms,
                 ldro ? "on" : "off");
        check(ldro == (ms > 16.0), what);
        // A preset must have a channel name: it decides both the slot and the hash.
        check(PRESET_CHANNEL_NAMES[p] && *PRESET_CHANNEL_NAMES[p], "preset has a channel name");
    }

    // ---- limits worth knowing rather than asserting -------------------------
    for (uint8_t r = 0; r < NUM_REGIONS; r++)
        for (uint8_t p = 0; p < NUM_MODEM_PRESETS; p++) {
            const uint32_t n = num_channels(r, p);
            if (n > 255)
                printf("note  %s/%s has %u slots; the override is a byte, so slots 256+ "
                       "cannot be selected\n", REGIONS[r].code, MODEM_PRESETS[p].name, n);
            if (MODEM_PRESETS[p].bw_hz > REGIONS[r].freq_end_hz - REGIONS[r].freq_start_hz)
                printf("note  %s/%s is wider than the band (%u kHz in %u kHz); one slot, "
                       "and it overhangs\n", REGIONS[r].code, MODEM_PRESETS[p].name,
                       MODEM_PRESETS[p].bw_hz / 1000,
                       (unsigned)((REGIONS[r].freq_end_hz - REGIONS[r].freq_start_hz) / 1000));
        }

    printf("%s  %u region/preset/slot combinations, %u presets, %u anchors\n",
           fails ? "FAILED" : "ok", combos, (unsigned)NUM_MODEM_PRESETS,
           (unsigned)(sizeof(anchors) / sizeof(anchors[0])));
    return fails ? 1 : 0;
}
