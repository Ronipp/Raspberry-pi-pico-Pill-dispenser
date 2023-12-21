#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "led.h"
#include <stdio.h>

#define LED1 20
#define LED2 21
#define LED3 22

#define BRIGHTNESS 100

#define TOGGLE_DELAY_MS 500

static uint led_array[3] = {LED1, LED2, LED3};

void led_init(void) {
    for (int i=0; i<3; i++) {
        gpio_set_function(LED3-i, GPIO_FUNC_PWM);
        gpio_set_dir(LED3-i, GPIO_OUT);

        uint slice = pwm_gpio_to_slice_num(LED3-i);
        uint chan = pwm_gpio_to_channel(LED3-i);

        pwm_set_enabled(slice, false);
        pwm_config conf = pwm_get_default_config();
        pwm_config_set_clkdiv_int(&conf, 125);
        pwm_config_set_wrap(&conf, 999);
        pwm_init(slice, &conf, false);
        pwm_set_chan_level(slice, chan, 0);
        pwm_set_enabled(slice, true);
    }
}

static bool led_on_off = false;

void led_wait_toggle(uint32_t time) {
    if (!led_timer(time)) return; // do nothing if its not time to do something

    for (int i=0; i<3; i++) {
        pwm_set_gpio_level(led_array[i], led_on_off ? BRIGHTNESS : 0); // on or off
    }
    led_on_off = !led_on_off;
}

static uint8_t stage = 0;

void led_calibration_toggle(uint32_t time) {
    if (!led_timer(time)) return; // do nothing if its not time to do something

    for (int i=0; i<3; i++) {
        if (i == stage) {
            pwm_set_gpio_level(led_array[i], BRIGHTNESS); // led on
        } else {
            pwm_set_gpio_level(led_array[i], 0); // led off
        }
    }
    stage = (stage+1) % 3; // stage can be 0, 1 or 2
}

static uint32_t led_time = 0;

bool led_timer(uint32_t time) {
    if (time - led_time > TOGGLE_DELAY_MS) {
        led_time = time;
        return true;
    }
    return false;
}

void led_off(void) {
    for (int i=0; i<3; i++) {
        pwm_set_gpio_level(led_array[i], 0); // led off
    }
}

void led_on(void) {
    for (int i=0; i<3; i++) {
        pwm_set_gpio_level(led_array[i], BRIGHTNESS); // led off
    }
}