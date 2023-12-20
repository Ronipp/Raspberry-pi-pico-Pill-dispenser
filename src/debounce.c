#include "pico/stdlib.h"
#include<stdio.h>
#include <stdbool.h>
#include "debounce.h"

#define N_IRQ_TRIGGERS 4
#define IRQ_MASK_MAX 8

static uint32_t time = 0;
static uint32_t delay = 20;
static gpio_irq_callback_t callbackptr = NULL;

bool debounce(uint32_t *time) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - *time > delay) {
        *time = current_time;
        return true;
    }
    return false;
}

void callback_with_debounce(uint gpio, uint32_t mask) {
    if (debounce(&time)) {
        callbackptr(gpio, mask);
    }
}

void init_button_with_callback(uint gpio, uint number_of_buttons, uint32_t event_mask, gpio_irq_callback_t callback) {
    for (int i=0; i<number_of_buttons; i++) {
        gpio_init(gpio+i);
        gpio_pull_up(gpio+i);
    }
    
    callbackptr = callback;
    gpio_set_irq_enabled_with_callback(gpio, event_mask, true, callback_with_debounce);
    for (int i=1; i<number_of_buttons; i++) {
        gpio_set_irq_enabled(gpio+i, event_mask, true);
    }
}