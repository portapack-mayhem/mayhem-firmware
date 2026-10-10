# Copilot Code Review Instructions for PortaPack Mayhem

This document guides GitHub Copilot's review of pull requests to the PortaPack Mayhem firmware project.

## Project Philosophy

PortaPack Mayhem is a **minimal-change, hardware-constrained embedded firmware** project. Code reviews must prioritize:
- **Surgical precision**: Minimal, focused changes
- **Hardware realism**: Respect for extreme memory/CPU constraints
- **Maintainability**: Code that future maintainers can understand and debug
- **Safety**: No unverified hardware interactions

## Review Priorities

### 1. **Verify PR Intent & Problem Validity**
Before reviewing code quality, validate the *reason* for the PR:

- ✅ **Does the described problem actually exist?** Check if the issue is based on real user feedback, crash logs, or reproducible behavior—not assumptions or misunderstandings.
- ✅ **Is the proposed solution appropriate?** Confirm the PR solves the stated problem without introducing side effects or architectural violations.
- ✅ **Could this be a user misunderstanding?** Flag PRs where the "bug" might actually be intended behavior, a configuration issue, or a limitation the user should work around rather than fix.
- ⚠️ **Does the feature make sense?** For new features: verify they align with project scope, don't add bloat, and serve a genuine need (not a niche use case).

**Action**: If the PR's fundamental premise is questionable, flag this clearly before diving into code review.

---

### 2. **Enforce Minimal, Focused Changes**
Per AGENT.md, PRs must be surgical and targeted:

- ✅ **Single responsibility**: Each PR should address one problem or feature—not multiple unrelated fixes.
- ✅ **No refactoring scope creep**: Reject PRs that mix small fixes with large-scale code reorganization or style overhauls.
- ✅ **No high-level STL wrappers**: Do NOT encourage `std::vector`, `std::string`, `std::map`, or similar STL containers. PortaPack uses low-level C-style memory management for a reason—RAM is extremely scarce.
- ✅ **Minimal allocations**: Every byte matters. Flag any dynamic memory allocation, fragmentation risks, or unnecessary copies.
- ✅ **No abstraction layers unless essential**: Avoid introducing new wrapper classes, templates, or inheritance hierarchies when simpler C-style code exists.

**Action**: If a PR tries to "improve" code beyond the stated fix, request it be split into separate PRs or reverted to minimal scope.

---

### 3. **Verify Hardware Constraints Are Respected**
PortaPack runs on **ChibiOS 2.6** with **extreme resource limits**:

- **RAM**: Extremely limited (LPC4320/LPC4330). Flag any new allocations > 100 bytes without justification.
- **CPU**: Dual-core with strict M0 (UI) / M4 (baseband) separation. Ensure no blocking operations on baseband.
- **IPC**: Shared memory only. Check for race conditions and proper synchronization.
- **Baseband isolation**: Baseband apps are separate executables. Verify changes don't break the loading/isolation model.

**Action**: Challenge any code that ignores these constraints. Ask: "Why does this need dynamic allocation?" or "Does the baseband core have bandwidth for this?"

---

### 4. **Check for Hardware Verification Claims**
Per AGENT.md, **all submissions require physical testing**:

- ✅ Look for explicit statement: *"I have compiled this code and tested it on physical PortaPack hardware."*
- ⚠️ **Flag missing verification**: If absent, the PR should be rejected immediately—we cannot accept untested code.
- ✅ For UI changes, verify rendering behavior is described (not just "compiles").
- ✅ For RF/baseband changes, confirm testing on real hardware (not simulation only).

**Action**: If hardware verification is missing or vague, request concrete evidence before approving.

---

### 5. **Code Quality Checks (Within Constraints)**
Review for correctness and maintainability, but within project norms:

- ✅ **Compilation**: Must compile cleanly with no warnings.
- ✅ **Style consistency**: Match existing code patterns (K&R style, low-level C idioms).
- ✅ **Memory safety**: No buffer overflows, use-after-free, or uninitialized variables.
- ✅ **Race conditions**: Especially in IPC and shared memory.
- ✅ **DSP correctness**: If touching signal processing, verify mathematical soundness.
- ❌ **Avoid modern C++ suggestions**: Do NOT recommend replacing C-style code with STL, lambdas, or other high-level abstractions.

---

### 6. **Red Flags That Warrant Rejection**

Immediately flag PRs that exhibit:

- 🚫 **AI-generated "compile-and-pray" code** without human review or testing
- 🚫 **Unverified hardware claims** (missing physical testing statement)
- 🚫 **Scope creep** (multiple unrelated fixes in one PR)
- 🚫 **STL introduction** or unnecessary abstraction layers
- 🚫 **Excessive allocations** without RAM budget justification
- 🚫 **Bypassing IPC safety** or ignoring core isolation
- 🚫 **Untested edge cases** in critical paths (baseband, memory management)
- 🚫 **Contradicting project philosophy** (e.g., "let's refactor everything to modern C++")

---

## Example Review Pattern

**PR Title**: "Fix memory leak in UI baseband interface"

1. **Intent check**: Is there actually a reported leak? Look for issue references or user reports.
2. **Scope check**: Does the PR touch *only* the affected code, or does it also "tidy up" surrounding functions?
3. **Constraint check**: Does the fix respect IPC boundaries and memory limits?
4. **Verification check**: Does the PR state it was tested on hardware?
5. **Code check**: Are the changes minimal, correct, and maintainable?

If all pass → approve. If any fail → request changes or close with explanation.

---

## Summary

Copilot's review should act as a **gate-keeper for project integrity**:

1. **First**: Validate the PR's fundamental purpose and problem statement.
2. **Second**: Ensure changes are minimal and focused.
3. **Third**: Verify hardware compatibility and memory constraints.
4. **Fourth**: Confirm physical testing claims.
5. **Fifth**: Check code correctness within project norms.
6. **Finally**: Reject any PR that violates core principles.

Remember: **PortaPack Mayhem thrives on constraints.** Copilot should reinforce them, not encourage abstractions that add bloat.
