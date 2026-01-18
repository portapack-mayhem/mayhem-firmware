# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

PortaPack Mayhem is a custom firmware for HackRF+PortaPack devices (H1/H2/H4 models). This is a fork of the Havoc firmware with additional features, bugfixes, and active community development. The firmware enables the HackRF software-defined radio to function as a standalone device with LCD screen and physical controls.

**Key Technologies:**
- **Language**: 73.8% C, 18.2% C++
- **Architecture**: Dual-processor ARM Cortex (M0 for UI/apps, M4 for DSP/radio)
- **Build System**: CMake
- **Hardware**: HackRF One + PortaPack addon boards

## Building the Firmware

### Docker Build (Recommended)

```bash
# Clone with submodules
git clone --recurse-submodules https://github.com/portapack-mayhem/mayhem-firmware.git
cd mayhem-firmware

# Build Docker image
docker build -t portapack-dev -f dockerfile-nogit .

# Build firmware
docker run -it --rm -v ${PWD}:/havoc portapack-dev
# Inside container:
cd /havoc/build && cmake .. && make -j4
```

### Native Linux Build

```bash
# Install dependencies (Debian/Ubuntu)
sudo apt-get install git tar wget dfu-util cmake python3 bzip2 lz4 curl hackrf python3-distutils
pip install pyyaml

# Download ARM toolchain (CRITICAL: Must use version 9-2019-q4)
# Newer versions produce oversized binaries that won't fit on device
# Download from: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads

# Build
cd mayhem-firmware && mkdir build && cd build
cmake ..
make -j$(nproc)
```

### macOS Build

```bash
brew install cmake python3 ninja lz4 dfu-util hackrf
# Download ARM GCC toolchain (version 9-2019-q4)
cd mayhem-firmware && mkdir build && cd build
cmake -G Ninja ..
ninja
```

**CRITICAL**: The ARM toolchain version matters. Use `gcc-arm-none-eabi-9-2019-q4-major` exclusively. Newer versions generate binaries that exceed device memory limits.

### Build Artifacts

- `build/firmware/*.bin` - Firmware binary only
- `build/firmware/*.ppfw.tar` - Firmware + external apps bundle

## Project Architecture

### Dual-Processor Model

**M0 Processor (Application Core)**:
- Handles UI rendering and user interaction
- Runs application logic
- Located in `firmware/application/`

**M4 Processor (Baseband/DSP Core)**:
- Real-time signal processing
- Radio control and DSP operations
- Located in `firmware/baseband/`

**Shared Memory**: 8KB range (`0x10088000` to `0x1008a000`) for inter-processor communication

### Key Directories

```
firmware/
├── application/          # M0 processor - UI and app logic
│   ├── apps/            # Individual app implementations
│   ├── ui_*.cpp/hpp     # UI components and views
│   └── CMakeLists.txt   # Build configuration for apps
├── baseband/            # M4 processor - DSP and radio
├── common/              # Shared code between processors
└── chibios/            # RTOS (ChibiOS)
```

## Creating a New App

### 1. Create App Files

Apps live in `firmware/application/apps/` with two files:

**Header (`ui_yourapp.hpp`)**:
```cpp
#ifndef __UI_YOURAPP_H__
#define __UI_YOURAPP_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "receiver_model.hpp"

namespace ui {

class YourAppView : public View {
   public:
    YourAppView(NavigationView& nav);
    ~YourAppView();

    void focus() override;
    std::string title() const override { return "Your App"; };

   private:
    NavigationView& nav_;

    // UI widgets (Labels, Buttons, Text, etc.)
    Labels labels{
        {{0, 16}, "Label Text", Theme::getInstance()->fg_light->foreground}
    };

    Button button_back{
        {10 * 8, 17 * 16, 10 * 8, 32},
        "Back"
    };

    // Message handler for real-time updates
    MessageHandlerRegistration message_handler_display{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->update_display();
        }
    };

    void update_display();
};

} /* namespace ui */

#endif
```

**Implementation (`ui_yourapp.cpp`)**:
```cpp
#include "ui_yourapp.hpp"
#include "portapack.hpp"

using namespace portapack;

namespace ui {

YourAppView::YourAppView(NavigationView& nav)
    : nav_(nav) {

    add_children({&labels, &button_back});

    button_back.on_select = [&nav](Button&) {
        nav.pop();
    };
}

YourAppView::~YourAppView() {
    // Cleanup
}

void YourAppView::focus() {
    button_back.focus();
}

void YourAppView::update_display() {
    // Called on DisplayFrameSync for real-time updates
}

} /* namespace ui */
```

### 2. Register App in Build System

**In `firmware/application/CMakeLists.txt`**:
Add to CPPSRC section (alphabetically):
```cmake
apps/ui_yourapp.cpp
```

### 3. Add to Menu

**In `firmware/application/ui_navigation.cpp`**:

Add include at top:
```cpp
#include "ui_yourapp.hpp"
```

Add menu entry in `appList` array:
```cpp
// Choose category: RX, TX, TRX, UTILITIES, GAMES
{"yourapp", "Your App", UTILITIES, Color::green(), &bitmap_icon_scanner, new ViewFactory<YourAppView>()},
```

Available categories:
- `RX` - Receiver apps
- `TX` - Transmitter apps
- `TRX` - Transceiver apps
- `UTILITIES` - Utility tools
- `GAMES` - Games

## UI Coordinate System

- **Screen Resolution**: 240×320 pixels (portrait mode typically shown as 240×304 usable)
- **Character Size**: 8×8 pixels
- **Coordinate Format**: `{x, y, width, height}` in pixels
- **Character Format**: `{col * 8, row * 16}` for typical text positioning

## Common UI Widgets

```cpp
// Labels (static text)
Labels labels{
    {{x, y}, "Text", Theme::getInstance()->fg_light->foreground}
};

// Text (dynamic content)
Text text_value{
    {x, y, width, height},
    "Initial"
};
// Update: text_value.set("New Text");

// Button
Button button_action{
    {x, y, width, height},
    "Button Label"
};
button_action.on_select = [](Button&) { /* action */ };

// FrequencyField
FrequencyField field_frequency{
    {x, y},
    initial_frequency_hz
};

// ProgressBar
ProgressBar progress{
    {x, y, width, height}
};
progress.set_value(0-255);

// Checkbox
Checkbox checkbox_option{
    {x, y},
    3,  // label length
    "Label"
};
```

## Radio Control

### Receiver Configuration

```cpp
#include "receiver_model.hpp"

// Set frequency (Hz)
receiver_model.set_frequency(433'000'000);  // 433 MHz

// Set modulation mode
receiver_model.set_modulation(ReceiverModel::Mode::WidebandFMAudio);

// Set bandwidth
receiver_model.set_baseband_bandwidth(1750000);

// Set sampling rate
receiver_model.set_sampling_rate(3072000);

// Set gains
receiver_model.set_lna(32);    // RF gain (0-40 dB)
receiver_model.set_vga(16);    // Baseband gain (0-62 dB)

// Enable/disable
receiver_model.enable();
receiver_model.disable();

// Get RSSI
int32_t rssi = receiver_model.rssi();  // in dB
```

### Transmitter Configuration

```cpp
#include "transmitter_model.hpp"

transmitter_model.set_frequency(433'000'000);
transmitter_model.set_sampling_rate(3072000);
transmitter_model.set_RF_amp(true);  // Enable RF amplifier
transmitter_model.enable();
transmitter_model.disable();
```

## Message-Based Updates

For real-time UI updates (signal strength, frequency sweeping, etc.), use the message handler pattern:

```cpp
// In header
MessageHandlerRegistration message_handler_display{
    Message::ID::DisplayFrameSync,
    [this](const Message* const) {
        this->update_display();
    }
};

// In implementation
void YourAppView::update_display() {
    // Called approximately 60 times per second
    // Update UI widgets here
}
```

## Common Development Patterns

### Frequency Scanning

```cpp
void scan_next_frequency() {
    current_freq += step_size;
    if (current_freq > end_freq) {
        current_freq = start_freq;
    }
    receiver_model.set_frequency(current_freq);
}
```

### RSSI Monitoring

```cpp
void check_signal() {
    int32_t rssi = receiver_model.rssi();

    // Normalize RSSI (-100 to 0 dBm) to 0-255 for progress bar
    int32_t normalized = (rssi + 100) * 255 / 100;
    normalized = std::clamp(normalized, 0, 255);

    progress_bar.set_value(normalized);
}
```

## App Compatibility and Versioning

**Critical**: External apps (`.ppma` files) must match firmware version exactly.

### Version Mismatch Issues

- Apps in `APPS/` folder on SD card must match installed firmware version
- SD card contents must be updated when firmware is updated
- Settings corruption can occur when upgrading - delete files in `SETTINGS/` folder if issues arise

### External App Distribution

For distributing standalone apps without requiring firmware rebuild, see:
- [PortaPack Mayhem MDK](https://github.com/portapack-mayhem/mayhem-mdk)
- Creates `.ppma` files that load from SD card
- More complex workflow but enables user-installable apps

## Testing and Debugging

### Build and Flash Workflow

1. Build firmware with your changes
2. Flash to device using DFU mode:
   ```bash
   # Put device in DFU mode (varies by model)
   dfu-util --device 1fc9:000c --download build/firmware/portapack-h1_h2-mayhem.bin --reset
   ```
3. Test on actual hardware (no software simulator available)

### Common Build Issues

**Binary too large**: Using wrong ARM toolchain version. Must use `gcc-arm-none-eabi-9-2019-q4`.

**Submodule errors**: Clone with `--recurse-submodules` or run `git submodule update --init --recursive`.

**Missing dependencies**: Check build command output - install missing packages with your system's package manager.

## Code Style

- Follow existing code patterns in similar apps
- Use snake_case for functions and variables
- Use PascalCase for class names
- Widget naming: `widget_type_descriptor` (e.g., `button_start`, `text_frequency`)
- Keep UI update logic in `update_display()` or similar methods
- Clean up resources in destructor (disable models, unregister handlers)

## HackRF Frequency Range

- **Operational Range**: 1 MHz - 6 GHz
- **Optimal Range**: 10 MHz - 6 GHz
- **Half-duplex**: Cannot transmit and receive simultaneously
- **Sample Rate**: Up to 20 MHz

## Additional Resources

- [GitHub Wiki](https://github.com/portapack-mayhem/mayhem-firmware/wiki)
- [Create a Simple App Guide](https://github.com/portapack-mayhem/mayhem-firmware/wiki/Create-a-simple-app)
- [Access Radio Hardware](https://github.com/portapack-mayhem/mayhem-firmware/wiki/Access-Radio-Hardware)
- [Discord Community](https://discord.gg/tuwVMv3)
- [Mayhem Hub](https://hackrf.app/)

## Current Status

Latest stable release: Check [GitHub Releases](https://github.com/portapack-mayhem/mayhem-firmware/releases/latest)

Nightly builds available at: https://github.com/portapack-mayhem/mayhem-firmware/releases/

## Note on Bug Detector App

A bug detector app (`ui_bug_detector.cpp/hpp`) has been created in this repository as an example. It demonstrates:
- Frequency scanning across configurable ranges
- RSSI monitoring and display
- Real-time UI updates via message handlers
- Common surveillance device frequencies (433 MHz - 2.5 GHz range including GSM, WiFi, Bluetooth, RF bugs)

The app is registered in the Utilities menu and ready for compilation.
