#include "flex_app.hpp"

#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"
#include "string_format.hpp"
#include "file.hpp"
#include "memory_map.hpp"
#include "usb_serial_asyncmsg.hpp"

using namespace portapack;

namespace ui {

FlexAppView::FlexAppView(NavigationView& nav)
    : nav_{nav} {
    // Load from SD card (external image) to save flash space
    File file;
    auto error = file.open("/PFLX.BIN");
    if (!error) {
        if (file.size() <= portapack::memory::map::m4_code.size()) {
             // Read in chunks to avoid max transaction size limit
             auto file_size = file.size();
             auto buffer = reinterpret_cast<uint8_t*>(portapack::memory::map::m4_code.base());
             bool read_ok = true;
             
             for (size_t i = 0; i < file_size; i += 512) {
                 size_t bytes_to_read = 512;
                 if (i + bytes_to_read > file_size) {
                     bytes_to_read = file_size - i;
                 }
                 auto result = file.read(buffer + i, bytes_to_read);
                 if (result.is_error()) {
                     read_ok = false;
                     break;
                 }
             }

             if (read_ok) {
                 baseband::run_prepared_image(portapack::memory::map::m4_code.base());
                 UsbSerialAsyncmsg::asyncmsg("DBG: Flex Image Loaded");
             } else {
                 UsbSerialAsyncmsg::asyncmsg("DBG: Flex Image Read Fail");
             }
        } else {
             UsbSerialAsyncmsg::asyncmsg("DBG: Flex Image Size Error");
        }
    } else {
        // Fallback or error - try internal if present (compatibility)
        UsbSerialAsyncmsg::asyncmsg("DBG: PFLX.BIN Not Found - Using Internal");
        baseband::run_image(portapack::spi_flash::image_tag_flex);
    }

    add_children({
        &field_frequency,
        &field_rf_amp,
        &field_lna,
        &field_vga,
        &rssi,
        &console
    });

    field_frequency.set_value(receiver_model.target_frequency());
    // Use 'updated' callback for RxFrequencyField instead of on_change (which is private)
    field_frequency.updated = [this](rf::Frequency f) {
        update_freq(f);
    };
    // No need to set on_edit, RxFrequencyField handles it internally via navigation view
    
    // Enable radio
    receiver_model.set_sampling_rate(3072000);
    receiver_model.set_baseband_bandwidth(1750000); // 1.75MHz filter or similar
    receiver_model.enable();
    receiver_model.set_squelch_level(0); // Open squelch for now or use NFM squelch
    
    // Configure baseband (optional if defaults are good)
    baseband::set_flex_config(); 
    
    // Hook up message handler
    baseband::set_spectrum(0, 0); // Disable spectrum to save bandwidth if needed
}

// NOTE: moved on_debug to after constructor to match header declaration order if strict, but order in cpp doesn't matter as much.
// Already removed duplicate in previous step.
// Ensure correct namespace closing.


FlexAppView::~FlexAppView() {
    receiver_model.disable();
    baseband::shutdown();
}

void FlexAppView::focus() {
    field_frequency.focus();
}

void FlexAppView::update_freq(rf::Frequency f) {
    receiver_model.set_target_frequency(f);
    
    std::string debug_text = "Freq: " + to_string_dec_uint(f);
    UsbSerialAsyncmsg::asyncmsg(debug_text);
}

void FlexAppView::on_packet(const FlexPacketMessage* message) {
    std::string text = "";
    
    // Format timestamp
    // text += ...
    
    text += "FLEX ";
    text += to_string_dec_uint(message->packet.bitrate);
    text += " ";
    text += to_string_dec_uint(message->packet.capcode);
    text += ": ";
    text += message->packet.message;
    
    console.writeln(text);
    
    // Serial debug output
    std::string debug_text = "FLEX packet: " + text;
    UsbSerialAsyncmsg::asyncmsg(debug_text);
}

void FlexAppView::on_stats(const FlexStatsMessage* /* message */) {
    // Update status bar or similar
}

void FlexAppView::on_debug(const FlexDebugMessage* message) {
    std::string text = "DBG: ";
    text += message->text;
    text += " " + to_string_hex(message->val1, 8);
    text += " " + to_string_hex(message->val2, 8);
    UsbSerialAsyncmsg::asyncmsg(text);
}

} /* namespace ui */
