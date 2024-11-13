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
                  &button_browse

    });

    button_startstop.on_select = [this](Button&) {
        if (is_running) {
            is_running = false;
            stop();
        } else {
            if (start()) is_running = true;
        }
    };

    button_browse.on_select = [this](Button&) {
        auto open_view = nav_.push<FileLoadView>(".sub");
        open_view->push_dir(subghz_dir);
        open_view->on_changed = [this](std::filesystem::path new_file_path) {
            on_file_changed(new_file_path);
        };
    };
}

bool FlipperTxView::on_file_changed(std::filesystem::path new_file_path) {
    auto ret = read_flippersub_file(new_file_path);
    filename = "";
    if (!ret.is_valid()) {
        field_filename.set_text("File: err, bad file");
        return false;
    }
    proto = ret.value().protocol;
    if (proto != FLIPPER_PROTO_BINRAW && proto != FLIPPER_PROTO_RAW) {
        field_filename.set_text("File: err, not supp. proto");
        return false;
    }
    preset = ret.value().preset;
    if (preset != FLIPPER_PRESET_OOK) {
        field_filename.set_text("File: err, not supp. preset");
        return false;
    }
    te = ret.value().te;
    binraw_bit_count = ret.value().binraw_bit_count;
    if (binraw_bit_count >= 512 * 8) {
        field_filename.set_text("File: err, too long");  // should stream, but not in this scope YET
        return false;
    }
    field_filename.set_text("File: " + new_file_path.string());
    filename = new_file_path;
    return true;
}

void FlipperTxView::stop() {
    replay_thread.reset();
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
        512, 3,
        &ready_signal,
        [](uint32_t return_code) {
            ReplayThreadDoneMessage message{return_code};
            EventDispatcher::send_message(message);
        });
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
    is_running = false;
    stop();
}

FlipperPlayThread::FlipperPlayThread(
    std::filesystem::path filename,
    size_t read_size,
    size_t buffer_count,
    bool* ready_signal,
    std::function<void(uint32_t return_code)> terminate_callback)
    : config{read_size, buffer_count},
      filename{filename},
      ready_sig{ready_signal},
      terminate_callback{std::move(terminate_callback)} {
    thread = chThdCreateFromHeap(NULL, 1 * 1024, NORMALPRIO + 10, FlipperPlayThread::static_fn, this);
}

FlipperPlayThread::~FlipperPlayThread() {
    if (thread) {
        chThdTerminate(thread);
        chThdWait(thread);
        thread = nullptr;
    }
}

msg_t FlipperPlayThread::static_fn(void* arg) {
    auto obj = static_cast<FlipperPlayThread*>(arg);
    const auto return_code = obj->run();
    if (obj->terminate_callback) {
        obj->terminate_callback(return_code);
    }
    return 0;
}

uint32_t FlipperPlayThread::run() {
    BasebandReplay replay{&config};
    BufferExchange buffers{&config};

    StreamBuffer* prefill_buffer{nullptr};
    int32_t read_result = 0;
    // read file base stuff
    auto fsub = read_flippersub_file(filename);
    if (!fsub.is_valid()) return READ_ERROR;
    // assume it is ok, since prev we checked it.
    auto proto = fsub.value().protocol;

    // open the sub file
    File f;
    auto error = f.open(filename);
    if (error) return READ_ERROR;
    // seek to the first data
    if (proto == FLIPPER_PROTO_RAW)
        seek_flipper_raw_first_data(f);
    else
        seek_flipper_binraw_first_data(f);

    // Wait for FIFOs to be allocated in baseband
    // Wait for ui_replay_view to tell us that the buffers are ready (awful :( )
    while (!(*ready_sig)) {
        chThdSleep(100);
    };
    return END_OF_FILE;
    constexpr size_t block_size = 64;  // 64/4 = 16 data per block. must be abe to divide it with 8!
    bool readall = false;
    // While empty buffers fifo is not empty...
    while (!buffers.empty()) {
        prefill_buffer = buffers.get_prefill();

        if (prefill_buffer == nullptr) {
            buffers.put_app(prefill_buffer);
        } else {
            size_t blocks = config.read_size / block_size;  //

            for (size_t c = 0; c < blocks; c++) {
                for (size_t d = 0; d < 512; d += 4) {
                    read_result = 0;
                    if (!readall) {
                        if (proto == FLIPPER_PROTO_RAW) {
                            auto data = read_flipper_raw_next_data(f);
                            if (!data.is_valid())
                                readall = true;
                            else {
                                read_result = data.value();
                                ((int32_t*)prefill_buffer->data())[c * block_size + d] = read_result;
                            }

                        } else {
                            auto data = read_flipper_binraw_next_data(f);
                            if (!data.is_valid())
                                readall = true;
                            else {
                                read_result = data.value();
                                // add 8 data
                                for (uint8_t bit = 0; bit < 8; bit++) {
                                    ((int32_t*)prefill_buffer->data())[c * block_size + d + bit] = get_flipper_binraw_bitvalue(read_result, bit);
                                }
                            }
                        }
                    } else {
                        ((int32_t*)prefill_buffer->data())[c * block_size + d] = 0;
                    }
                }
            }
            prefill_buffer->set_size(config.read_size);
            buffers.put(prefill_buffer);
        }
    };
    if (readall) return END_OF_FILE;

    baseband::set_fifo_data(nullptr);

    while (!chThdShouldTerminate()) {
        auto buffer = buffers.get();
        readall = false;
        int32_t read_result = 0;
        for (size_t d = 0; d < buffer->capacity(); d += 4) {
            read_result = 0;
            if (!readall) {
                if (proto == FLIPPER_PROTO_RAW) {
                    auto data = read_flipper_raw_next_data(f);
                    if (!data.is_valid())
                        readall = true;
                    else
                        read_result = data.value();
                } else {
                    auto data = read_flipper_binraw_next_data(f);
                    if (!data.is_valid())
                        readall = true;
                    else
                        read_result = data.value();
                }
            }
            ((int32_t*)buffer->data())[d] = read_result;
        }
        buffer->set_size(buffer->capacity());
        buffers.put(buffer);
        if (readall) return END_OF_FILE;
    }

    return TERMINATED;
}

}  // namespace ui::external_app::flippertx
