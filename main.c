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


char *rebootStatusCodes[20] = {
    "Boot",
    "Button press",
    "Watchdog reset",
    "Kremlins in the code",
    "Blood for the blood god, skulls for the skull throne."};

int main()
{
    stdio_init_all();
    eeprom_init_i2c(i2c0, 1000000, 5);
    // lora_init(uart1, UART_TX_PIN, UART_RX_PIN);

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
