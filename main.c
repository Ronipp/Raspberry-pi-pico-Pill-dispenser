#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "debounce.h"
#include "stepper.h"
#include "lora.h"
#include "eeprom.h"
#include "magnusFuncs.h"
#include "statemachine.h"
#include "led.h"
#include <time.h>
#include "stdlib.h"

#include "hardware/pio.h"

#define UART_TX_PIN
#define UART_RX_PIN
#define EEPROM_ARR_LENGTH 64
#define LOG_START_ADDR 0

#define OPTO_FORK_PIN 28
#define PIEZO_PIN 27
#define BUTTON1 7
#define BUTTON2 8

#define PILL_DROP_DELAY_MS 5000



    static bool calib_btn_pressed = false;
    static bool dispense_btn_pressed = false;

void button_handler(uint gpio, uint32_t mask) {
    if (gpio == BUTTON1) {
        if (mask & GPIO_IRQ_EDGE_FALL) {
            calib_btn_pressed = true;
        } 
        if (mask & GPIO_IRQ_EDGE_RISE) {
            calib_btn_pressed = false;
        }
    } else {
        if (mask & GPIO_IRQ_EDGE_FALL) {
            dispense_btn_pressed = true;
        } 
        if (mask & GPIO_IRQ_EDGE_RISE) {
            dispense_btn_pressed = false;
        }
    }
}

static bool dropped = false;

void piezo_handler(void) {
    if (gpio_get_irq_event_mask(PIEZO_PIN) & GPIO_IRQ_EDGE_FALL) {
        gpio_acknowledge_irq(PIEZO_PIN, GPIO_IRQ_EDGE_FALL);
        dropped = true;
    }
}

int main()
{
    stdio_init_all();
    //EEPROM
    eeprom_init_i2c(i2c0, 1000000, 5);
    //LORAWAN
    // lora_init(uart1, UART_TX_PIN, UART_RX_PIN);

    // STEPPER MOTOR
    uint stepperpins[4] = {BLUE, PINK, YELLOW, ORANGE}; // pins by color, see stepper.h for pin numbers.
    stepper_ctx step_ctx = stepper_get_ctx(); // context for the stepper motor, includes useful stuff.
    stepper_init(&step_ctx, pio0, stepperpins, OPTO_FORK_PIN, 10, STEPPER_CLOCKWISE); // inits everything, uses pio to drive stepper motor.
    //LEDS
    led_init(); // inits pwm for leds so we don't get blind.
    //BUTTONS
    init_button_with_callback(BUTTON1, 2, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, button_handler); // set debounced irq for buttons.
    // PIEZO SENSOR
    gpio_init(PIEZO_PIN); // enabled, func set, dir in.
    gpio_pull_up(PIEZO_PIN); // pull up
    gpio_add_raw_irq_handler(PIEZO_PIN, piezo_handler); // adding raw handler since we dont want this debounced.
    gpio_set_irq_enabled(PIEZO_PIN, GPIO_IRQ_EDGE_FALL, true); // irq enabled, only interested in falling edge (something hit the sensor).


    //STATE MACHINE
    state_machine sm = statemachine_get(0); // inits statemachine, pills dropped determines the first state.

    while (1) {
        state_machine_update_time(&sm);
        switch (sm.state) {
        case CALIBRATE:
        // TODO: logs and lorawan
            step_ctx.stepper_calibrated = false; // set stepper calibrated status to false.
            led_wait_toggle(sm.time_ms); // toggling all leds on and off while waiting for a button press.
            if (calib_btn_pressed) { // if button is pressed
                stepper_calibrate(&step_ctx); // calibrate :D
                sm.pills_dropped = 0; // reset pill dropping count.
                sm.state = WAIT_FOR_DISPENSE; // when calibration is done move to next state.
            }
            break;
        case HALF_CALIBRATE:
            /* code */
            break;
        case WAIT_FOR_DISPENSE:
            if (stepper_is_calibrating(&step_ctx)) { // if calibrating
                led_calibration_toggle(sm.time_ms); // toggling leds in a nice pattern.
            } else {
                led_on(); // turn leds on when user can press the button to start dispensing.
                if (dispense_btn_pressed) { // if button pressed
                    led_off(); // turn the led off
                    sm.state = DISPENSE; // move to state where dispensing is actually done.
                }
            }
            break;
        case DISPENSE:
            // TODO: timer
            if ((sm.time_ms - sm.time_pill_dropped_ms) > PILL_DROP_DELAY_MS) { // if enough time has passed from last pill drop.
                stepper_turn_steps(&step_ctx, step_ctx.step_max / 8); // turn stepper eighth of a full turn.
                sm.time_pill_dropped_ms = sm.time_ms;
                sm.state = CHECK_IF_DISPENSED; // go to check if pill was dispensed correctly.
            }
            break;
        case CHECK_IF_DISPENSED:
        //TODO: actually check if something dropped... and logs
            if (stepper_is_running(&step_ctx)) { // if stepper is still running we let it do its thing
                led_run_toggle(sm.time_ms); // and toggle some pretty lights
            } else if (dropped) { // if pill drop was detected by piezo sensor
                sm.pills_dropped++; // increment pill drop count
                if (sm.pills_dropped == 7) { // if maximum number of pills dropped
                    sm.state = CALIBRATE; // set state to calibration. (start all over again)
                } else {
                    sm.state = DISPENSE; // if number of pills dropped not max dispense another
                }
            } else { // if stepper is not running and no pill drop detected
                // TODO: figure out timing things and setting state
            }
            break;
        case PILL_NOT_DROPPED:
            break;
        default:
            break;
        } 
    }
}
