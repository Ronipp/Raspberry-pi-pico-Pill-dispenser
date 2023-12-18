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

#include "hardware/pio.h"

#define UART_TX_PIN
#define UART_RX_PIN

typedef enum state {
    a,
    b,
    c,
} state;


int main() {
    stdio_init_all();
    eeprom_init_i2c(i2c0, 9600, 5);
    lora_init(uart1, UART_TX_PIN, UART_RX_PIN);
    stepper_ctx step_ctx = stepper_get_ctx();
    stepper_init(&step_ctx, pio0, );

    struct rebootValues eepromRebootValues;  // Holds values read from EEPROM
    reboot_sequence(&eepromRebootValues);

    state sm = a;

    while (1) {
        switch (sm)
        {
        case a:
            break;
        }
    }
}