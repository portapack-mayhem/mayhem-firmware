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

#include "hackrf_gpio.hpp"
using namespace hackrf::one;

#include "gpio.hpp"

#include "adc.hpp"

#include "audio.hpp"

#include "si5351.hpp"
using namespace si5351;

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

#include "core_control.hpp"

#include "irq_rtc.hpp"

#include "i2c_lld.h"

#include <array>

#include "ui_navigation.hpp"

static int delayed_error = 0;

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
    }
}

void EventDispatcher::request_stop() {
    is_running = false;
}

void EventDispatcher::set_display_sleep(const bool sleep) {
    // TODO: Distribute display sleep message more broadly, shut down data generation
    // on baseband side, since all that data is being discarded during sleep.  -- DON'T TODO it, sincethe stealth mode want to send with screen off!
    if (sleep) {
        portapack::backlight()->off();
        portapack::display.sleep(false);  // when called the hw_sleep = true, the irq wont fire, so the EVT_MASK_LCD_FRAME_SYNC won't set.
    } else {
        portapack::display.wake(true);  // not important, command not affect if already hw waken up
        // Don't turn on backlight here.
        // Let frame sync handler turn on backlight after repaint.
    }
    EventDispatcher::display_sleep = sleep;
}

void EventDispatcher::charge_deep_sleep(const bool sleep) {
    uint8_t valid_mask = 0;
    uint8_t percent = 0;
    uint16_t voltage = 0;
    int32_t current = 0;

    constexpr I2CConfig i2c_config_12mhz{
        .high_count = 15,
        .low_count = 15,
    };

    if (sleep) {
        portapack::shutdown(false, true);
        nvicDisableVector(DMA_IRQn);
        chSysDisable();
        systick_stop();
        ShutdownMessage shutdown_message;
        shared_memory.application_queue.push(shutdown_message);
        shared_memory.baseband_message = nullptr;

        SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk;

#ifdef PRALINE
        // 1. TÁPVONALAK LEKAPCSOLÁSA
        gpio_vaa_disable.set();     // VAA_ENABLE P8_1 -> HIGH
        gpio_1v2_enable.clear();    // 1V2_E P8_7 -> LOW
        gpio_3v3aux_disable.set();  // 3V3 aux P6_7 -> HIGH
        gpio_VBUS_enable.clear();   // VBUS_IN_EN P8_4 -> LOW
        gpio_VIN_enable.set();      // VIN_IN_EN P8_5 -> HIGH

        // 2. FANTOMÁRAM MEGSZÜNTETÉSE
        LPC_GPIO->DIR[0] &= ~0xFFFF4000;
        LPC_GPIO->DIR[1] &= ~0xFFFF1000;
        LPC_GPIO->DIR[2] &= ~((1 << 14) | (1 << 13) | (1 << 12) | (1 << 11) | (1 << 10) | (1 << 6) | (1 << 0));
        LPC_GPIO->DIR[3] &= ~((1 << 7) | (1 << 5));
        LPC_GPIO->DIR[5] &= ~(1 << 16);
#else
        gpio_og_vaa_disable.set();    // VAA -> HIGH
        gpio_r9_vaa_disable.clear();  // 1V8 -> LOW
        LPC_GPIO->DIR[0] &= ~0xFFFF4000;
        LPC_GPIO->DIR[1] &= ~0xFFFF1000;
        LPC_GPIO->DIR[2] &= ~((1 << 14) | (1 << 13) | (1 << 12) | (1 << 11) | (1 << 10) | (1 << 6) | (1 << 0));
        LPC_GPIO->DIR[3] &= ~((1 << 7) | (1 << 5));
        LPC_GPIO->DIR[5] &= ~(1 << 16);
#endif

        // ====================================================================
        // EGYSZERI HARDVERES TAKARÍTÁS A CIKLUS ELŐTT
        // ====================================================================

        // USB0 PHY és USB PLL leállítása
        LPC_CGU->PLL0USB_CTRL.PD = 1;
        LPC_CREG->CREG0 |= (1 << 5);

        // Külső oszcillátor (XTAL) és Audio PLL végleges leállítása
        (*(volatile uint32_t*)(&LPC_CGU->XTAL_OSC_CTRL)) |= (1 << 0);
        (*(volatile uint32_t*)(&LPC_CGU->PLL0AUDIO_CTRL)) |= (1 << 0);

        // ADC0 és ADC1 analóg táp kikapcsolása
        LPC_ADC0->CR &= ~(1 << 21);
        LPC_ADC1->CR &= ~(1 << 21);

        // Nem használt periféria buszok KIKAPCSOLÁSA (I2C1, SDIO, SSP1)
        LPC_CGU->BASE_APB3_CLK.PD = 1;  // APB3 hajtja az I2C1-et! Kikapcsolva.
        LPC_CGU->BASE_SDIO_CLK.PD = 1;
        LPC_CGU->BASE_SSP1_CLK.PD = 1;
        LPC_CGU->BASE_PERIPH_CLK.PD = 1;

        // Chip Select lábak bemenetté alakítása a szivárgás ellen
        LPC_GPIO->DIR[1] &= ~((1 << 11) | (1 << 14));

        rtc_interrupt_enable();
        LPC_RTC->CIIR = (1 << 1);
        LPC_RTC->ILR = 3;

        while (1) {
            // Mérés (Az I2C0 ekkor aktív)
            battery::BatteryManagement::getBatteryInfo(valid_mask, percent, voltage, current);

            if (valid_mask) {
                if (current > 0) {
                    if (percent >= 100 && current <= 5) {
                        led_tx.off();
                        led_rx.off();
                    } else {
                        led_rx.off();
                        led_tx.on();
                    }
                } else {
                    led_tx.off();
                    led_rx.off();
                }
            } else {
                led_tx.off();
                led_rx.on();
            }

            // --------------- ALVÁS ELŐKÉSZÍTÉSE ------------------------------------

            // I2C0 leállítása és a hozzá tartozó busz (APB1) lekapcsolása az alvás idejére
            portapack::i2c0.stop();
            LPC_CGU->BASE_APB1_CLK.PD = 1;

            LPC_RTC->ILR = 3;

            uint32_t saved_iser0 = NVIC->ISER[0];
            uint32_t saved_iser1 = NVIC->ISER[1];

            NVIC->ICER[0] = 0xFFFFFFFF;
            NVIC->ICER[1] = 0xFFFFFFFF;

            NVIC->ICPR[0] = 0xFFFFFFFF;
            NVIC->ICPR[1] = 0xFFFFFFFF;

            NVIC_EnableIRQ(RTC_IRQn);

            *(volatile uint32_t*)(0x40044008) = 0xFFFFFFFF;
            *(volatile uint32_t*)(0x4004400C) = (1 << 5);
            *(volatile uint32_t*)(0x40044018) = 0xFFFFFFFF;

            // Flash Power-down
            LPC_CREG->CREG6.ETHMODE |= (1 << 2);

            // PMC (PCON) beállítása Deep-sleepre
            (*(volatile uint32_t*)(0x40042000)) &= ~(0x7);

            __disable_irq();

            SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

            __DSB();
            __ISB();

            __WFI();  // Zzz... A rendszer áramfelvétele itt leesik!

            // --- ÉBREDÉS ---;

            // 1. Flash visszakapcsolása AZONNAL
            LPC_CREG->CREG6.ETHMODE &= ~(1 << 2);
            for (volatile int i = 0; i < 5000; i++) __asm__("nop");

            // 2. I2C0 busz (APB1) és az I2C0 periféria visszakapcsolása a következő méréshez
            LPC_CGU->BASE_APB1_CLK.PD = 0;
            portapack::i2c0.start(i2c_config_12mhz);

            SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
            *(volatile uint32_t*)(0x40044018) = 0xFFFFFFFF;

            NVIC->ISER[0] = saved_iser0;
            NVIC->ISER[1] = saved_iser1;

            __enable_irq();

            chEvtGetAndClearEvents(ALL_EVENTS);
        }  // while (1) - end
    } else {
        portapack::display.wake(true);
    }
}

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

    handle_shell();

    if (events & EVT_MASK_APPLICATION) {
        handle_application_queue();
    }

    if (events & EVT_MASK_LOCAL) {
        handle_local_queue();
    }

    if (events & EVT_MASK_RTC_TICK) {
        // delay error message by 2 seconds to wait for LCD being ready
        if (portapack::init_error != nullptr && ++delayed_error > 1)
            draw_guru_meditation(CORTEX_M4, portapack::init_error);

        handle_rtc_tick();
    }

    handle_usb_transfer();
    handle_usb();

    if (events & EVT_MASK_SWITCHES) {
        handle_switches();
    }

    /*if( events & EVT_MASK_LCD_FRAME_SYNC ) {
                blink_timer();
        }*/
    if (events & EVT_MASK_LCD_FRAME_SYNC) {
        handle_lcd_frame_sync(!EventDispatcher::display_sleep);
    }
    if (!EventDispatcher::display_sleep) {
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

void EventDispatcher::handle_usb() {
    portapack::usb_serial.dispatch();
}

void EventDispatcher::handle_usb_transfer() {
    portapack::usb_serial.dispatch_transfer();
}

void EventDispatcher::handle_shell() {
    if (waiting_for_shellmode) {
        waiting_for_shellmode = false;
        shellmode_active = true;
        while (shellmode_active) {
            chThdSleepMilliseconds(5);
        }
    }

    if (injected_touch_event != nullptr) {
        on_touch_event(*injected_touch_event);
        injected_touch_event = nullptr;
    }

    if (injected_keyboard_event != nullptr) {
        on_keyboard_event(*injected_keyboard_event);
        injected_keyboard_event = nullptr;
    }
}

ui::Widget* EventDispatcher::touch_widget(ui::Widget* const w, ui::TouchEvent event) {
    if (!w->hidden()) {
        // To achieve reverse depth ordering (last object drawn is
        // considered "top"), descend first.
        auto& children = w->children();
        for (auto it = children.rbegin(); it != children.rend(); ++it) {  // reverse, bc the lastly added will be "top" if overlaps
            const auto& child = *it;
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
    injected_touch_event = &event;
    while (injected_touch_event != nullptr) {
        chThdSleepMilliseconds(5);
    }
    injected_touch_event = nullptr;  // to clean event_mo.cpp, compile warning error : "storing the address of local variable 'event' in 'this_4(D)->injected_touch_event' [-Wdangling-pointer=]"
}

void EventDispatcher::emulateKeyboard(ui::KeyboardEvent event) {
    injected_keyboard_event = &event;
    while (injected_keyboard_event != nullptr) {
        chThdSleepMilliseconds(5);
    }
    injected_keyboard_event = nullptr;  // to clean event_mo.cpp, compile warning error : "storing the address of local variable 'event' in 'this_4(D)->injected_keyboard_event' [-Wdangling-pointer=]"
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

void EventDispatcher::handle_lcd_frame_sync(bool screen_on) {
    bool waiting_for_frame = this->waiting_for_frame;

    DisplayFrameSyncMessage message;  // send framesync msg all the time, bc some apps relay on it
    message_map.send(&message);
    if (screen_on) {  // only draw when screen is on
        static_cast<ui::SystemView*>(top_widget)->paint_overlay();
        painter.paint_widget_tree(top_widget);
        portapack::backlight()->on();
    }
    if (waiting_for_frame)
        this->waiting_for_frame = false;
}

void EventDispatcher::wait_finish_frame() {
    waiting_for_frame = true;
    while (waiting_for_frame) {
        chThdSleepMilliseconds(5);
    }
}

void EventDispatcher::enter_shell_working_mode() {
    waiting_for_shellmode = true;

    while (waiting_for_shellmode) {
        chThdSleepMilliseconds(5);
    }
}

void EventDispatcher::exit_shell_working_mode() {
    shellmode_active = false;
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
    if (delta == 0)
        return;

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
