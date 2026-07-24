/*
 * ui_sentinel_labkit.hpp — Sentinel labkit PortaPack views (GPLv2-or-later).
 *
 * A safety-hardened on-device port of the labkit-web control panel:
 *   - SentinelLabkitView : compose an EffectPlan, dry-run VALIDATE (the gate, no RF),
 *                          and — only when armed + attested + clear — EMIT.
 *   - BandsView          : the fixed GNSS denylist + reference ISM/control bands.
 *   - AttestView         : operator identity + the affirmation that arms real emission.
 *
 * All safety math lives in the portable, host-tested core (core/sl_*.hpp); this file is
 * only UI + radio glue. The TX pipeline mirrors the in-tree Signal Gen app, but
 * pre_emit_gate() (GNSS emitted-footprint block + attestation) is called BEFORE
 * transmitter_model.enable() — there is no path to key the radio that skips it.
 */
#ifndef SENTINEL_LABKIT_UI_HPP
#define SENTINEL_LABKIT_UI_HPP

#include "app_settings.hpp"
#include "radio_state.hpp"
#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui/ui_receiver.hpp"  // FrequencyField

#include "portapack.hpp"
#include "message.hpp"

#include <string>

#include "sl_gate.hpp"   // portable core: GNSS footprint gate + plan validity + attestation
#include "sl_power.hpp"  // portable core: power_dbm -> TX VGA gain

namespace ui::external_app::sentinel_labkit {

namespace core = ::sentinel_labkit;

// Shared, app-scoped attestation state (operator id + affirmation). One instance lives
// in the root view; AttestView edits it by reference.
struct AttestState {
    std::string operator_id{};
    bool affirmed{false};

    core::Attestation as_attestation() const {
        core::Attestation a;
        a.affirmed = affirmed;
        a.operator_id = operator_id.empty() ? nullptr : operator_id.c_str();
        return a;
    }
};

// --- GNSS denylist + reference bands (read-only reference view) ----------------------
class BandsView : public View {
   public:
    explicit BandsView(NavigationView& nav);
    void focus() override;
    std::string title() const override { return "GNSS / Bands"; }

   private:
    NavigationView& nav_;
    Console console{{0, 0, 240, 288}};
    Button button_done{{80, 292, 80, 24}, "Done"};
};

// --- Operator attestation (arms real emission) --------------------------------------
class AttestView : public View {
   public:
    AttestView(NavigationView& nav, AttestState& state);
    void focus() override;
    std::string title() const override { return "Attestation"; }

   private:
    NavigationView& nav_;
    AttestState& state_;

    Labels labels{
        {{0, 1 * 16}, "Operator affirmation.", Theme::getInstance()->fg_light->foreground},
        {{0, 8 * 16}, "Affirm procedures +", Theme::getInstance()->fg_light->foreground},
        {{0, 9 * 16}, "authorization in force.", Theme::getInstance()->fg_light->foreground}};

    // Operator id: a short symbol field the operator sets (initials / callsign).
    Text text_op{{0, 3 * 16, 240, 16}, "Operator: (set below)"};
    Button button_set_op{{0, 4 * 16, 120, 24}, "Set operator"};

    Checkbox checkbox_affirm{{0, 11 * 16}, 20, "I affirm (see docs)"};
    Button button_done{{80, 17 * 16, 80, 24}, "Done"};

    void refresh();
};

// --- Root: compose + dry-run validate + gated emit ----------------------------------
class SentinelLabkitView : public View {
   public:
    explicit SentinelLabkitView(NavigationView& nav);
    ~SentinelLabkitView();

    void focus() override;
    std::string title() const override { return "Sentinel Lab"; }

   private:
    NavigationView& nav_;

    // Preset the TX model: freq kept (0), 1.75 MHz baseband filter, 20 MS/s DAC rate
    // (matches labkit's default sample rate → the ± sample_rate/2 emitted footprint).
    TxRadioState radio_state_{0, 1750000, 20000000};
    app_settings::SettingsManager settings_{"tx_sentinel_labkit", app_settings::Mode::TX};

    AttestState attest_state_{};
    bool transmitting_{false};

    // Build an EffectPlan and the current sample rate (Hz) from the widget values.
    core::EffectPlan plan_from_fields() const;
    uint32_t sample_rate_hz() const;

    core::GateResult evaluate() const;   // pure dry-run of the gate for the current fields
    void update_verdict();               // run evaluate(), render to the console (no RF)
    void on_emit();                      // armed path: gate → configure radio → enable()
    void stop_tx();
    void configure_baseband_for_waveform();
    void on_tx_progress(uint32_t progress, bool done);

    // -- widgets --
    Labels labels{
        {{0, 0 * 16}, "Waveform:", Theme::getInstance()->fg_light->foreground},
        {{0, 1 * 16}, "Center:", Theme::getInstance()->fg_light->foreground},
        {{0, 2 * 16}, "BW MHz:", Theme::getInstance()->fg_light->foreground},
        {{15 * 8, 2 * 16}, "SR:", Theme::getInstance()->fg_light->foreground},
        {{0, 3 * 16}, "Pwr dBm:", Theme::getInstance()->fg_light->foreground},
        {{15 * 8, 3 * 16}, "Dur s:", Theme::getInstance()->fg_light->foreground}};

    OptionsField field_waveform{
        {9 * 8, 0 * 16}, 12,
        {{"CW", 0}, {"Band noise", 1}, {"Sweep", 2}}};

    FrequencyField field_center{{9 * 8, 1 * 16}};

    NumberField field_bw_mhz{{9 * 8, 2 * 16}, 2, {0, 60}, 1, ' ', false};

    // Model sampling rate → real occupied span (± rate/2). The GNSS guard uses THIS.
    OptionsField field_sr{
        {19 * 8, 2 * 16}, 6,
        {{"2M", 2000000}, {"8M", 8000000}, {"10M", 10000000},
         {"16M", 16000000}, {"20M", 20000000}}};

    NumberField field_power_dbm{{9 * 8, 3 * 16}, 3, {-40, 30}, 1, ' ', false};
    NumberField field_duration_s{{21 * 8, 3 * 16}, 2, {1, 60}, 1, ' ', false};

    Checkbox checkbox_arm{{0, 5 * 16}, 6, "Arm TX", false};
    Checkbox checkbox_owned{{12 * 8, 5 * 16}, 8, "Owned tgt", false};

    Console console{{0, 7 * 16, 240, 160}};

    Button button_validate{{0, 18 * 16, 112, 28}, "Validate"};
    Button button_emit{{120, 18 * 16, 112, 28}, "EMIT"};

    Button button_bands{{0, 20 * 16, 112, 24}, "GNSS/Bands"};
    Button button_attest{{120, 20 * 16, 112, 24}, "Attestation"};

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
            this->on_tx_progress(message.progress, message.done);
        }};
};

}  // namespace ui::external_app::sentinel_labkit

#endif  // SENTINEL_LABKIT_UI_HPP
