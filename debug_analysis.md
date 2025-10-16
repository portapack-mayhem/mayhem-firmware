## DEBUG: Mini Spectrum Implementation Analysis

### Current state verification:
1. Checking file integrity and includes
2. Verifying constants and patterns  
3. Comparing with working implementations

### Key constants to check:
- MINI_SPECTRUM_WIDTH (120)
- MINI_SPECTRUM_HEIGHT (8) 
- MINI_SPECTRUM_Y_START (180)

### Need to fix based on Looking Glass pattern:
- Scroll area setup in on_show()
- Waterfall rendering methodology  
- Coordinate system alignment

### Missing improvements:
- Fractional bin processing efficiency
- Memory optimization
- Runtime bounds checking removal
### Working Search Pattern (from ui_search.cpp):
- baseband::spectrum_streaming_start() in on_show()
- baseband::spectrum_streaming_stop() in on_hide()
- on_channel_spectrum callback for processing
- Direct pixel rendering for spectrum display
### Working Looking Glass Pattern (from ui_looking_glass_app.cpp):
- display.scroll_set_area(109, screen_height - 1) in on_show()
- Fractional bin processing: bins_hz_size += each_bin_size
- Scroll area rendering: display.draw_pixels({0, display.scroll(1)}, ...)
- Mode-specific processing (singlepass vs fastscan)
### EDA Current Broken Pattern:
- No scroll area setup
- Manual streaming control (BAD)
- Fixed array bounds processing 
- Y coordinate calculation instead of scroll API
