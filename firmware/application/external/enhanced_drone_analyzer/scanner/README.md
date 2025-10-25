# Enhanced Drone Analyzer Scanner App

## Overview
The Scanner App is responsible for drone detection, RF scanning, hardware control, and real-time analysis within the Enhanced Drone Analyzer modular application suite.

## Architecture
This app focuses on scanning operations and delegates configuration management to the companion Settings App.

### Key Features
- **Drone Detection**: Advanced algorithm-based drone identification
- **Spectrum Analysis**: Real-time RF spectrum monitoring
- **Hardware Control**: Direct interface with SDR hardware
- **Logging**: CSV-based detection logging with timestamps
- **Movement Tracking**: Approaching/Receding drone detection
- **Multiple Scan Modes**: Database scan, Wideband monitor, Hybrid discovery

## File Structure
```
scanner/
├── enhanced_drone_analyzer_scanner_main.cpp  # Portapack entry point
├── ui_scanner_combined.hpp                   # Unified scanner headers (22KB)
├── ui_scanner_combined.cpp                   # Unified scanner implementation (43KB)
├── CMakeLists.txt                           # Build configuration
└── README.md                               # This documentation
```

## Dependencies
### Integration with Settings App
The Scanner App reads configuration from `/sdcard/ENHANCED_DRONE_ANALYZER_SETTINGS.txt` which is managed by the Settings App. Key parameters include:

- `spectrum_mode` - NARROW, MEDIUM, WIDE, ULTRA_WIDE
- `scan_interval_ms` - Scanning interval (default: 1000)
- `rssi_threshold_db` - Detection threshold (default: -90)
- `enable_audio_alerts` - Audio alerts on/off
- `freqman_path` - Frequency database path

### Hardware Requirements
- SDR device with Portapack interface
- Compatible frequency range: 50MHz - 6GHz
- Minimum bandwidth: 100kHz

## Usage
1. **Launch**: Access via Portapack Apps menu → "Enhanced Drone Analyzer Scanner"
2. **Configuration**: Settings controlled via companion Settings App
3. **Operation**: Start scanning with START/STOP button
4. **Monitoring**: View real-time detections and threat levels

## Classes Overview

### Core Components
- **DroneScanner**: Main scanning logic and detection algorithms
- **DroneHardwareController**: Hardware abstraction and tuning
- **DetectionRingBuffer**: High-performance detection history
- **DroneDetectionLogger**: CSV logging system

### UI Components
- **SmartThreatHeader**: Live threat display with progress bar
- **ThreatCard**: Individual drone detection cards
- **ConsoleStatusBar**: System status and alerts
- **DroneDisplayController**: Comprehensive display management
- **EnhancedDroneSpectrumAnalyzerView**: Main application view

## Communication with Settings App
Settings are exchanged via the shared TXT file. The Scanner App:
- Reads settings from TXT file on startup
- Exports current scanning state back to TXT file
- Supports live parameter updates from Settings app

## Build Instructions
```bash
# Build within Portapack project
mkdir build && cd build
cmake .. -DINCLUDE_SCANNER_APP=ON
make enhanced_drone_analyzer_scanner
```

## Testing
- Launch app and verify hardware initialization
- Change settings in Settings app and verify propagation
- Test different scan modes (Database, Wideband, Hybrid)
- Validate detection logging to CSV
- Test audio alerts on threat detection

## Version
Current Version: 1.0.0 (Post-Modular Upgrade)
Previous: Part of monolithic Enhanced Drone Analyzer v0.3.1

## Known Issues
- Requires Settings app TXT file for initial configuration
- Hardware initialization may fail in demo mode
- CSV logging dependent on SD card availability

## Future Enhancements
- Real-time map integration
- Advanced AI detection algorithms
- Wireless coordination between multiple scanners
- Improved spectrum visualization
