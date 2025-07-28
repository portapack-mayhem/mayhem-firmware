# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

PortaPack Mayhem is a fork of the PortaPack firmware for HackRF SDR (Software Defined Radio) devices. It's a comprehensive SDR application suite that runs on ARM Cortex-M microcontrollers, providing a touchscreen interface for various radio applications including spectrum analysis, signal generation, protocol decoders, and more.

## Build System and Development Commands

### Building the Project

The project uses CMake as its primary build system:

- **Basic build**: `cmake .. && make` (from a build directory)
- **Parallel build**: `make -j` or `make -j10` (for faster compilation)
- **With ccache**: `cmake -DUSE_CCACHE=ON ..` (for faster rebuilds, disabled by default)
- **Using Ninja**: `cmake -G Ninja .. && ninja`

### Docker Development

The project provides Docker support for cross-platform development:

- **Build dev container**: `./dockerize.sh build`
- **Build mayhem**: `./dockerize.sh` (auto-builds container if needed)
- **Interactive shell**: `./dockerize.sh shell`
- **With parameters**: `./dockerize.sh -j10` or `./dockerize.sh ninja -j10`

### Code Formatting

- **Format code**: `./format-code.sh` (uses clang-format-13)

### Testing

The project has test suites in `firmware/test/`:
- **Application tests**: Located in `firmware/test/application/`
- **Baseband tests**: Located in `firmware/test/baseband/`
- Tests use the doctest framework

## Architecture Overview

### Core Components

The firmware is organized into several key modules:

1. **Application Layer** (`firmware/application/`):
   - Main UI and application logic
   - Individual apps for different SDR functions (AIS, ADSB, POCSAG, etc.)
   - User interface components and navigation
   - Hardware abstraction for peripherals

2. **Baseband Layer** (`firmware/baseband/`):
   - Real-time signal processing
   - DSP algorithms and filters  
   - Protocol processors for various radio standards
   - Direct hardware interface for radio operations

3. **Common Layer** (`firmware/common/`):
   - Shared utilities and data structures
   - Hardware drivers (ADCs, display, audio codecs, etc.)
   - Communication protocols between layers
   - Memory management and buffer handling

4. **External Apps** (`firmware/application/external/`):
   - Standalone applications that can be loaded dynamically
   - Games, utilities, and specialized tools
   - Separated from main firmware for modularity

### Key Architectural Patterns

- **Dual-core Architecture**: M0 core runs application/UI, M4 core handles baseband processing
- **Message-based Communication**: Cores communicate via shared memory and message queues
- **Event-driven UI**: Uses ChibiOS RTOS with event handling for user interactions
- **Modular Apps**: Each radio function is implemented as a separate app module
- **Hardware Abstraction**: Clean separation between hardware drivers and application logic

### Important Files

- `firmware/application/main.cpp`: Application entry point
- `firmware/application/ui_navigation.cpp`: Main UI navigation system
- `firmware/baseband/baseband.cpp`: Baseband processor entry point
- `firmware/common/portapack_*.cpp`: Core hardware abstraction
- `CMakeLists.txt`: Root build configuration

## Development Guidelines

### Adding New Features

1. For new radio applications, create files in `firmware/application/apps/`
2. For signal processing, add processors in `firmware/baseband/`
3. For UI components, extend classes in `firmware/application/ui/`
4. External apps go in `firmware/application/external/`

### Code Organization

- Header files use `.hpp` extension, implementation uses `.cpp`
- Hardware-specific code goes in `firmware/common/`
- UI code follows the widget/view pattern
- Use existing message types for inter-core communication

### Memory Considerations

- RAM is limited - be mindful of BSS section size
- Use `constexpr` where possible for compile-time constants
- Prefer const data in .cpp files rather than headers
- Check map files to monitor memory usage

### Testing

- Add unit tests to `firmware/test/application/` for application code
- Add baseband tests to `firmware/test/baseband/` for DSP code
- Use the existing doctest framework
- Run tests as part of the build process

## Hardware Support

The firmware supports multiple PortaPack hardware versions:
- H1 (original)
- H2/H2M (Mayhem edition)
- H4M (latest with improvements)

Hardware-specific code is abstracted through the common layer to support different revisions.