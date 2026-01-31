# Frequency Suggestion App

## Overview

The Frequency Suggestion App is a utility tool for the PortaPack Mayhem firmware that helps users quickly determine the optimal receiver settings for any given frequency. It provides band information, suggested demodulation modes, bandwidth settings, gain recommendations, and antenna guidance.

## Features

- **Automatic Band Identification**: Identifies the frequency band and its typical usage
- **Demodulation Recommendations**: Suggests the best demodulation mode(s) for the frequency
- **Bandwidth Guidance**: Provides optimal bandwidth settings (min and max)
- **Gain Settings**: Recommends LNA, VGA, and RF Amp settings for optimal reception
- **Regional Information**: Shows which region/country the band is used in
- **Usage Context**: Explains what services typically use the frequency
- **Antenna Recommendations**: Calculates optimal antenna length and provides recommendations
- **Detailed Information**: Displays comprehensive band details in the info console

## Usage

### Basic Operation

1. Launch the app from the Utilities menu
2. Enter a frequency using the frequency field (or it will default to the current receiver frequency)
3. The app will automatically display:
   - Band name
   - Region (US, EU, GLOBAL, etc.)
   - Usage type (Amateur, Commercial, Aviation, etc.)
   - Recommended demodulation mode
   - Suggested bandwidth range
   - Optimal gain settings (LNA/VGA/RF Amp)
   - Detailed information in the console area

### Controls

- **Frequency Field**: Enter or adjust the frequency to analyze
- **Apply Button**: Apply the suggested settings to the receiver (feature in development)
- **Back Button**: Return to the previous screen

### Information Display

The app displays information in two sections:

1. **Quick Reference Panel**: Shows key information at a glance
   - Frequency
   - Band Name
   - Region
   - Usage
   - Demod Mode
   - Bandwidth
   - LNA / VGA gains
   - RF Amp status

2. **Detail Console**: Provides comprehensive information including:
   - Full frequency range of the band
   - Detailed description
   - Complete settings recommendations
   - Antenna suggestions with calculated wavelengths

## Database Format

The frequency database is stored in `FREQMAN/FREQSUGGESTIONS.TXT` on the SD card.

### File Format

Each line represents one frequency band:

```
freq_start,freq_end,band_name,region,usage,primary_mode,secondary_mode,bw_min,bw_max,lna,vga,rf_amp,description
```

### Field Descriptions

| Field | Type | Description | Example |
|-------|------|-------------|---------|
| `freq_start` | Integer | Starting frequency in Hz | `144000000` |
| `freq_end` | Integer | Ending frequency in Hz | `148000000` |
| `band_name` | String | Name of the frequency band | `2m Ham` |
| `region` | String | Region/country code | `GLOBAL`, `US`, `EU`, `JP` |
| `usage` | String | Primary usage type | `Amateur`, `Broadcasting`, `Aviation` |
| `primary_mode` | String | Primary demodulation mode | `NFM`, `AM`, `WFM`, `USB`, `LSB` |
| `secondary_mode` | String | Secondary mode (or `MULTI`) | `USB`, `DIGITAL`, `MULTI` |
| `bw_min` | Integer | Minimum bandwidth in Hz | `12500` |
| `bw_max` | Integer | Maximum bandwidth in Hz | `25000` |
| `lna` | Integer | Suggested LNA gain (0-40) | `24` |
| `vga` | Integer | Suggested VGA gain (0-62) | `30` |
| `rf_amp` | Boolean | RF Amp setting (0=OFF, 1=ON) | `0` or `1` |
| `description` | String | Detailed description | `2 meter amateur radio band, voice and digital` |

### Demodulation Modes

- `AM` - Amplitude Modulation
- `NFM` - Narrowband FM
- `WFM` - Wideband FM
- `USB` - Upper Sideband
- `LSB` - Lower Sideband
- `DSB` - Double Sideband
- `SPEC` - Spectrum only (no demodulation)
- `DIGITAL` - Digital modes
- `MULTI` - Multiple modes possible

### Example Entries

```
# FM Broadcast
88000000,108000000,FM Radio,US,Broadcasting,WFM,MULTI,180000,200000,16,20,0,VHF FM broadcasting

# 2m Amateur Band
144000000,148000000,2m Ham,GLOBAL,Amateur,NFM,USB,12500,25000,24,30,0,2 meter amateur radio band

# Aircraft Band
118000000,137000000,Airband,GLOBAL,Aviation,AM,MULTI,8333,25000,24,30,0,Civil aviation communications

# ISM 433 MHz
433050000,434790000,ISM 433,EU,ISM,NFM,DIGITAL,25000,200000,32,40,0,ISM band - Remote controls, IoT, LoRa
```

### Comments

Lines starting with `#` are treated as comments and ignored.

## Customization

### Adding Custom Bands

You can add custom frequency bands by editing `FREQMAN/FREQSUGGESTIONS.TXT`:

1. Open the file on your SD card
2. Add a new line following the format above
3. Ensure all 13 fields are present
4. Save the file
5. Restart the app to load the new data

### Regional Variants

Create region-specific databases by:
- Using appropriate region codes (US, EU, JP, AU, etc.)
- Adjusting frequency ranges for local allocations
- Adding country-specific services

## Default Bands

If no database file is found, the app includes these default bands:

- AM Broadcast (530 kHz - 1.7 MHz)
- FM Broadcast (88-108 MHz)
- 2m Amateur Band (144-148 MHz)
- Aircraft Band (118-137 MHz)
- 70cm Amateur Band (420-450 MHz)
- PMR446 (446 MHz, EU)
- FRS/GMRS (462-467 MHz, US)
- ISM 433 MHz
- ISM 868 MHz (EU)
- ISM 915 MHz (US)
- GPS L1 (1575.42 MHz)

## Antenna Recommendations

The app provides antenna recommendations by:
1. Reading available antennas from `WHIPCALC/ANTENNAS.TXT`
2. Calculating quarter-wave length for the frequency
3. Providing guidance on antenna suitability

### Antenna Guidance by Frequency

- **< 50 MHz**: Long antenna recommended
- **50-150 MHz**: Full-size antenna ideal
- **150-500 MHz**: Standard antenna suitable
- **500 MHz - 1 GHz**: Compact antenna works
- **> 1 GHz**: Short antenna adequate

## Tips and Best Practices

### For Optimal Reception

1. **Start with suggested settings**: Use the recommended demod mode and bandwidth as a starting point
2. **Adjust bandwidth**: Narrow bandwidth for weak signals, wider for strong signals
3. **Fine-tune gains**: Start with suggested LNA/VGA, adjust based on signal quality
4. **Check antenna**: Ensure you're using an appropriate antenna for the frequency range

### Signal Quality Assessment

While listening/receiving:
- **No signal**: Check antenna and frequency accuracy
- **Weak signal**: Increase LNA/VGA, check antenna orientation
- **Strong signal with distortion**: Reduce gains, especially VGA
- **Intermittent signal**: Check bandwidth settings

### Common Scenarios

#### Listening to Local FM Radio
- Frequency: 88-108 MHz
- Mode: WFM
- Bandwidth: 180-200 kHz
- Gains: LNA 16, VGA 20

#### Monitoring Amateur Radio Repeaters
- Frequency: 144-148 MHz (2m) or 420-450 MHz (70cm)
- Mode: NFM
- Bandwidth: 12.5-25 kHz
- Gains: LNA 24, VGA 30

#### Aircraft Communications
- Frequency: 118-137 MHz
- Mode: AM
- Bandwidth: 8.33-25 kHz
- Gains: LNA 24, VGA 30

#### Scanning ISM Devices
- Frequency: 433 MHz (EU) or 915 MHz (US)
- Mode: NFM or DIGITAL
- Bandwidth: 25-200 kHz
- Gains: LNA 32, VGA 40

## Technical Details

### Architecture

The app is structured as an external application for PortaPack Mayhem:

```
freqsuggestion/
├── main.cpp                    # App entry point
├── ui_freqsuggestion.hpp       # UI class definition
└── ui_freqsuggestion.cpp       # Implementation
```

### Key Components

1. **FrequencyBand Structure**: Stores all band information
2. **Database Loader**: Parses FREQSUGGESTIONS.TXT
3. **Band Finder**: Searches database for matching frequency
4. **UI Components**: Displays information and accepts user input
5. **Antenna Calculator**: Computes wavelengths and recommendations

### Memory Considerations

- Database loaded once at startup
- Efficient search algorithm for fast lookups
- Minimal memory footprint for external app

## Troubleshooting

### App doesn't show any band information

1. Check that `FREQMAN/FREQSUGGESTIONS.TXT` exists on SD card
2. Verify the file format is correct (13 comma-separated fields)
3. Ensure frequencies are in Hz, not MHz
4. Check that the file doesn't have UTF-8 BOM or special characters

### Wrong band displayed

1. Verify frequency is correct (in Hz)
2. Check for overlapping band definitions in database
3. Ensure frequency ranges in database don't have gaps

### Antenna recommendations missing

1. Verify `WHIPCALC/ANTENNAS.TXT` exists
2. Check file format matches antenna length app format
3. Default antennas (ANT500, ANT700) will be used if file not found

## Future Enhancements

Planned features for future releases:

- [ ] Apply settings directly to receiver with one button
- [ ] Real-time signal quality assessment
- [ ] Signal strength meter integration
- [ ] Waterfall display integration
- [ ] Custom presets and favorites
- [ ] Band scanner mode
- [ ] Export/import database functionality
- [ ] Online database updates
- [ ] Integration with frequency manager
- [ ] Machine learning signal classification

## Contributing

To contribute frequency data:

1. Edit your local `FREQSUGGESTIONS.TXT` file
2. Test the entries in the app
3. Submit entries via GitHub pull request
4. Include source/reference for frequency allocations
5. Specify region/country for local allocations

### Database Guidelines

- Use official frequency allocations when possible
- Cite sources for frequency ranges
- Be conservative with gain recommendations
- Test settings with actual hardware when possible
- Include both primary and common secondary modes

## Credits

- Based on feature request: [GitHub Issue #2469](https://github.com/portapack-mayhem/mayhem-firmware/issues/2469)
- Inspired by the Antenna Length (WhipCalc) app
- Community-contributed frequency database

## License

This app is part of PortaPack Mayhem and is licensed under GPL v2 or later.

## Version History

### v1.0.0 (2026)
- Initial release
- Comprehensive frequency database
- Automatic band identification
- Demodulation mode suggestions
- Bandwidth and gain recommendations
- Antenna guidance
- Support for global, US, EU, and JP regions

## Related Documentation

- [PortaPack Mayhem Documentation](https://github.com/portapack-mayhem/mayhem-firmware/wiki)
- [Antenna Length App](../../WHIPCALC/)
- [Frequency Manager](../../FREQMAN/)
- [HackRF Documentation](https://hackrf.readthedocs.io/)

## Support

For issues, suggestions, or questions:
- Open an issue on GitHub
- Join the PortaPack Discord community
- Check the wiki for additional information
