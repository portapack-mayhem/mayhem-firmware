# 🔧 Mayhem Firmware: Improvements & Feature Roadmap

> **Document Created:** 2026-01-14  
> **Last Updated:** 2026-01-14 (Quick Wins Implementation)  
> **Status:** Active Development

---

## 📋 Table of Contents

1. [Executive Summary](#executive-summary)
2. [Code Quality Improvements](#code-quality-improvements)
3. [New Feature Suggestions](#new-feature-suggestions)
   - [RF/Radio Features](#rfradio-features)
   - [Utility/UX Features](#utilityux-features)
   - [Protocol Decoders](#protocol-decoders)
   - [Games & Entertainment](#games--entertainment)
   - [Standalone/External Apps Infrastructure](#standaloneexternal-apps-infrastructure)
4. [Architecture Improvements](#architecture-improvements)
5. [Documentation & Developer Experience](#documentation--developer-experience)
6. [Priority Matrix](#priority-matrix)
7. [Quick Wins](#quick-wins)
8. [Progress Tracking](#progress-tracking)

---

## Executive Summary

The **PortaPack Mayhem** firmware is a mature, feature-rich codebase for SDR (Software Defined Radio) applications. This document identifies areas for improvement and potential new features organized by category.

### Current Codebase Statistics
- **350+ TODO comments** scattered throughout the codebase
- **68 external apps** in the `firmware/application/external/` directory
- **90 SubGhz/Weather/Car protocols** in `firmware/baseband/fprotos/`
- **Standalone apps:** Digital Rain, Pac-Man (with infrastructure for more)

---

## Code Quality Improvements

### 1. Address Existing TODOs

The codebase contains over 350 TODO comments. Priority items to address:

| File | Line | Issue | Priority |
|------|------|-------|----------|
| `ui_navigation.hpp` | 167 | StatusTray should be refactored into a generic `StackPanel` control | Medium |
| `analog_audio_app.cpp` | 386 | "Lame hack to hide options view due to bad paint/damage algorithm" | High |
| `ble_rx_app.cpp` | 393 | "Display Connect Request Information - just printing full hex data" | Medium |
| `baseband_thread.cpp` | 92 | Correctly place sampling rate into buffer | Low |
| `event_m0.cpp` | 126-127 | Distribute display sleep message, shut down baseband data | Medium |
| `ui_adsb_rx.hpp` | 186 | Make logging optional | Low |
| `portapack.hpp` | 42 | Refactor as a class with guardrails on setting properties | Medium |

- [ ] Audit all TODOs and categorize by priority
- [ ] Create GitHub issues for critical TODOs
- [ ] Establish TODO policy for new code

### 2. Memory & Performance Optimization

- [ ] Replace raw pointers with smart pointers where feasible
- [ ] Optimize frequent string operations in `string_format.cpp`
- [ ] Consider lazy-loading for external apps to reduce memory footprint
- [ ] Profile memory usage during heavy operations
- [ ] Optimize DSP routines in baseband processors

### 3. Error Handling Consistency

- [ ] Implement proper `Result<>` types (noted in `file.hpp` lines 333, 355)
- [ ] Add exception handling in critical file operations
- [ ] Standardize error reporting across all apps
- [ ] Create error code enumeration for debugging

---

## New Feature Suggestions

### RF/Radio Features

#### 1. 🛰️ LoRa Support
**Priority:** ⭐⭐⭐⭐⭐ | **Effort:** High | **Impact:** Very High

Add LoRa modulation/demodulation support for IoT device analysis:

- [ ] LoRa decoder (125kHz-500kHz bandwidth)
- [ ] LoRaWAN packet inspection
- [ ] Support for common frequency bands:
  - EU: 868 MHz
  - US: 915 MHz
  - AU: 915 MHz
  - AS: 433 MHz
- [ ] Spreading factor detection (SF7-SF12)
- [ ] Basic packet decoding and display

**Files to modify:**
- Create `proc_lora.cpp` / `proc_lora.hpp` in `firmware/baseband/`
- Create `ui_lora.cpp` / `ui_lora.hpp` in `firmware/application/apps/`

---

#### 2. 📡 Enhanced DMR/DSTAR/P25 Support
**Priority:** ⭐⭐⭐⭐ | **Effort:** Very High | **Impact:** High

Digital voice protocol improvements:

- [ ] DMR Tier 2 decoding
- [ ] P25 Phase 1 decoding
- [ ] P25 Phase 2 support
- [ ] NXDN decode capability
- [ ] D-STAR header decoding
- [ ] Integration with trunking database
- [ ] Channel activity logging

---

#### 3. 🚗 Extended Car Key Protocol Support
**Priority:** ⭐⭐⭐⭐ | **Effort:** Low-Medium | **Impact:** High

Current `fprotos` supports: BMW, Fiat, Ford, Kia (v0-v5), Subaru, Suzuki, VW

**Add support for:**
- [ ] Tesla key fob protocol
- [ ] Mercedes-Benz newer protocols
- [ ] Toyota/Lexus smart key
- [ ] Honda rolling code analysis
- [ ] Hyundai/Genesis protocols
- [ ] Enhanced reverse engineering tools for unknown protocols

**Implementation path:** Follow existing pattern in `firmware/baseband/fprotos/c-*.hpp`

---

#### 4. 📻 RDS Decoding Enhancement
**Priority:** ⭐⭐⭐ | **Effort:** Medium | **Impact:** Medium

Current implementation in `proc_rds.cpp` / `ui_rds.cpp`. Extend with:

- [ ] RDS+ (RT+) support
- [ ] Traffic Message Channel (TMC) decoding
- [ ] Time synchronization from RDS CT (Clock Time)
- [ ] RadioText+ parsing
- [ ] Enhanced PI code database
- [ ] Station logo fetching (where available)

---

#### 5. 🛩️ ADS-B Enhancements
**Priority:** ⭐⭐⭐⭐ | **Effort:** Medium | **Impact:** High

Building on existing `ui_adsb_rx.cpp`:

- [ ] Add MLAT (Multilateration) support with external GPS
- [ ] Aircraft database integration (OpenSky Network API)
- [ ] Alert system for specific aircraft:
  - Tail numbers
  - Flight numbers
  - Aircraft types
- [ ] Historical flight path visualization
- [ ] Export to KML for Google Earth
- [ ] Integration with existing GeoMap component

---

### Utility/UX Features

#### 6. 📱 Mobile App Companion
**Priority:** ⭐⭐⭐ | **Effort:** Very High | **Impact:** High

Create bridge for smartphone connectivity:

- [ ] USB serial bridge protocol
- [ ] Bluetooth serial bridge (if hardware permits)
- [ ] Android companion app
- [ ] iOS companion app
- [ ] Features:
  - Remote control
  - Real-time spectrum display
  - Cloud backup for captured data
  - Large-screen visualization

---

#### 7. 🎮 Enhanced UI/UX
**Priority:** ⭐⭐⭐⭐ | **Effort:** Medium | **Impact:** High

- [ ] **Theme system improvements:**
  - Multiple theme presets
  - Custom color configuration
  - Per-app theme overrides
- [ ] **Haptic feedback** patterns for button presses
- [ ] **Screen orientation** options (portrait mode for some apps)
- [ ] **Gesture support:**
  - Swipe between apps/screens
  - Long-press context menus
- [ ] **Quick settings panel** (pull-down from status bar)
- [ ] **Favorites/shortcuts** system for frequently used apps
- [ ] **Recent apps** list

**Relevant files:** `firmware/application/theme.cpp`, `firmware/common/ui_widget.cpp`

---

#### 8. 📊 Signal Analysis Tools
**Priority:** ⭐⭐⭐⭐ | **Effort:** High | **Impact:** High

- [ ] **Constellation diagram** view for QAM/PSK signals
- [ ] **Eye diagram** for digital signal quality assessment
- [ ] **Persistence spectrum** analyzer mode
- [ ] **Signal strength mapping** with GPS coordinates
- [ ] **Waterfall save/replay** functionality
- [ ] **Automatic modulation recognition** (AMR)
- [ ] **Signal recording triggers** (threshold-based)

---

#### 9. 🔐 Security Analysis Tools
**Priority:** ⭐⭐⭐ | **Effort:** High | **Impact:** Medium

- [ ] **433MHz rolling code analyzer** with pattern detection
- [ ] **Replay attack detection** and alerting
- [ ] **Wireless sensor network** discovery tool
- [ ] **ZigBee basic protocol** analysis (if bandwidth permits)
- [ ] **Z-Wave detection** and basic decoding
- [ ] **Enhanced BLE analysis:**
  - GATT service enumeration
  - Pairing process visualization
  - Known vulnerability detection

---

#### 10. 🗺️ Enhanced Geographic Features
**Priority:** ⭐⭐⭐⭐ | **Effort:** Medium | **Impact:** High

Building on existing `ui_geomap.cpp`:

- [ ] **OSM tile caching** for offline use
- [ ] **Route recording** during wardriving sessions
- [ ] **Heat map generation** for RF coverage
- [ ] **Export formats:**
  - KML (Google Earth)
  - GPX (GPS Exchange)
  - GeoJSON
  - CSV with coordinates
- [ ] **Import waypoints** from external files
- [ ] **Distance/bearing** calculations
- [ ] **Grid square** display (for amateur radio)

---

### Protocol Decoders

#### 11. Weather Station Protocol Expansion
**Priority:** ⭐⭐⭐ | **Effort:** Low | **Impact:** Medium

Current protocols in `firmware/baseband/fprotos/w-*.hpp` (~25 protocols).

**Add:**
- [ ] Davis Instruments weather stations
- [ ] Fine Offset newer models (WH2900, WH5000)
- [ ] WeatherFlow Tempest
- [ ] Ambient Weather WS-2000 series
- [ ] Ecowitt series
- [ ] Bresser 5-in-1 / 7-in-1
- [ ] Sainlogic weather stations

**Implementation:** Follow pattern in existing `w-*.hpp` files

---

#### 12. Smart Home Protocol Support
**Priority:** ⭐⭐⭐ | **Effort:** High | **Impact:** Medium

- [ ] **Matter/Thread** basic packet inspection
- [ ] **Tuya/Smart Life** 433MHz devices
- [ ] **Amazon Sidewalk** detection
- [ ] **Enhanced BLE beacon types:**
  - iBeacon
  - Eddystone (URL, UID, TLM, EID)
  - AltBeacon
  - Microsoft beacons
- [ ] **Xiaomi MiBeacon** support
- [ ] **Govee/INKBIRD** temperature sensors

---

#### 13. Industrial Protocol Support
**Priority:** ⭐⭐ | **Effort:** High | **Impact:** Low

- [ ] **Wireless M-Bus** (ISM band sensor networks)
- [ ] **POCSAG enhancement:**
  - Multi-capcode monitoring
  - Alphanumeric decoding improvements
  - Address book with names
- [ ] **Industrial remote control** protocols
- [ ] **Utility meter reading:**
  - OMS (Open Metering System)
  - DLMS/COSEM basics
- [ ] **TPMS expansion** (more vehicle protocols)

---

### Games & Entertainment

#### 14. Retro Game Additions
**Priority:** ⭐⭐ | **Effort:** Low | **Impact:** Low

Current games: Snake, Tetris, Breakout, Doom, Pac-Man, Space Invaders, 2048, Blackjack, Battleship, Dino Game

**Add:**
- [ ] **Pong** (potential 2-player via RF with second PortaPack)
- [ ] **Simon Says** (audio-visual memory game using speaker/LEDs)
- [ ] **Minesweeper**
- [ ] **Flappy Bird** clone
- [ ] **Audio synthesizer** with touch keyboard

**Location:** `firmware/application/external/` or `firmware/standalone/`

---

### Standalone/External Apps Infrastructure

#### 15. App Store Concept
**Priority:** ⭐⭐⭐⭐ | **Effort:** High | **Impact:** High

- [ ] **OTA app updates** - download new .ppma files directly via WiFi module
- [ ] **App catalog** browsing interface
- [ ] **Version checking** for compatibility
- [ ] **Dependency management** for complex apps
- [ ] **App ratings/reviews** integration (if server available)
- [ ] **Update notifications** in status bar

---

#### 16. Script/Macro System
**Priority:** ⭐⭐⭐ | **Effort:** High | **Impact:** Medium

- [ ] **Simple scripting language** for automated tasks
- [ ] **Record and replay** button sequences
- [ ] **Scheduled operations:**
  - Scan at specific time
  - Time-based frequency hopping
- [ ] **Conditional triggers:**
  - When signal detected, do X
  - When threshold exceeded, alert
- [ ] **Script library** on SD card
- [ ] **Script editor** (extend existing Notepad app)

---

## Architecture Improvements

### 17. Plugin System Refactoring
**Priority:** ⭐⭐⭐⭐ | **Effort:** Very High | **Impact:** High

The external app system (`firmware/application/external/`) is good but could be improved:

- [ ] **Hot-reload** of plugins without device restart
- [ ] **Inter-app communication** framework
- [ ] **Shared library** system for common functions
- [ ] **Better sandboxing** for external apps
- [ ] **Resource management** (prevent memory leaks)
- [ ] **Standardized settings** storage per app

---

### 18. Power Management
**Priority:** ⭐⭐⭐⭐ | **Effort:** Medium | **Impact:** High

Building on `battery.cpp` and `i2cdev_max17055.cpp`:

- [ ] **Battery profiling** for accurate remaining time estimation
- [ ] **Low-power modes** for long-duration scanning:
  - Screen off but processing
  - Reduced sample rate mode
  - Periodic wake scanning
- [ ] **USB power delivery** negotiation (if hardware supports)
- [ ] **Solar charging** optimization profiles
- [ ] **Power consumption** statistics per app
- [ ] **Battery health** monitoring and alerts

---

### 19. Data Management
**Priority:** ⭐⭐⭐ | **Effort:** Medium | **Impact:** Medium

- [ ] **SQLite integration** for structured data storage
- [ ] **Standardized export formats:**
  - CSV with consistent schema
  - JSON for programmatic access
  - XML for legacy systems
- [ ] **Automatic log rotation** and compression
- [ ] **Cloud sync** integration (optional, privacy-conscious)
- [ ] **Data encryption** for sensitive captures
- [ ] **Search functionality** across logs

---

## Documentation & Developer Experience

### 20. Enhanced Documentation
**Priority:** ⭐⭐⭐⭐ | **Effort:** Medium | **Impact:** High

- [ ] **API documentation** generator (improve existing Doxygen)
- [ ] **Protocol documentation** for each decoder
- [ ] **External app development guide** with templates
- [ ] **Hardware specification sheets** for all supported models
- [ ] **Architecture diagrams** (M0/M4 communication, UI flow)
- [ ] **Contributing guide** expansion
- [ ] **Troubleshooting guide**

---

### 21. Testing Infrastructure
**Priority:** ⭐⭐⭐⭐ | **Effort:** High | **Impact:** High

Expand `firmware/test/` directory:

- [ ] **Unit tests** for DSP functions
- [ ] **Integration tests** for baseband processors
- [ ] **Mock hardware** for CI/CD testing
- [ ] **Code coverage** reporting
- [ ] **Performance benchmarks**
- [ ] **Memory leak detection**
- [ ] **Automated UI testing** (screenshot comparison)

---

## Priority Matrix

| Feature | Impact | Effort | Priority Score |
|---------|--------|--------|----------------|
| Address critical TODOs | High | Medium | ⭐⭐⭐⭐⭐ |
| LoRa Support | Very High | High | ⭐⭐⭐⭐⭐ |
| Enhanced BLE features | High | Medium | ⭐⭐⭐⭐ |
| ADS-B Enhancements | High | Medium | ⭐⭐⭐⭐ |
| App Store concept | High | High | ⭐⭐⭐⭐ |
| Power management | Medium | Medium | ⭐⭐⭐⭐ |
| Additional car protocols | High | Low | ⭐⭐⭐⭐ |
| Enhanced UI/UX | High | Medium | ⭐⭐⭐⭐ |
| Signal analysis tools | High | High | ⭐⭐⭐⭐ |
| Geographic features | High | Medium | ⭐⭐⭐⭐ |
| Testing infrastructure | High | High | ⭐⭐⭐⭐ |
| Documentation | High | Medium | ⭐⭐⭐⭐ |
| Weather protocols | Medium | Low | ⭐⭐⭐ |
| Script/Macro system | Medium | High | ⭐⭐⭐ |
| Mobile companion app | Medium | Very High | ⭐⭐⭐ |
| Smart home protocols | Medium | High | ⭐⭐⭐ |
| Digital voice (DMR/P25) | High | Very High | ⭐⭐⭐ |
| Games | Low | Low | ⭐⭐ |
| Industrial protocols | Low | High | ⭐⭐ |

---

## Quick Wins

Low effort, high impact items to tackle first:

1. **Add more SubGhz protocols** to the existing fprotos system
   - Templates and patterns exist, just add new protocol definitions
   
2. **Improve weather station protocol coverage**
   - Many similar implementations exist to copy from

3. **Enhanced logging** with timestamps and GPS coordinates
   - Modify existing logger classes

4. **Preset frequency lists** for different regions/use cases
   - Add files to `sdcard/FREQMAN/`

5. **Button combo shortcuts** for frequently used functions
   - Modify `irq_controls.cpp`

6. ~~**Fix the VU meter bug** in `ui_mictx.cpp` (line 143)~~ ✅ **COMPLETED**
   - Fixed by resetting both `audio_level` variable and vumeter widget

7. ~~**Add optional logging toggle** for ADS-B (noted TODO)~~ ✅ **COMPLETED**
   - Added checkbox to enable/disable logging, saves setting to app_settings

8. ~~**Improve BLE Connect Request display** (currently hex only)~~ ✅ **COMPLETED**
   - Now displays parsed CONNECT_REQ fields: Initiator MAC, Access Address, CRC, Window, Interval, Latency, Timeout, Hop, SCA

---

## Progress Tracking

### Phase 1: Foundation (Current)
- [x] Codebase analysis complete
- [x] Roadmap document created
- [ ] TODO audit and categorization
- [x] Quick wins implementation (3 of 8 completed)
  - [x] VU meter bug fix in ui_mictx.cpp
  - [x] Optional ADS-B logging toggle
  - [x] BLE Connect Request parsed display
- [ ] Testing infrastructure basics

### Phase 2: Core Features
- [ ] LoRa support implementation
- [ ] Car protocol expansion
- [ ] Enhanced geographic features
- [ ] Power management improvements

### Phase 3: Advanced Features
- [ ] Signal analysis tools
- [ ] App store concept
- [ ] Mobile companion app planning
- [ ] Script/macro system

### Phase 4: Polish
- [ ] Documentation completion
- [ ] UI/UX refinements
- [ ] Performance optimization
- [ ] Community feedback integration

---

## Contributing

If you'd like to work on any of these features:

1. Check if there's an existing GitHub issue
2. Comment on the issue or create a new one
3. Reference this roadmap document
4. Follow the existing code style and patterns
5. Submit PR with tests where applicable

---

## Notes & References

### Key Directories
- `firmware/application/apps/` - Built-in applications
- `firmware/application/external/` - External loadable apps
- `firmware/baseband/` - DSP and signal processing
- `firmware/baseband/fprotos/` - Protocol decoders
- `firmware/common/` - Shared utilities and drivers
- `firmware/standalone/` - Standalone applications (Pac-Man, etc.)

### Related Documentation
- [GitHub Wiki](https://github.com/portapack-mayhem/mayhem-firmware/wiki)
- [Discord Community](https://discord.gg/tuwVMv3)
- [Hardware Overview](https://github.com/portapack-mayhem/mayhem-firmware/wiki/Hardware-overview)

---

*This document is a living roadmap and will be updated as features are implemented or priorities change.*
