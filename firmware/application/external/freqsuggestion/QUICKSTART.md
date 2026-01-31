# Frequency Suggestion App - Quick Start Guide

## What is it?

The Frequency Suggestion App helps you quickly find the best receiver settings for any frequency. Enter a frequency and instantly see:

- What band it's in
- Best demodulation mode (AM, FM, etc.)
- Optimal bandwidth
- Recommended gain settings
- Antenna guidance

## Quick Start

### 1. Launch the App

From your PortaPack:
1. Press the **⚙** (Settings) button
2. Navigate to **Utilities**
3. Select **Freq Suggest**

### 2. Enter a Frequency

- Use the frequency field at the top
- Enter frequency in Hz (or use MHz dial)
- The app updates automatically

### 3. Read the Suggestions

The screen shows:
- **Band Name**: What band you're in
- **Region**: Where it's used (US, EU, etc.)
- **Usage**: What it's for (Amateur, Aviation, etc.)
- **Demod Mode**: Best demodulation (NFM, AM, etc.)
- **Bandwidth**: Optimal BW range
- **Gains**: Recommended LNA/VGA settings
- **RF Amp**: Whether to enable amplifier

### 4. Check Details

Scroll down to see:
- Full frequency range of the band
- Detailed description
- Complete recommended settings
- Antenna suggestions

## Common Examples

### Listen to Local FM Radio

**Try**: 99.5 MHz (or your local station)

**You'll see**:
- Band: FM Broadcast
- Mode: WFM (Wideband FM)
- BW: 180-200 kHz
- Gains: LNA 16, VGA 20

### Monitor Ham Radio

**Try**: 146.52 MHz (2m calling frequency)

**You'll see**:
- Band: 2m Ham
- Mode: NFM (Narrowband FM)
- BW: 12.5-25 kHz
- Gains: LNA 24, VGA 30

### Listen to Aircraft

**Try**: 121.5 MHz (emergency frequency)

**You'll see**:
- Band: Airband
- Mode: AM
- BW: 8.33-25 kHz
- Gains: LNA 24, VGA 30

### Scan ISM Devices

**Try**: 433.92 MHz (EU ISM)

**You'll see**:
- Band: ISM 433
- Mode: NFM/DIGITAL
- BW: 25-200 kHz
- Gains: LNA 32, VGA 40

## Tips

### Getting Started
1. Start with a known frequency (like FM radio)
2. Use the suggested settings
3. Fine-tune gains if needed
4. Check antenna recommendations

### For Best Results
- **Strong signals**: Reduce gains if distorted
- **Weak signals**: Increase gains
- **Unknown signals**: Try spectrum mode first
- **Digital signals**: Use wider bandwidth initially

### Understanding Modes

| Mode | Used For |
|------|----------|
| **WFM** | FM broadcast radio |
| **NFM** | Two-way radio, repeaters |
| **AM** | Aircraft, some broadcast |
| **USB/LSB** | Ham radio, SSB |
| **DIGITAL** | Digital signals (varies) |
| **SPEC** | Spectrum viewing only |

## Troubleshooting

### "Unknown band" shown

**Cause**: Frequency not in database

**Solution**:
- Try nearby frequencies
- Check if frequency is correct
- Add custom entry (see README.md)

### No antenna recommendations

**Cause**: ANTENNAS.TXT not found

**Solution**:
- Check SD card has WHIPCALC/ANTENNAS.TXT
- Default antennas will be shown

### Settings seem wrong

**Cause**: May be regional variant

**Solution**:
- Check region shown
- Settings are general starting points
- Adjust based on your needs

## Keyboard Shortcuts

- **Up/Down**: Adjust frequency
- **Left/Right**: Change digit
- **Enter**: Confirm frequency
- **Back**: Return to previous screen

## Adding Your Own Bands

Want to add custom frequencies?

1. On SD card, edit: `FREQMAN/FREQSUGGESTIONS.TXT`
2. Add a line in this format:
   ```
   start_hz,end_hz,name,region,usage,mode1,mode2,bw_min,bw_max,lna,vga,amp,description
   ```
3. Example:
   ```
   162400000,162550000,NOAA WX,US,Weather,NFM,MULTI,12500,25000,20,24,0,NOAA Weather Radio
   ```
4. Save and restart app

## Database Coverage

The app knows about:

**Broadcast**:
- AM Radio (530 kHz - 1.7 MHz)
- Shortwave (3-30 MHz)
- FM Radio (88-108 MHz)
- TV bands

**Amateur Radio**:
- 160m, 80m, 40m, 30m, 20m, 17m, 15m, 10m (HF)
- 6m, 2m (VHF)
- 70cm, 33cm, 23cm (UHF/SHF)

**Services**:
- Aircraft (118-137 MHz)
- Marine VHF (156-162 MHz)
- Weather (162.4-162.55 MHz)
- CB Radio (27 MHz)
- FRS/GMRS (462-467 MHz)
- PMR446 (446 MHz)

**ISM Bands**:
- 433 MHz (EU)
- 868 MHz (EU)
- 915 MHz (US)
- 2.4 GHz (Global)

**Cellular/Satellite**:
- GSM 900, 1800, 1900
- LTE bands
- GPS, GLONASS, Galileo
- Iridium, Inmarsat

**And many more!** (150+ bands total)

## Next Steps

### Learn More
- Read full README.md for detailed info
- Check TEST_DOCUMENTATION.md for technical details
- Browse FREQSUGGESTIONS.TXT for all bands

### Get Involved
- Add bands for your region
- Share your custom database
- Report issues on GitHub
- Suggest improvements

### Integration
- Use with Antenna Length app
- Combine with Frequency Manager
- Apply settings to receiver modes

## Need Help?

- **Documentation**: See README.md in app folder
- **Issues**: GitHub issue tracker
- **Community**: PortaPack Discord
- **Wiki**: Check PortaPack Mayhem wiki

## Version

Current Version: **1.0.0**
Release Date: **January 2026**

---

**Happy frequency hunting!** 📻
