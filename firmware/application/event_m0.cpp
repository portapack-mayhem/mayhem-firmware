/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "event_m0.hpp"

#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"
#include "debug.hpp"

#include "sd_card.hpp"
#include "rtc_time.hpp"

#include "message.hpp"
#include "message_queue.hpp"

#include "irq_controls.hpp"

#include "buffer_exchange.hpp"

#include "ch.h"

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

#include <array>

#include "ui_navigation.hpp"

extern "C" {

CH_IRQ_HANDLER(M4Core_IRQHandler) {
    CH_IRQ_PROLOGUE();

    chSysLockFromIsr();
    BufferExchange::handle_isr();
    EventDispatcher::check_fifo_isr();
    chSysUnlockFromIsr();

    creg::m4txevent::clear();

    CH_IRQ_EPILOGUE();
}
}

class MessageHandlerMap {
   public:
    using MessageHandler = std::function<void(Message* const p)>;

    void register_handler(const Message::ID id, MessageHandler&& handler) {
        if (map_[toUType(id)] != nullptr) {
            chDbgPanic("MsgDblReg");
        }
        map_[toUType(id)] = std::move(handler);
    }

    void unregister_handler(const Message::ID id) {
        map_[toUType(id)] = nullptr;
    }

    void send(Message* const message) {
        if (message->id < Message::ID::MAX) {
            auto& fn = map_[toUType(message->id)];
            if (fn) {
                fn(message);
            }
        }
    }

   private:
    using MapType = std::array<MessageHandler, toUType(Message::ID::MAX)>;
    MapType map_{};
};

static MessageHandlerMap message_map;
Thread* EventDispatcher::thread_event_loop = nullptr;
bool EventDispatcher::is_running = false;
bool EventDispatcher::display_sleep = false;

EventDispatcher::EventDispatcher(
    ui::Widget* const top_widget,
    ui::Context& context)
    : top_widget{top_widget},
      painter{},
      context(context) {
    init_message_queues();

    thread_event_loop = chThdSelf();
    is_running = true;
    touch_manager.on_event = [this](const ui::TouchEvent event) {
        this->on_touch_event(event);
    };
}

void EventDispatcher::run() {
    while (is_running) {
        const auto events = wait();
        dispatch(events);
        portapack::usb_serial.dispatch();
    }
}

void EventDispatcher::request_stop() {
    is_running = false;
}

void EventDispatcher::set_display_sleep(const bool sleep) {
    // TODO: Distribute display sleep message more broadly, shut down data generation
    // on baseband side, since all that data is being discarded during sleep.
    if (sleep) {
        portapack::backlight()->off();
        portapack::display.sleep();
    } else {
        portapack::display.wake();
        // Don't turn on backlight here.
        // Let frame sync handler turn on backlight after repaint.
    }
    EventDispatcher::display_sleep = sleep;
};

eventmask_t EventDispatcher::wait() {
    return chEvtWaitAny(ALL_EVENTS);
}

void EventDispatcher::dispatch(const eventmask_t events) {
    if (shared_memory.m4_panic_msg[0] != 0) {
        if (shared_memory.bb_data.data[0] == 0)
            draw_guru_meditation(CORTEX_M4, shared_memory.m4_panic_msg);
        else
            draw_guru_meditation(
                CORTEX_M4,
                shared_memory.m4_panic_msg,
                (struct extctx*)&shared_memory.bb_data.data[8],
                *(uint32_t*)&shared_memory.bb_data.data[4]);
    }

    if (events & EVT_MASK_APPLICATION) {
        handle_application_queue();
    }

    if (events & EVT_MASK_LOCAL) {
        handle_local_queue();
    }

    if (events & EVT_MASK_RTC_TICK) {
        handle_rtc_tick();
    }

    if (events & EVT_MASK_SWITCHES) {
        handle_switches();
    }

    /*if( events & EVT_MASK_LCD_FRAME_SYNC ) {
                blink_timer();
        }*/

    if (!EventDispatcher::display_sleep) {
        if (events & EVT_MASK_LCD_FRAME_SYNC) {
            handle_lcd_frame_sync();
        }

        if (events & EVT_MASK_ENCODER) {
            handle_encoder();
        }

        if (events & EVT_MASK_TOUCH) {
            handle_touch();
        }
    }
}

void EventDispatcher::handle_application_queue() {
    shared_memory.application_queue.handle([](Message* const message) {
        message_map.send(message);
    });
}

void EventDispatcher::handle_local_queue() {
    shared_memory.app_local_queue.handle([](Message* const message) {
        message_map.send(message);
    });
}

void EventDispatcher::handle_rtc_tick() {
    sd_card::poll_inserted();

    portapack::temperature_logger.second_tick();

    const auto backlight_timer = portapack::persistent_memory::config_backlight_timer();
    if (backlight_timer.timeout_enabled()) {
        if (portapack::bl_tick_counter == backlight_timer.timeout_seconds())
            set_display_sleep(true);
        else
            portapack::bl_tick_counter++;
    }

    rtc_time::on_tick_second();

    portapack::persistent_memory::cache::persist();
}

ui::Widget* EventDispatcher::touch_widget(ui::Widget* const w, ui::TouchEvent event) {
    if (!w->hidden()) {
        // To achieve reverse depth ordering (last object drawn is
        // considered "top"), descend first.
        for (const auto child : w->children()) {
            const auto touched_widget = touch_widget(child, event);
            if (touched_widget) {
                return touched_widget;
            }
        }

        const auto r = w->screen_rect();
        if (r.contains(event.point)) {
            if (w->on_touch(event)) {
                // This widget responded. Return it up the call stack.
                return w;
            }
        }
    }
    return nullptr;
}

void EventDispatcher::emulateTouch(ui::TouchEvent event) {
    on_touch_event(event);
}

void EventDispatcher::emulateKeyboard(ui::KeyboardEvent event) {
    on_keyboard_event(event);
}

void EventDispatcher::on_keyboard_event(ui::KeyboardEvent event) {
    // send the key to focused widget, or parent if not accepts it
    auto target = context.focus_manager().focus_widget();
    while ((target != nullptr) && !target->on_keyboard(event)) {
        target = target->parent();
    }
}

void EventDispatcher::on_touch_event(ui::TouchEvent event) {
    /* TODO: Capture widget receiving the Start event, send Move and
     * End events to the same widget.
     */
    /* Capture Start widget.
     * If touch is over Start widget at Move event, then the widget
     * should be highlighted. If the touch is not over the Start
     * widget at Move event, widget should un-highlight.
     * If touch is over Start widget at End event, then the widget
     * action should occur.
     */
    if (event.type == ui::TouchEvent::Type::Start) {
        captured_widget = touch_widget(this->top_widget, event);
    }

    if (captured_widget) {
        captured_widget->on_touch(event);
    }
}

ui::Widget* EventDispatcher::getTopWidget() {
    return top_widget;
}

ui::Widget* EventDispatcher::getFocusedWidget() {
    return context.focus_manager().focus_widget();
}

void EventDispatcher::handle_lcd_frame_sync() {
    DisplayFrameSyncMessage message;
    message_map.send(&message);

    static_cast<ui::SystemView*>(top_widget)->paint_overlay();
    painter.paint_widget_tree(top_widget);

    portapack::backlight()->on();
}

void EventDispatcher::handle_switches() {
    const auto switches_state = get_switches_state();

    portapack::bl_tick_counter = 0;

    if (switches_state.count() == 0) {
        // If all keys are released, we are no longer in a key event.
        in_key_event = false;
    }

    if (in_key_event) {
        if (switches_state[(size_t)ui::KeyEvent::Left] && switches_state[(size_t)ui::KeyEvent::Up]) {
            const auto event = static_cast<ui::KeyEvent>(ui::KeyEvent::Back);
            context.focus_manager().update(top_widget, event);
        }

        // If we're in a key event, return. We will ignore all additional key
        // presses until the first key is released. We also want to ignore events
        // where the last key held generates a key event when other pressed keys
        // are released.
        return;
    }

    if (EventDispatcher::display_sleep) {
        // Swallow event, wake up display.
        if (switches_state.any()) {
            set_display_sleep(false);
        }
        return;
    }

    for (size_t i = 0; i < switches_state.size(); i++) {
        // TODO: Ignore multiple keys at the same time?
        if (switches_state[i]) {
            const auto event = static_cast<ui::KeyEvent>(i);
            if (!event_bubble_key(event)) {
                if (switches_state[(size_t)ui::KeyEvent::Dfu]) {
                    static_cast<ui::SystemView*>(top_widget)->toggle_overlay();
                } else {
                    context.focus_manager().update(top_widget, event);
                }
            }

            in_key_event = true;
        }
    }
}

void EventDispatcher::handle_encoder() {
    portapack::bl_tick_counter = 0;

    if (EventDispatcher::display_sleep) {
        // Swallow event, wake up display.
        set_display_sleep(false);
        return;
    }

    const uint32_t encoder_now = get_encoder_position();
    const int32_t delta = static_cast<int32_t>(encoder_now - encoder_last);
    encoder_last = encoder_now;
    const auto event = static_cast<ui::EncoderEvent>(delta);
    event_bubble_encoder(event);
}

void EventDispatcher::handle_touch() {
    portapack::bl_tick_counter = 0;

    touch_manager.feed(get_touch_frame());
}

bool EventDispatcher::event_bubble_key(const ui::KeyEvent event) {
    auto target = context.focus_manager().focus_widget();
    while ((target != nullptr) && !target->on_key(event)) {
        target = target->parent();
    }

    /* Return true if event was consumed. */
    return (target != nullptr);
}

void EventDispatcher::event_bubble_encoder(const ui::EncoderEvent event) {
    auto target = context.focus_manager().focus_widget();
    while ((target != nullptr) && !target->on_encoder(event)) {
        target = target->parent();
    }
}

void EventDispatcher::init_message_queues() {
    new (&shared_memory) SharedMemory;
}

MessageHandlerRegistration::MessageHandlerRegistration(
    const Message::ID message_id,
    MessageHandlerMap::MessageHandler&& callback)
    : message_id{message_id} {
    message_map.register_handler(message_id, std::move(callback));
}

MessageHandlerRegistration::~MessageHandlerRegistration() {
    message_map.unregister_handler(message_id);
}
