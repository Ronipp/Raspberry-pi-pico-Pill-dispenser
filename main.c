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

typedef enum state {
    a,
    b,
    c,
} state;


int main() {
    stdio_init_all();
    eeprom_init_i2c(i2c0, 1000000, 5);
    lora_init(uart1, UART_TX_PIN, UART_RX_PIN);

    uint stepperpins[4] = {BLUE, PINK, YELLOW, ORANGE};
    stepper_ctx step_ctx = stepper_get_ctx();
    stepper_init(&step_ctx, pio0, stepperpins, STEPPER_OPTO_FORK_PIN, 10, STEPPER_CLOCKWISE);

    state sm = a;

    while (1) {
        switch (sm)
        {
        case a:
            break;
        }
    }
}