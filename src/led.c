#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "led.h"
#include <stdio.h>

#define LED1 20
#define LED2 21
#define LED3 22

#define BRIGHTNESS 100

#define WAIT_TOGGLE_DELAY_MS 500
#define CALIBRATION_TOGGLE_DELAY_MS 500
#define RUN_TOGGLE_DELAY_MS 200
#define ERROR_TOGGLE_DELAY_MS 200

static uint led_array[3] = {LED1, LED2, LED3};

/**
 * Initializes the LEDs for PWM functionality and sets up the PWM configuration.
 */
void led_init(void) {
    for (int i = 0; i < 3; i++) {
        gpio_set_function(LED3 - i, GPIO_FUNC_PWM); // Set LED pins for PWM functionality
        gpio_set_dir(LED3 - i, GPIO_OUT); // Set LED pins as outputs

        uint slice = pwm_gpio_to_slice_num(LED3 - i); // Get PWM slice for LED
        uint chan = pwm_gpio_to_channel(LED3 - i); // Get PWM channel for LED

        pwm_set_enabled(slice, false); // Disable PWM initially
        pwm_config conf = pwm_get_default_config(); // Get default PWM configuration
        pwm_config_set_clkdiv_int(&conf, 125); // Set clock divisor for PWM
        pwm_config_set_wrap(&conf, 999); // Set wrap value for PWM
        pwm_init(slice, &conf, false); // Initialize PWM with the configuration
        pwm_set_chan_level(slice, chan, 0); // Set initial PWM level to 0
        pwm_set_enabled(slice, true); // Enable PWM for the LED
    }
}

static bool led_on_off = false;

/**
 * Turns off all LEDs by setting their PWM levels to 0.
 */
void led_off(void) {
    for (int i = 0; i < 3; i++) {
        pwm_set_gpio_level(led_array[i], 0); // Turn LED off by setting PWM level to 0
    }
    led_on_off = false; // Update LED state to off
}

/**
 * Turns on all LEDs at full brightness by setting their PWM levels to BRIGHTNESS.
 */
void led_on(void) {
    for (int i = 0; i < 3; i++) {
        pwm_set_gpio_level(led_array[i], BRIGHTNESS); // Turn LED on at full brightness
    }
    led_on_off = true; // Update LED state to on
}

/**
 * Toggles the LED state between on and off.
 */
void led_toggle(void) {
    if (led_on_off) {
        led_off(); // Turn off LEDs if they are on
    } else {
        led_on(); // Turn on LEDs if they are off
    }
}

/**
 * Toggles the LED state based on the time elapsed since the last LED action.
 *
 * @param time The current time.
 */
void led_wait_toggle(uint32_t time) {
    if (!led_timer(time, WAIT_TOGGLE_DELAY_MS)) return; // Do nothing if it's not time to toggle
    led_toggle();
}

/**
 * Toggles the LED state for an error indication based on the time elapsed since the last LED action.
 *
 * @param time The current time.
 * @return true if the LED toggling occurred, false otherwise.
 */
bool led_error_toggle(uint32_t time) {
    if (!led_timer(time, ERROR_TOGGLE_DELAY_MS)) return false; // Do nothing if it's not time to toggle
    led_toggle();
    return true;
}

/**
 * Toggles the LED sequence for calibration purposes based on the time elapsed since the last LED action.
 *
 * @param time The current time.
 */
void led_calibration_toggle(uint32_t time) {
    if (!led_timer(time, CALIBRATION_TOGGLE_DELAY_MS)) return; // Do nothing if it's not time to toggle
    led_sequence_toggler();
}

/**
 * Toggles the LED sequence for running purposes based on the time elapsed since the last LED action.
 *
 * @param time The current time.
 */
void led_run_toggle(uint32_t time) {
    if (!led_timer(time, RUN_TOGGLE_DELAY_MS)) return; // Do nothing if it's not time to toggle
    led_sequence_toggler();
}

/**
 * Toggles the LED sequence by cycling through LEDs turning them on one at a time.
 */
static void led_sequence_toggler(void) {
    for (int i = 0; i < 3; i++) {
        if (i == stage) {
            pwm_set_gpio_level(led_array[i], BRIGHTNESS); // Turn LED on
        } else {
            pwm_set_gpio_level(led_array[i], 0); // Turn LED off
        }
    }
    stage = (stage + 1) % 3; // Update the stage to cycle LEDs
}

/**
 * Checks the elapsed time and determines if an LED action can be performed based on the delay.
 *
 * @param time  The current time.
 * @param delay The delay required before an LED action.
 * @return true if the delay has passed since the last LED action, false otherwise.
 */
bool led_timer(uint32_t time, uint32_t delay) {
    if (time - led_time > delay) {
        led_time = time;
        return true; // Return true when the delay has passed
    }
    return false; // Return false when the delay hasn't passed
}
