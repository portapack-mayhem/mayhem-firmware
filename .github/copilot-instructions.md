# Copilot Pull Request Review Instructions

These instructions define how to review pull requests for the PortaPack Mayhem firmware (an embedded, resource-constrained, dual-core ARM Cortex-M0/M4 project). They complement `AGENT.md`.

## 1. Verify intent and validity first

Before reviewing any code, answer these questions from the PR title, description and diff:

- What does the PR claim to add or fix?
- Does the feature make sense for this firmware and its users?
- Is the problem real? Could it be a user misunderstanding, a misconfiguration, or expected behavior? Is the change actually invalid or unnecessary?
- Does the change really solve the stated problem, or does it only hide a symptom?

If the premise is questionable, say so first and explain why, before commenting on code details. Do not polish code whose purpose is invalid.

## 2. Minimal, focused changes

- The diff must be as small as possible and address a single purpose.
- Flag unrelated refactors, reformatting, renames, or drive-by fixes (scope creep).
- Flag high-level abstractions or STL wrappers (e.g. `std::function`, `std::string`, `std::vector`, `std::map`, streams, exceptions, heavy templates) where simple code or existing project helpers work.
- Prefer existing project patterns and helpers over new ones.

## 3. Hardware constraints

- RAM and flash are very limited. Flag dynamic allocation, large stack or static buffers, and unneeded copies.
- The firmware is dual-core (M0 application, M4 baseband). Check that code runs on the correct core and that shared memory and IPC messages are used safely (message sizes, ordering, no blocking in baseband/interrupt context, no unsynchronized shared state).
- External apps and baseband images must fit their memory regions.
- Baseband/DSP code must stay deterministic and fast.

## 4. Hardware verification

- Changes touching radio, baseband, peripherals, or timing must state how they were tested on real hardware (board model, firmware build).
- If the PR does not say it was tested, or only claims it "should work", ask for explicit verification.

## 5. Code quality (within project norms)

- Follow the existing style and naming of surrounding code.
- Check for undefined behavior, integer overflow, buffer bounds, uninitialized variables, and unsafe casts.
- Check that new UI strings, app IDs, and message IDs follow existing conventions and remain unique/consecutive where required.

## 6. Red flags (recommend rejection or major rework)

- The problem is not real, or the change rests on a misunderstanding.
- Large diffs for small problems; unrelated changes mixed in.
- Introduces STL-heavy abstractions or dynamic memory without strong justification.
- Unsafe cross-core/IPC access.
- Claims of hardware behavior with no verification.
- Generated, AI-written, or copied code the author clearly does not understand.
