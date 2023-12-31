#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "debounce.h"
#include "stepper.h"
#include "lora.h"
#include "eeprom.h"
#include "logHandling.h"
#include "statemachine.h"
#include "led.h"
#include <time.h>
#include "stdlib.h"
#include "hardware/watchdog.h"

#include "hardware/pio.h"

#define UART_TX_PIN
#define UART_RX_PIN
#define EEPROM_ARR_LENGTH 64
#define LOG_START_ADDR 0

#define OPTO_FORK_PIN 28
#define PIEZO_PIN 27
#define BUTTON1 7
#define BUTTON2 8

#define STEPPER_SPEED_RPM 10

#define PILL_DROP_DELAY_MS 5000
#define PILL_NOT_DROPPED_DELAY_MS 600

#define ERROR_BLINK_TIMES 5
#define MAX_PILLS 7
#define MAX_TURNS 8


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
    eeprom_init_i2c(i2c0, 1000000, 5); // TODO replace magic numbers
    //LORAWAN
    // lora_init(uart1, UART_TX_PIN, UART_RX_PIN);

    // STEPPER MOTOR
    uint stepperpins[4] = {BLUE, PINK, YELLOW, ORANGE}; // pins by color, see stepper.h for pin numbers.
    stepper_ctx step_ctx = stepper_get_ctx(); // context for the stepper motor, includes useful stuff.
    stepper_init(&step_ctx, pio0, stepperpins, OPTO_FORK_PIN, STEPPER_SPEED_RPM, STEPPER_CLOCKWISE); // inits everything, uses pio to drive stepper motor.
    //LEDS
    led_init(); // inits pwm for leds so we don't get blind.
    //BUTTONS
    init_button_with_callback(BUTTON1, 2, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, button_handler); // set debounced irq for buttons. //TODO: REPLACE MAGIC NUMBER
    // PIEZO SENSOR
    gpio_init(PIEZO_PIN); // enabled, func set, dir in.
    gpio_pull_up(PIEZO_PIN); // pull up
    gpio_add_raw_irq_handler(PIEZO_PIN, piezo_handler); // adding raw handler since we dont want this debounced.
    gpio_set_irq_enabled(PIEZO_PIN, GPIO_IRQ_EDGE_FALL, true); // irq enabled, only interested in falling edge (something hit the sensor).


    // Reboot sequence
    const uint64_t bootTime = time_us_64();
    DeviceStatus devStatus;
    reboot_sequence(&devStatus, bootTime);

    state_machine sm;
    const int dispenseCodes[7] = {DISPENSE1, DISPENSE2, DISPENSE3, DISPENSE4, DISPENSE5, DISPENSE6, DISPENSE7};
    if (devStatus.rebootStatusCode != devStatus.pillDispenseState && isValueInArray(devStatus.rebootStatusCode, dispenseCodes, 7) == true) {
        sm = statemachine_get(devStatus.pillDispenseState, devStatus.rebootStatusCode);
    }
    else {
        sm = statemachine_get(devStatus.pillDispenseState, devStatus.pillDispenseState);
    }

    //STATE MACHINE
    if (sm.state == HALF_CALIBRATE) {
        // half calibrate takes: max steps, hole width, number of pills dispensed (how many time stepper has turned)
        stepper_half_calibrate(&step_ctx, 4095, 315, 2); // start half calibration if its prudent to do so //TODO: REPLACE MAGIC NUMBERS
        sm.state = WAIT_FOR_DISPENSE; // state to wait for dispense
    }



    while (1) {
        state_machine_update_time(&sm);
        switch (sm.state) {
        case CALIBRATE:
        // TODO: logs and lorawan
            step_ctx.stepper_calibrated = false; // set stepper calibrated status to false.
            led_wait_toggle(sm.time_ms); // toggling all leds on and off while waiting for a button press.
            if (calib_btn_pressed) { // if button is pressed
                led_off(); // leds off
                stepper_calibrate(&step_ctx); // calibrate :D
                sm.pills_dropped = 0; // reset pill dropping count.
                sm.pills_error = 0; // reset error count.
                sm.state = WAIT_FOR_DISPENSE; // when calibration is done move to next state.
            }
            break;
        case WAIT_FOR_DISPENSE:
            if (stepper_is_running(&step_ctx)) { // if calibrating
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
            if ((sm.pills_dropped + sm.pills_error) >= MAX_PILLS) { // if maximum number of pills dropped (or didnt drop was but supposed to)
                    sm.state = CALIBRATE; // set state to calibration. (start all over again)
            } else if ((sm.time_ms - sm.time_drop_started_ms) > PILL_DROP_DELAY_MS) { // if enough time has passed from last pill drop.
                stepper_turn_steps(&step_ctx, stepper_get_max_steps(&step_ctx) / MAX_TURNS); // turn stepper eighth of a full turn.
                sm.time_drop_started_ms = sm.time_ms; // set the drop starting time to current time.
                dropped = false; // reset dropped status
                sm.state = CHECK_IF_DISPENSED; // go to check if pill was dispensed correctly.
            }
            break;
        case CHECK_IF_DISPENSED:
        //TODO: actually check if something dropped... and logs
            if (stepper_is_running(&step_ctx)) { // if stepper is still running we let it do its thing
                led_run_toggle(sm.time_ms); // and toggle some pretty lights
            } else if (dropped) { // if pill drop was detected by piezo sensor
                sm.pills_dropped++; // increment pill drop count
                dropped = false; // reset dropped status
                // LOG HERE
                sm.state = DISPENSE; // if number of pills dropped not max dispense another
            } else { // if stepper is not running and no pill drop detected
                if (sm.time_ms - sm.time_drop_started_ms > PILL_NOT_DROPPED_DELAY_MS) { // if too much time between pill drop starting and not sensing a drop
                    led_off(); // leds off
                    sm.pills_error++; // increment the error count 
                    // LOG HERE
                    sm.state = PILL_NOT_DROPPED; // go to error state
                } else { // if we are still waiting for the drop
                    led_run_toggle(sm.time_ms); // pretty lights
                }
            }
            break;
        case PILL_NOT_DROPPED:
            if (led_error_toggle(sm.time_ms)) { // if led is toggled
                // 2 times the error blink times because led_error_toggle returns true when state changes not when leds go on.
                if (++(sm.error_blink_counter) >= (2 * ERROR_BLINK_TIMES)) { // increment blink counter if its twice needed blink times
                    sm.error_blink_counter = 0; // reset blink counter
                    sm.state = DISPENSE; // set state to dispense (do we want it to recalibrate here?)
                }
            }
            break;
        default:
            // should never get here maybe log unexpected error?
            sm.state = CALIBRATE;
            break;
        } 
    }
    return 0;

}
