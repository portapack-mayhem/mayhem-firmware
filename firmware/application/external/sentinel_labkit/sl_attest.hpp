/*
 * sl_attest.hpp — operator attestation (portable, header-only).
 *
 * The only in-code precondition before a real emission besides the GNSS block: the
 * operator affirms that their external safety/compliance procedures and the governing
 * FCC authorization are being followed. Faithful C++ port of labkit/attest.py — an
 * attestation is valid when it is affirmed AND carries a non-empty operator identity.
 */
#ifndef SENTINEL_LABKIT_SL_ATTEST_HPP
#define SENTINEL_LABKIT_SL_ATTEST_HPP

namespace sentinel_labkit {

// Shown in the on-device AttestView; may be unused in a given translation unit.
[[maybe_unused]] static const char* ATTEST_STATEMENT =
    "I affirm that all applicable safety and compliance procedures, and the governing "
    "FCC authorization, are being followed for this test.";

struct Attestation {
    bool affirmed = false;
    const char* operator_id = nullptr;   // points to a UI/config-owned string

    // Mirrors Attestation.valid(): affirmed and a non-empty operator.
    bool valid() const {
        return affirmed && operator_id != nullptr && operator_id[0] != '\0';
    }
};

}  // namespace sentinel_labkit

#endif  // SENTINEL_LABKIT_SL_ATTEST_HPP
