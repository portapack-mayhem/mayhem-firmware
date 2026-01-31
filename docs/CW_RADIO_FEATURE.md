# CW Radio Feature - Low-Power Morse Code Transmitter

## Overview
The CW Radio feature turns your PortaPack H2 into a low-power CW (Morse code) transmitter suitable for short-range communication practice. This application allows operators to practice sending Morse code using either the touchscreen button, encoder wheel, or an external telegraph key.

## Features

### Core Functionality
- **Multiple Modulation Modes**: Supports AM, FM, DSB, USB, and LSB modulation
- **Adjustable Audio Tone**: Configure sidetone frequency from 300 Hz to 1200 Hz (default: 700 Hz)
- **Precise Frequency Control**: 50 Hz frequency steps for accurate CW operation
- **Low-Power Operation**: Configurable power levels suitable for practice and short-range communication
- **Real-time Key State Display**: Visual feedback showing when the key is down

### Input Methods
1. **Touchscreen Button**: Press and hold the "PRESS TO KEY" button to transmit
2. **Encoder Wheel**: Rotate the encoder to send brief key-down pulses
3. **External Telegraph Key** (future enhancement): Connect a traditional telegraph key to GPIO pins for authentic CW operation

### Modulation Modes
- **AM (Amplitude Modulation)**: Classic on-off keying with carrier modulation
- **FM (Frequency Modulation)**: Frequency-shift keying with adjustable deviation (1-25 kHz)
- **DSB (Double Sideband)**: Both sidebands transmitted
- **USB (Upper Sideband)**: Upper sideband only
- **LSB (Lower Sideband)**: Lower sideband only

## Usage Instructions

### Getting Started
1. Navigate to **Transmit** → **CW Radio** from the main menu
2. Set your desired frequency (default: 7.040 MHz - 40m CW band)
3. Select modulation mode (AM recommended for beginners)
4. Adjust sidetone frequency if desired (700 Hz is standard)
5. Press "START" in the transmitter control section

### Operating the CW Radio
1. **Frequency Selection**:
   - Use the frequency field to enter your target frequency
   - Click on the frequency to use the keypad for direct entry
   - Use 50 Hz steps for precise tuning

2. **Mode Selection**:
   - Choose from AM, FM, DSB, USB, or LSB
   - For FM mode, set the deviation (default: 5000 Hz)

3. **Keying**:
   - Press and hold the "PRESS TO KEY" button to transmit
   - The status will show "KEY DOWN" while transmitting
   - Release to stop transmitting

4. **Power Control**:
   - Adjust gain and amplifier settings in the transmitter view
   - Keep power low for practice sessions
   - Higher power increases range but requires appropriate licensing

### Recommended Frequencies for Practice
- **7.040 MHz**: 40m band CW segment (license required)
- **14.060 MHz**: 20m band CW segment (license required)
- **21.060 MHz**: 15m band CW segment (license required)

**Important**: Always operate within your amateur radio license class privileges and follow local regulations.

## Technical Specifications

### Frequency Range
- **VHF/UHF**: Full range supported by HackRF hardware
- **HF**: 1 MHz - 30 MHz (with appropriate upconverter)

### Audio Tone Range
- **Minimum**: 300 Hz
- **Maximum**: 1200 Hz
- **Default**: 700 Hz
- **Standard**: 600-800 Hz for CW operation

### Modulation Parameters
- **Bandwidth**: 150 kHz (suitable for CW)
- **Sampling Rate**: 1.536 MHz
- **FM Deviation**: 1-25 kHz (adjustable, FM mode only)

### Key Timing
- **Manual Button**: User-controlled key-down duration
- **Encoder**: ~100ms brief pulses per encoder movement

## Hardware Connections (Future Enhancement)

### External Telegraph Key Connection
For connecting a traditional telegraph key, the following GPIO connections can be used:

**Option 1: Using Switch Input**
- Connect key to P2 header (to be implemented)
- Pin 1: Ground
- Pin 2: Key input (with internal pull-up)

**Option 2: Using Encoder Input**
- Connect key to encoder switch input
- Provides interrupt-driven key detection

### Recommended Keys
- J-38 Telegraph Key
- Straight keys
- Iambic paddles (future enhancement with keyer logic)

## Safety and Legal Considerations

### Legal Operation
1. **Licensing**: CW operation on amateur radio frequencies requires an appropriate amateur radio license
2. **Frequency Selection**: Ensure you operate within authorized frequency bands
3. **Power Limits**: Observe power limits for your license class
4. **Identification**: When operating on amateur bands, identify your station per regulations

### Safety
1. **Low Power**: Start with low power settings for practice
2. **Antenna**: Use appropriate antenna for your frequency
3. **Monitoring**: Listen before transmitting to avoid interference
4. **Short Transmissions**: Keep practice sessions brief

## Troubleshooting

### No Transmission
- Verify transmitter is enabled (press START)
- Check antenna connection
- Verify frequency is within HackRF range
- Check power settings

### Weak Signal
- Increase TX gain
- Enable RF amplifier
- Check antenna and connections
- Verify frequency selection

### No Audio Sidetone
- Check volume settings
- Verify audio output is enabled
- Adjust tone frequency

### Button Not Responding
- Ensure transmitter is started first
- Check touchscreen calibration
- Try encoder wheel as alternative input

## Future Enhancements

### Planned Features
1. **External Key Input**: GPIO support for telegraph keys
2. **Keyer Logic**: Built-in iambic keyer for paddle operation
3. **CW Decoder**: Real-time Morse code decoding
4. **Memory Keyer**: Store and replay CW messages
5. **Speed Control**: Adjustable WPM (words per minute) for automatic keying
6. **Practice Mode**: Built-in Morse code training with receive/transmit practice
7. **Contest Mode**: Quick frequency change and logging

### Hardware Additions
- 3.5mm key jack for external key input
- Paddle jack for iambic keyers
- PTT control output

## Technical Notes

### Baseband Processor
The CW Radio uses the `proc_morsetx` baseband processor which provides:
- Real-time key state control
- Multiple modulation modes
- Sidetone generation
- Low-latency response

### Message Flow
1. User presses key button
2. UI sends `MorseTXkeyMessage` to baseband
3. Baseband generates appropriate RF signal
4. Status updates displayed in real-time

## Credits
- Based on existing PortaPack Mayhem Morse code infrastructure
- Leverages `proc_morsetx.cpp` baseband processor
- Integrated with standard transmitter framework

## Version History
- **v1.0** (2026): Initial release with touchscreen and encoder key input
  - AM, FM, DSB, USB, LSB modulation modes
  - Adjustable sidetone and frequency
  - Low-power operation for practice

---

**73 and Happy CW Operating!**

For questions, issues, or suggestions, please visit the PortaPack Mayhem GitHub repository.
