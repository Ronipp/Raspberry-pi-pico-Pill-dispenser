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

#include "hardware/pio.h"

#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define STEPPER_OPTO_FORK_PIN 28

#define CALIB_BUTTON 7
#define DISPENSE_BUTTON 8

static bool calib_btn_pressed = false;
static bool dispense_btn_pressed = false;

void button_handler(uint gpio, uint32_t mask) {
    if (gpio == CALIB_BUTTON) {
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
    eeprom_init_i2c(i2c0, 1000000, 5);
    // lora_init(uart1, UART_TX_PIN, UART_RX_PIN);

    uint stepperpins[4] = {BLUE, PINK, YELLOW, ORANGE};
    stepper_ctx step_ctx = stepper_get_ctx();
    stepper_init(&step_ctx, pio0, stepperpins, STEPPER_OPTO_FORK_PIN, 10, STEPPER_CLOCKWISE);

    //CALIBRATION AND DISPENSE BUTTON
    init_button_with_callback(CALIB_BUTTON, 2, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, button_handler);



    struct rebootValues eepromRebootValues;   // Holds values read from EEPROM
    struct rebootValues watchdogRebootValues; // Holds values read from watchdog

    uint8_t arr[4] = {0, 1, 5, 255};

    enterLogToEeprom(arr);

    if (reboot_sequence(&eepromRebootValues, &watchdogRebootValues) == true)
    {
        printf("crc true\n");
        printf("Pill Dispense State: %d\n", eepromRebootValues.pillDispenseState);
        printf("Reboot Status Code: %d\n", eepromRebootValues.rebootStatusCode);
        printf("Previous Calibration Step Count: %d\n", eepromRebootValues.prevCalibStepCount);
    }
    else
    {
        printf("crc false\n");
    }

    // STATE MACHINE
    state_machine sm = statemachine_get(eepromRebootValues.pillDispenseState);

    //LEDS
    led_init();

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
            /* code */
            break;
        case DISPENSE:
            led_on();
            sleep_ms(2000);
            break;
        case CHECK_IF_DISPENSED:
            /* code */
            break;
        default:
            break;
        } 
    }
}