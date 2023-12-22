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
#define PIEZO_PIN 27

static bool dropped = false;

void piezo_handler(void) {
    if (gpio_get_irq_event_mask(PIEZO_PIN) & GPIO_IRQ_EDGE_RISE) {
        gpio_acknowledge_irq(PIEZO_PIN, GPIO_IRQ_EDGE_RISE);
        dropped = true;
    }
}

int main() {
    stdio_init_all();

    gpio_init(PIEZO_PIN); // enabled, func set, dir in.
    gpio_pull_up(PIEZO_PIN); // pull up
    gpio_add_raw_irq_handler(PIEZO_PIN, piezo_handler); // adding raw handler since we dont want this debounced.
    gpio_set_irq_enabled(PIEZO_PIN, GPIO_IRQ_EDGE_RISE, true); // irq enabled, only interested in falling edge (something hit the sensor).
    irq_set_enabled(IO_IRQ_BANK0, true);

    while (1) {
        if (dropped) {
            dropped = false;
            printf("yes\n");
        }
        sleep_ms(100);
    }
}