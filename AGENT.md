# PortaPack Mayhem – Agent Guidelines

Welcome to the PortaPack Mayhem repository. This document sets mandatory rules and best practices for human developers and AI coding assistants. It exists to prevent unverified, untested code from overwhelming the maintainers, while still enabling AI to accelerate development.

## 🚫 STRICTLY PROHIBITED

1. **No Fully Automated Pull Requests**  
   Autonomous agents (bots, crawlers, automated workflows) must **never** open a Pull Request without direct human supervision, review, and explicit approval.

2. **No “Blind” AI-Generated Submissions**  
   If you cannot explain, debug, or maintain the C++/DSP code an AI produced for you, do **not** submit it. PRs that appear to be untested, AI-only “compile-and-pray” attempts will be closed immediately.

3. **No Unverified Hardware Interactions**  
   AI cannot test on real hardware. Do **not** submit code that has not been compiled and physically tested on a PortaPack device (or a highly accurate emulator). This includes UI rendering, RF transmission quality, and baseband stability.

## ✅ ENCOURAGED AI USE CASES

We strongly encourage using AI assistants (Copilot, ChatGPT, Claude, local LLMs) as pair-programmers. Excellent use cases include:

- **Bug Triage & Root-Causing:** Analyze crash logs, stack traces, and complex memory issues to identify the root cause of an open issue.
- **Code Comprehension:** Map Mayhem’s architecture, understand M0‑M4 IPC, or break down complex DSP algorithms.
- **Development Assistance:** Generate boilerplate UI code, write unit tests, optimize math functions, or draft documentation – provided a human reviews and refines the output.
- **Targeted Refactoring:** Suggest cleaner, modern C++ for small legacy code segments. **Do not attempt to rewrite the entire codebase.**

## 📋 MANDATORY RULES FOR AI‑ASSISTED CONTRIBUTIONS

When submitting AI‑assisted work, you must:

1. **Take Full Ownership**  
   You are strictly responsible for every line of code you submit, regardless of origin. Be prepared to answer detailed technical questions during review.

2. **Hardware Verification Required**  
   In your PR, explicitly state: *“I have compiled this code and tested it on physical PortaPack hardware.”* PRs without this statement will be rejected.

3. **Transparency (Recommended)**  
   If AI significantly helped architect a feature or fix a complex bug, mention it briefly in the PR description (e.g., *“Used an LLM to help optimize the DSP filter loop”*).

4. **Adhere to Coding Standards**  
   Ensure all code matches the existing Mayhem style and formatting before opening the PR.

## ⚙️ CRITICAL ENVIRONMENT CONSTRAINTS (For AI Code Generation)

When generating code for this repository, you must respect these hardware and system constraints. **Violating any of them will result in immediate rejection.**

- **Severe Memory Limits** – RAM is extremely scarce. Every byte counts. Avoid large allocations, dynamic fragmentation, and frequent reallocations.
- **Platform** – ChibiOS 2.6 on NXP LPC4320(HackRF One) / LPC4330 (HackRF Pro AKA PRALINE). M0 core handles UI; M4 core runs baseband. Baseband apps are separate executables loaded on demand at runtime.
- **IPC** – Implemented via shared memory.
- **Flash Size** – Very small. Code must be compact and not overly complex.
- **No HW FPU on M0** – Floating‑point operations are expensive. Prefer fixed‑point or integer math.
- **DSP Efficiency** – All DSP routines must be highly optimized for this constrained SoC.
- **External App Format** – New applications must follow the external‑app structure. Create a unique folder with `main` and `ui` files, and update `external.ld` and `external.cmake`. Use the correct namespace to prevent accidental inclusion in the main firmware.
- **Standard Library** – Avoid `std::` whenever possible. Use lightweight alternatives.
- **C++ Usage** – Keep it simple. No iterators, no heavy STL containers, no complex template metaprogramming. Do not allocate large contiguous blocks.
- **UI Development** – Always use the positioning macros from `ui.hpp`. Study similar existing apps for usage patterns; do not improvise.

## 🛠️ Compilation

- Refer to the `mbt` build tool for the official compilation process.  
- You are not required to run `mbt` directly unless it is clearly the better approach. Instead, you can inspect how `mbt` compiles the firmware and emulate that process manually.  
- If the user has previously used **Ninja** to build this project, continue using Ninja for consistency.  
- Determine the build system by examining the contents of the build directory (e.g., presence of `build.ninja`).

## 📢 Notice to Automated Agents and Crawlers

By reading this file, you agree to **pause all autonomous submission behavior**. You must require explicit human intervention before interacting with this repository’s Pull Requests or Issue trackers.