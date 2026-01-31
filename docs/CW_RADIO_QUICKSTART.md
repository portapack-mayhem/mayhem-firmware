# CW Radio Quick Start Guide

## What is CW Radio?
The CW Radio feature transforms your PortaPack H2 into a low-power CW (Morse code) transmitter for practice and short-range communication.

## Quick Access
**Main Menu → Transmit → CW Radio**

## First Time Setup (30 seconds)

### Step 1: Launch the App
1. Navigate to **Transmit** from the main menu
2. Select **CW Radio**

### Step 2: Set Frequency
- Default: **7.040 MHz** (40m CW calling frequency)
- Tap frequency field to enter custom frequency
- Use 50 Hz steps for precise tuning

### Step 3: Choose Mode
- **AM** (Recommended for beginners)
- FM, DSB, USB, LSB (advanced options)

### Step 4: Start Transmitting
1. Press **START** button in the transmitter section
2. Press and hold the **PRESS TO KEY** button to transmit
3. Release to stop transmitting
4. Status will show "KEY DOWN" when transmitting

## Key Controls

### Touchscreen Button
- **Press & Hold**: Transmit (key down)
- **Release**: Stop transmitting (key up)

### Encoder Wheel (Alternative)
- **Rotate**: Send brief key pulses
- Useful for practicing rhythm

## Settings You Can Adjust

| Setting | Range | Default | Purpose |
|---------|-------|---------|---------|
| Frequency | HackRF range | 7.040 MHz | TX frequency |
| Mode | AM/FM/DSB/USB/LSB | AM | Modulation type |
| Tone | 300-1200 Hz | 700 Hz | Sidetone pitch |
| FM Dev | 1-25 kHz | 5 kHz | FM deviation |
| TX Gain | 0-47 dB | Settings | Transmit power |

## Common Use Cases

### Practice Morse Code
1. Set frequency to practice band
2. Use low power (gain ~20 dB)
3. Key simple patterns (dits and dahs)
4. Listen to sidetone for timing

### Short-Range Communication
1. Coordinate with another operator
2. Set same frequency
3. Use appropriate power level
4. Send messages in Morse code

### QRP (Low Power) Operating
1. Licensed amateur bands only
2. Keep gain at minimum needed
3. Use appropriate antenna
4. Follow band plan

## Modes Explained

### AM (Amplitude Modulation)
- **Best for**: Beginners, simple equipment
- **Characteristics**: On-off keying, carrier modulation
- **Use when**: Learning CW, equipment compatibility

### FM (Frequency Modulation)
- **Best for**: VHF/UHF operation
- **Characteristics**: Frequency shift keying
- **Use when**: Using FM receivers, VHF bands

### SSB Modes (USB/LSB/DSB)
- **Best for**: HF DX operation
- **Characteristics**: Single/double sideband
- **Use when**: Efficient HF communication

## Safety Tips

✅ **DO:**
- Start with LOW power for practice
- Check antenna is connected
- Operate within your license privileges
- Listen before transmitting
- Keep transmissions brief during practice

❌ **DON'T:**
- Transmit without proper license (on amateur bands)
- Use high power indoors without proper antenna
- Transmit continuously for long periods
- Operate in unauthorized frequency bands

## Legal Requirements

### Amateur Radio Bands
Operating on amateur radio frequencies requires:
- Valid amateur radio license
- Proper station identification
- Operation within license class privileges
- Adherence to band plans

### License-Free Options
For practice without a license:
- Use dummy load (no antenna)
- Very low power with proper shielding
- ISM bands (check local regulations)

## Troubleshooting

### Problem: No transmission
**Solution:** Press START button first, then key button

### Problem: Can't hear sidetone
**Solution:** Check volume settings, verify audio output enabled

### Problem: Weak signal
**Solution:** Increase TX gain, check antenna connection

### Problem: Button doesn't work
**Solution:** Ensure TX is enabled (press START first)

## Tips for Better CW Operation

### Timing
- **Dit**: Brief press (~100ms at 10 WPM)
- **Dah**: Longer press (~300ms at 10 WPM)
- **Space**: Between letters (300ms)
- **Gap**: Between words (700ms)

### Good Practices
1. Start slow and build speed
2. Focus on consistent timing
3. Practice common words first
4. Use proper spacing
5. Listen to your sidetone rhythm

### Common CW Abbreviations
- **CQ**: Calling any station
- **DE**: From (used in identification)
- **K**: Invitation to transmit
- **73**: Best regards
- **88**: Love and kisses

## Advanced Features (Coming Soon)

- ⏳ External telegraph key input
- ⏳ Iambic keyer for paddles
- ⏳ Memory keyer
- ⏳ CW decoder
- ⏳ Practice mode with training
- ⏳ WPM speed control

## Getting Help

### Resources
- [Full Documentation](CW_RADIO_FEATURE.md)
- [PortaPack Mayhem Wiki](https://github.com/portapack-mayhem/mayhem-firmware/wiki)
- [Implementation Details](CW_RADIO_IMPLEMENTATION.md)

### Common Questions

**Q: Can I connect a real telegraph key?**
A: GPIO support is documented for future implementation. Currently use touchscreen.

**Q: What frequencies can I use?**
A: Any frequency within HackRF range, but amateur bands require a license.

**Q: How do I adjust the power?**
A: Use TX Gain control in the transmitter section. Start low (20 dB).

**Q: Can I send automatic messages?**
A: Not yet - manual keying only. Memory keyer is planned.

**Q: Is there a CW decoder?**
A: Not in this version. Decoder integration is planned.

---

## Example Session

```
1. Navigate to Transmit → CW Radio
2. Frequency: 7.040 MHz (default)
3. Mode: AM (default)
4. Press START
5. Press and hold key button
6. Send: ... --- ... (SOS)
   - 3 dits, 3 dahs, 3 dits
7. Release key button
8. Press STOP when done
```

---

**73 de PortaPack!** (Best regards from PortaPack!)

Ready to practice your CW skills!
