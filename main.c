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

#include "hardware/pio.h"

#define UART_TX_PIN
#define UART_RX_PIN
#define EEPROM_ARR_LENGTH 64
#define LOG_START_ADDR 0

#define OPTO_FORK_PIN 28
#define BUTTON1 7
#define BUTTON2 8

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

int main()
{

    stdio_init_all();

    watchdog_enable(100000, true);

    uint8_t data[EEPROM_ARR_LENGTH];
    uint64_t time = time_us_64();
    deviceStatus devStatus;
    reboot_sequence(&devStatus, watchdog_caused_reboot(), time);

    while (true){
        watchdog_update();
    }


    /*

    stdio_init_all();
    //EEPROM
    eeprom_init_i2c(i2c0, 1000000, 5);
    //LORAWAN
    // lora_init(uart1, UART_TX_PIN, UART_RX_PIN);

    // STEPPER MOTOR
    uint stepperpins[4] = {BLUE, PINK, YELLOW, ORANGE};
    stepper_ctx step_ctx = stepper_get_ctx();
    stepper_init(&step_ctx, pio0, stepperpins, OPTO_FORK_PIN, 10, STEPPER_CLOCKWISE);
    //LEDS
    led_init();
    //BUTTONS
    init_button_with_callback(BUTTON1, 2, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, button_handler);


    //STATE MACHINE
    state_machine sm = statemachine_get(0);

    const uint64_t bootTime = time_us_64();
    deviceStatus devStatus;
    reboot_sequence(&devStatus, watchdog_caused_reboot(), bootTime);

    while (1) {
        state_machine_update_time(&sm);
        switch (sm.state) {
        case CALIBRATE:
            if (step_ctx.stepper_calibrating) {
                led_calibration_toggle(sm.time_ms);
            } else {
                led_wait_toggle(sm.time_ms);
                if (calib_btn_pressed) stepper_calibrate(&step_ctx);
            }
            if (step_ctx.stepper_calibrated) sm.state = DISPENSE;
            break;
        case HALF_CALIBRATE:
            // code 
            break;
        case DISPENSE:
            led_on();
            sleep_ms(2000);
            break;
        case CHECK_IF_DISPENSED:
            // code
            break;
        default:
            break;
        } 
    }
    return 0;
    */
}
