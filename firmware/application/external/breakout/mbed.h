/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#ifndef __UI_mbed_H__
#define __UI_mbed_H__

using Callback = void (*)(void);

#define wait_us(x) (void)0
#define wait(x) chThdSleepMilliseconds(x * 1000)
#define PullUp 1

#include "ui_navigation.hpp"

enum {
    dp0,
    dp1,
    dp2,
    dp3,
    dp4,
    dp5,
    dp6,
    dp7,
    dp8,
    dp9,
    dp10,
    dp11,
    dp12,
    dp13,
    dp14,
    dp15,
    dp16,
    dp17,
    dp18,
    dp19,
    dp20,
    dp21,
    dp22,
    dp23,
    dp24,
    dp25,
};

static bool but_RIGHT;
static bool but_LEFT;
static bool but_SELECT;

class Timer {
   public:
    Timer() { (void)0; };
    void reset() { (void)0; };
    void start() { (void)0; }
    uint32_t read_ms() { return 1000; };

   private:
};

static Callback game_update_callback;
static uint32_t game_update_timeout;
static uint32_t game_update_counter;

static void check_game_timer() {
    if (game_update_callback) {
        if (++game_update_counter >= game_update_timeout) {
            game_update_counter = 0;
            game_update_callback();
        }
    }
}

class Ticker {
   public:
    Ticker() { (void)0; };

    void attach(Callback func, double delay_sec) {
        game_update_callback = func;
        game_update_timeout = delay_sec * 60;
    }

    void detach() {
        game_update_callback = nullptr;
    }

   private:
};

static Callback button_callback;

class InterruptIn {
   public:
    InterruptIn(int reg) {
        (void)reg;
    };
    void fall(Callback func) { button_callback = func; };
    void mode(int v) { (void)v; };

   private:
};

#endif /*__UI_mbed_H__*/