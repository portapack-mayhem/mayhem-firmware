#include "gpio_lpc.h"
#include "gpio.h"

typedef enum {
    LED1 = 0,
    LED2 = 1,
    LED3 = 2,
    LED4 = 3,
} led_t;

/* GPIO Output PinMux */
static struct gpio gpio_led[] = {
    GPIO(2, 1),
    GPIO(2, 2),
    GPIO(2, 8),
#ifdef RAD1O
    GPIO(5, 26),
#endif
};

void delay(uint32_t duration) {
    while (duration--) {
        /* cannot be optimized out */
        __asm__ volatile("nop");
    }
}

void delay_us_at_mhz(uint32_t us, uint32_t mhz) {
    /* overflow-safe multiply */
    uint64_t cycles64 = (uint64_t)us * (uint64_t)mhz;
    if (cycles64 > UINT32_MAX) {
        cycles64 = UINT32_MAX;
    }
    delay((uint32_t)cycles64);
}

void led_on(const led_t led) {
    gpio_set(&gpio_led[led]);
}

void led_off(const led_t led) {
    gpio_clear(&gpio_led[led]);
}

void halt_and_flash(const uint32_t duration) {
    /* blink LED1, LED2, and LED3 */
    while (1) {
        led_on(LED1);
        led_on(LED2);
        led_on(LED3);
        delay(duration);
        led_off(LED1);
        led_off(LED2);
        led_off(LED3);
        delay(duration);
    }
}
