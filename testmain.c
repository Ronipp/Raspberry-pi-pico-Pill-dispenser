#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "debounce.h"
#include "stepper.h"
#include "lora.h"
#include "eeprom.h"

#include "hardware/pio.h"

#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define STEPPER_OPTO_FORK_PIN 28


int main() {
    stdio_init_all();

    uint stepperpins[4] = {BLUE, PINK, YELLOW, ORANGE};
    stepper_ctx step_ctx = stepper_get_ctx();
    stepper_ctx *ctx = &step_ctx;
    stepper_init(ctx, pio0, stepperpins, STEPPER_OPTO_FORK_PIN, 10, STEPPER_CLOCKWISE);

    stepper_half_calibrate(ctx, 4095, 314, 8);

    while (1) {
        sleep_ms(100);
    }
}