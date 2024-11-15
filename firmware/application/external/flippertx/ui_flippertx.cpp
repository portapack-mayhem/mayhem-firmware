/*
 * Copyright (C) 2024 HTotoo
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "ui_flippertx.hpp"
#include "audio.hpp"
#include "rtc_time.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"
#include "buffer_exchange.hpp"

using namespace portapack;
using namespace ui;

namespace ui::external_app::flippertx {

void FlipperTxView::focus() {
    button_browse.focus();
}

void FlipperTxView::set_ready() {
    ready_signal = true;
}

FlipperTxView::FlipperTxView(NavigationView& nav)
    : nav_{nav} {
    add_children({&button_startstop,
                  &field_frequency,
                  &tx_view,
                  &field_filename,
                  &button_browse});

    button_startstop.on_select = [this](Button&) {
        if (is_running) {
            stop();
        } else {
            start();
        }
    };

    button_browse.on_select = [this](Button&) {
        auto open_view = nav_.push<FileLoadView>(".sub");
        open_view->push_dir(subghz_dir);
        open_view->on_changed = [this](std::filesystem::path new_file_path) {
            if (on_file_changed(new_file_path)) {
                ;  // tif (button_startstop.parent()) button_startstop.focus(); parent_ is null error.....
            }
        };
    };
}

bool FlipperTxView::on_file_changed(std::filesystem::path new_file_path) {
    submeta = read_flippersub_file(new_file_path);
    filename = "";
    if (!submeta.is_valid()) {
        field_filename.set_text("File: err, bad file");
        return false;
    }
    proto = submeta.value().protocol;
    if (proto != FLIPPER_PROTO_RAW && proto != FLIPPER_PROTO_BINRAW) {  // temp disabled
        field_filename.set_text("File: err, not supp. proto");
        return false;
    }
    preset = submeta.value().preset;
    if (preset != FLIPPER_PRESET_OOK) {
        field_filename.set_text("File: err, not supp. preset");
        return false;
    }
    te = submeta.value().te;
    binraw_bit_count = submeta.value().binraw_bit_count;
    /*if (binraw_bit_count >= 512 * 800) {
        field_filename.set_text("File: err, too long");  // should stream, but not in this scope YET
        return false;
    }*/
    field_filename.set_text("File: " + new_file_path.string());
    filename = new_file_path;
    return true;
}

void FlipperTxView::stop() {
    if (is_running && replay_thread) replay_thread.reset();
    is_running = false;
    ready_signal = false;

    transmitter_model.disable();
    baseband::shutdown();
    button_startstop.set_text(LanguageHelper::currentMessages[LANG_START]);
}

bool FlipperTxView::start() {
    if (filename.empty()) return false;
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());
    transmitter_model.enable();
    button_startstop.set_text(LanguageHelper::currentMessages[LANG_STOP]);
    // start thread
    replay_thread = std::make_unique<FlipperPlayThread>(
        filename,
        1024, 4,
        &ready_signal,
        submeta.value().protocol,
        submeta.value().te,
        [](uint32_t return_code) {
            ReplayThreadDoneMessage message{return_code};
            EventDispatcher::send_message(message);
        });
    is_running = true;
    return true;
}

void FlipperTxView::on_tx_progress(const bool done) {
    if (done) {
        if (is_running) {
            stop();
        }
    }
}

FlipperTxView::~FlipperTxView() {
    stop();
}

FlipperPlayThread::FlipperPlayThread(
    std::filesystem::path filename,
    size_t read_size,
    size_t buffer_count,
    bool* ready_signal,
    FlipperProto proto,
    uint16_t te,
    std::function<void(uint32_t return_code)> terminate_callback)
    : config{read_size, buffer_count},
      filename{filename},
      ready_sig{ready_signal},
      proto{proto},
      te{te},
      terminate_callback{std::move(terminate_callback)} {
    thread = chThdCreateFromHeap(NULL, 6 * 1024, NORMALPRIO + 10, FlipperPlayThread::static_fn, this);
}

FlipperPlayThread::~FlipperPlayThread() {
    if (thread) {
        chThdTerminate(thread);
        if (thread->p_refs) chThdWait(thread);
        thread = nullptr;
    }
}

msg_t FlipperPlayThread::static_fn(void* arg) {
    auto obj = static_cast<FlipperPlayThread*>(arg);
    const auto return_code = obj->run();
    if (obj->terminate_callback) {
        obj->terminate_callback(return_code);
    }
    chThdExit(0);
    return 0;
}

uint32_t FlipperPlayThread::run() {
    BasebandReplay replay{&config};
    BufferExchange buffers{&config};
    StreamBuffer* prefill_buffer{nullptr};
    int32_t read_result = 0;
    // assume it is ok, since prev we checked it.
    // open the sub file
    File f;  // don't worry, destructor will close it.
    auto error = f.open(filename);
    if (error) return READ_ERROR;
    // seek to the first data
    if (proto == FLIPPER_PROTO_RAW)
        seek_flipper_raw_first_data(f);
    else {
        seek_flipper_binraw_first_data(f);
    }
    // Wait for FIFOs to be allocated in baseband
    // Wait for _view to tell us that the buffers are ready
    while (!(*ready_sig) && !chThdShouldTerminate()) {
        chThdSleep(100);
    };
    uint8_t readall = 0;
    int32_t endsignals[3] = {0, 42069, 613379};  // unlikely a valid data
    // While empty buffers fifo is not empty...
    while (!buffers.empty() && !chThdShouldTerminate()) {
        prefill_buffer = buffers.get_prefill();

        if (prefill_buffer == nullptr) {
            buffers.put_app(prefill_buffer);
        } else {
            size_t c = 0;
            for (c = 0; c < config.read_size / 4;) {
                read_result = 0;
                if (0 == readall) {
                    if (proto == FLIPPER_PROTO_RAW) {
                        auto data = read_flipper_raw_next_data(f);
                        if (!data.is_valid()) {
                            ((int32_t*)prefill_buffer->data())[c] = endsignals[++readall];
                        } else {
                            read_result = data.value();
                            ((int32_t*)prefill_buffer->data())[c] = read_result;
                        }
                        c++;

                    } else {  // BINRAW
                        auto data = read_flipper_binraw_next_data(f);
                        if (!data.is_valid())
                            readall = 1;
                        else {
                            read_result = data.value();
                            // add 8 data
                            for (uint8_t bit = 0; bit < 8; bit++) {
                                ((int32_t*)prefill_buffer->data())[c + bit] = ((get_flipper_binraw_bitvalue(read_result, (7 - bit)) ? 1 : -1) * te);
                                c++;
                            }
                        }
                    }
                } else {
                    ((int32_t*)prefill_buffer->data())[c] = endsignals[readall];
                    if (readall == 1) readall++;
                    c++;
                }
            }
            prefill_buffer->set_size(config.read_size);
            buffers.put(prefill_buffer);
        }
    };
    baseband::set_fifo_data(nullptr);
    if (readall) return END_OF_FILE;

    while (!chThdShouldTerminate()) {
        auto buffer = buffers.get();
        int32_t read_result = 0;
        for (size_t d = 0; d < buffer->capacity() / 4; d++) {
            read_result = -133469;
            if (!readall) {
                if (proto == FLIPPER_PROTO_RAW) {
                    auto data = read_flipper_raw_next_data(f);
                    if (!data.is_valid()) {
                        readall = 1;
                    } else {
                        read_result = data.value();
                    }
                } else {
                    auto data = read_flipper_binraw_next_data(f);
                    if (!data.is_valid())
                        readall = 1;
                    else {
                        read_result = data.value();
                        // add 8 data
                        for (uint8_t bit = 0; bit < 8; bit++) {
                            ((int32_t*)prefill_buffer->data())[d + bit] = ((get_flipper_binraw_bitvalue(read_result, (7 - bit)) ? 1 : -1) * te);
                        }
                        d += 7;
                        continue;
                    }
                }
            }
            if (readall >= 1 && readall <= 2) {
                read_result = endsignals[readall++];
            }
            ((int32_t*)buffer->data())[d] = read_result;
        }
        buffer->set_size(buffer->capacity());
        buffers.put(buffer);
        if (readall == 3) return END_OF_FILE;
    }
    return TERMINATED;
}

}  // namespace ui::external_app::flippertx
