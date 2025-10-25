# Enhanced Drone Analyzer Settings App

## Overview
The Settings App provides configuration management, parameter editing, and system settings control for the Enhanced Drone Analyzer modular application suite. It serves as the centralized configuration hub that other components (like the Scanner App) read from.

## Architecture
This app specializes in configuration management and user interface for parameter editing, while actual scanning operations are delegated to the companion Scanner App.

### Key Features
- **Parameter Management**: Comprehensive settings editing with validation
- **Persistent Storage**: Automatic saving/loading of configuration
- **Multi-Tab Interface**: Organized settings in tabs (Audio, Hardware, Scanning)
- **Scanner Integration**: Real-time display of scanner status and state
- **Validation**: Parameter validation and default value restoration

## File Structure
```
Settings/
├── enhanced_drone_analyzer_settings_main.cpp   # Portapack entry point
├── ui_settings_combined.hpp                    # Unified settings headers (17KB)
├── ui_settings_combined.cpp                    # Unified settings implementation (16KB)
├── CMakeLists.txt                              # Build configuration
└── README.md                                  # This documentation
```

## Dependencies
### Integration with Scanner App
The Settings App writes configuration to `/sdcard/ENHANCED_DRONE_ANALYZER_SETTINGS.txt` and reads back scanner status updates. Key parameters managed:

- `spectrum_mode` - Scanning bandwidth selection
- `scan_interval_ms` - Time between scans
- `rssi_threshold_db` - Signal strength threshold
- `enable_audio_alerts` - Alert system toggle
- `audio_alert_frequency_hz` - Alert tone frequency
- `hardware_bandwidth_hz` - SDR bandwidth settings

## Communication with Scanner App
Parameters flow from Settings → TXT file → Scanner. Status flows back from Scanner → TXT file → Settings display.

## Hardware Requirements
- Compatible with standard Portapack device
- SD card for persistent settings storage
- Touch screen for UI navigation (optional but recommended)

## Usage
1. **Launch**: Access via Portapack Apps menu → "Enhanced Drone Analyzer Settings"
2. **Navigate**: Use tabs for different setting categories
3. **Configure**: Adjust parameters with validation
4. **Save**: Settings automatically persist and propagate to Scanner app
5. **Monitor**: View real-time scanner status when active

## Classes Overview

### Configuration Management
- **DroneAnalyzerSettingsManager**: Serialization, validation, file I/O
- **DroneAnalyzerSettings**: Main settings structure
- **ScannerConfig**: Legacy configuration wrapper

### UI Components
- **AudioSettingsView**: Audio alert configuration
- **HardwareSettingsView**: SDR hardware parameters
- **ScanningSettingsView**: Detection algorithm settings
- **SettingsTabbedView**: Multi-tab interface container
- **DroneAnalyzerSettingsMainView**: Main settings application view

### Utility Classes
- **Language**: Internationalization support
- **SimpleDroneValidation**: Parameter validation
- **DroneFrequencyPresets**: Frequency preset management
- **DroneFrequencyEntry**: Individual frequency definitions

## Persistent Storage
Settings are stored in `/sdcard/ENHANCED_DRONE_ANALYZER_SETTINGS.txt` using key=value format:
```
spectrum_mode=MEDIUM
scan_interval_ms=1000
rssi_threshold_db=-90
enable_audio_alerts=true
# ... additional parameters
```

## Build Instructions
```bash
# Build within Portapack project
mkdir build && cd build
cmake .. -DINCLUDE_SETTINGS_APP=ON
make enhanced_drone_analyzer_settings
```

## Testing
1. **Interface Testing**: Verify all tabs load and navigation works
2. **Parameter Validation**: Test min/max/boundary values
3. **Persistence**: Confirm settings save/load correctly
4. **Scanner Integration**: Verify settings propagate to Scanner app
5. **Status Display**: Confirm scanner status updates appear

## Default Configuration
The app includes comprehensive default settings that work out-of-the-box:

- **Spectrum**: MEDIUM mode (balanced detection/performance)
- **Intervals**: 1 second scan cycles
- **Thresholds**: -90dB RSSI detection
- **Audio**: Enabled at 800Hz alert tone
- **Hardware**: Real hardware mode, standard bandwidth

## Version
Current Version: 1.0.0 (Post-Modular Upgrade)
Previous: Part of monolithic Enhanced Drone Analyzer v0.3.1

## Known Issues
- Status updates from Scanner app require manual refresh
- Some parameters may need device reboot to take full effect
- Large frequency databases may cause slow initial loads

## Future Enhancements
- Cloud synchronization for settings
- Advanced preset management system
- Automated performance tuning
- Remote configuration via network
- Advanced validation and error checking
