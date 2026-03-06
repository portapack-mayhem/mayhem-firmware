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

#include "touch.hpp"

#include "portapack_persistent_memory.hpp"

#include "usb_serial_asyncmsg.hpp"

using namespace portapack;

#include "utility.hpp"

namespace touch {

Metrics calculate_metrics(const Frame& frame) {
    /* TODO: Yikes! M0 doesn't have floating point, so this code is
     * expensive! On the other hand, it seems to be working well (and
     * fast *enough*?), so maybe leave it alone at least for now.
     */
    // source
    const auto x_max = frame.x.xp;
    const auto x_min = frame.x.xn;
    const float x_position = (frame.x.yp + frame.x.yn) * 0.5f;

    // const auto x_max = frame.x.yp;   
    // const auto x_min = frame.x.yn;   
    // const float x_position = (frame.x.xp + frame.x.xn) * 0.5f; 
    
    const auto x_range = x_max - x_min;
    const float x_norm = (x_position - x_min) / x_range;

    UsbSerialAsyncmsg::asyncmsg("x_norm:");
    UsbSerialAsyncmsg::asyncmsg(x_norm);
    // UsbSerialAsyncmsg::asyncmsg("\r\n");

    // char tmp_string[64];
    // snprintf(tmp_string,64,"####xp:%u xn:%u yp:%u yn:%u\n",frame.x.xp,frame.x.xn,frame.x.yp,frame.x.yn);
    // std::string test_string = tmp_string;
    // UsbSerialAsyncmsg::asyncmsg(test_string);
    // UsbSerialAsyncmsg::asyncmsg(x_norm);

    // char tmp_string[64];
    // snprintf(tmp_string,64,"frame y: xp:%u\txn:%u\typ:%u\tyn:%u\r\n",frame.y.xp,frame.y.xn,frame.y.yp,frame.y.yn);
    // std::string test_string = tmp_string;
    // UsbSerialAsyncmsg::asyncmsg(test_string);

    // source
    // const auto y_max = frame.y.yn;
    // const auto y_min = frame.y.yp;
    // const auto y_range = y_max - y_min;
    // const float y_position = (frame.y.xp + frame.y.xn) * 0.5f;
    // const float y_norm = (y_position - y_min) / y_range;

    auto y_top = 8;
    auto y_bottom = 150;

    const float y_position = (frame.y.xp + frame.y.xn) * 0.5f;

    float y_norm = (y_position - y_top) / (y_bottom - y_top);

    if(y_norm >=1.0)
        y_norm = 1.0;
    if(y_norm <=0.0)
        y_norm = 0.0;

    // // 目前观测到的 SENSEY xp xn是正常变化，但是 yp yn一直是定值不变？
    // const auto y_max = frame.y.yp;   // 978，驱动高电平
    // const auto y_min = frame.y.yn;    // 12，驱动低电平

     
    // 因为这里是定制所以，就不考虑？
    // const auto y_range = y_max - y_min;
    // const float y_position = (frame.y.xp + frame.y.xn) * 0.5f;  // 采样端，会随触摸变化
    // // from top to bottom 0~360 frame.y.xp
    // // from left to right 0~50 frame.y.xn
    // // (360 + 50) / 2 = 205
    // const float y_norm = y_position / 205.0f; 
    

    UsbSerialAsyncmsg::asyncmsg("\ty_norm:");
    UsbSerialAsyncmsg::asyncmsg(y_norm);
    UsbSerialAsyncmsg::asyncmsg("\r\n");

    //压力检测也有错误？
    const auto z_max = frame.pressure.yp;
    const auto z_min = frame.pressure.xn;
    const auto z_range = z_max - z_min;
    const auto z1_position = frame.pressure.xp;
    const float z1_norm = float(z1_position - z_min) / z_range;
    const auto z2_position = frame.pressure.yn;
    const float z2_norm = float(z2_position - z_min) / z_range;
    const float r_x_plate = 330.0f;
    // const float r_y_plate = 600.0f;
    const float r_touch = r_x_plate * x_norm * (z2_norm / z1_norm - 1.0f);

    // UsbSerialAsyncmsg::asyncmsg("p.yp:");
    // UsbSerialAsyncmsg::asyncmsg(frame.pressure.yp);
    // UsbSerialAsyncmsg::asyncmsg("\tp.yn:");
    // UsbSerialAsyncmsg::asyncmsg(frame.pressure.yn);
    // UsbSerialAsyncmsg::asyncmsg("\tp.xp:");
    // UsbSerialAsyncmsg::asyncmsg(frame.pressure.xp);
    // UsbSerialAsyncmsg::asyncmsg("\tp.xn:");
    // UsbSerialAsyncmsg::asyncmsg(frame.pressure.xn);
    // UsbSerialAsyncmsg::asyncmsg("\tz1:");
    // UsbSerialAsyncmsg::asyncmsg(z1_norm);
    // UsbSerialAsyncmsg::asyncmsg("\tz2:");
    // UsbSerialAsyncmsg::asyncmsg(z2_norm);
    // UsbSerialAsyncmsg::asyncmsg("\tr_touch");
    // UsbSerialAsyncmsg::asyncmsg(r_touch);
    // UsbSerialAsyncmsg::asyncmsg("\r\n");

    return {
        .x = x_norm,
        .y = y_norm,
        .r = r_touch,
    };
}

ui::Point Calibration::translate(const DigitizerPoint& p) const {
    // static constexpr range_t<int32_t> x_range{0, 240 - 1};
    // static constexpr range_t<int32_t> y_range{0, 320 - 1};

    static range_t<int32_t> x_range{0, screen_width - 1};
    static range_t<int32_t> y_range{0, screen_height - 1};

    const int32_t x = (a * p.x + b * p.y + c) / k;
    const int32_t y = (d * p.x + e * p.y + f) / k;
    const auto x_clipped = x_range.clip(x);
    const auto y_clipped = y_range.clip(y);
    return {x_clipped, y_clipped};
}

void Manager::feed(const Frame& frame) {
    // touch_debounce.feed(touch_raw);
    const auto touch_raw = frame.touch;
    // const auto touch_stable = touch_debounce.state();
    const auto touch_stable = frame.touch;
    bool touch_down_pressure = false;
    bool touch_up_pressure = false;

    // Only feed coordinate averaging if there's a touch.
    // TODO: Separate threshold to gate coordinates for filtering?
    if (touch_raw) {
        const auto metrics = calculate_metrics(frame);

        constexpr float r_touch_down_threshold = 3200.0f;
        constexpr float r_touch_up_threshold = r_touch_down_threshold * 2.0f;

        touch_down_pressure = (metrics.r < r_touch_down_threshold);
        touch_up_pressure = (metrics.r < r_touch_up_threshold);

        if (touch_down_pressure) {
            filter_x.feed(metrics.x * 1024);
            filter_y.feed(metrics.y * 1024);
        }
    } else {
        filter_x.reset();
        filter_y.reset();
    }

    switch (state) {
        case State::NoTouch:
            if (touch_stable && touch_down_pressure && !persistent_memory::disable_touchscreen()) {
                if (point_stable()) {
                    state = State::TouchDetected;
                    touch_started();
                }
            }
            break;

        case State::TouchDetected:
            if (touch_stable && touch_up_pressure) {
                touch_moved();
            } else {
                state = State::NoTouch;
                touch_ended();
            }
            break;

        default:
            state = State::NoTouch;
            break;
    }
}

ui::Point Manager::filtered_point() const {
    return persistent_memory::touch_calibration().translate({filter_x.value(), filter_y.value()});
}

} /* namespace touch */