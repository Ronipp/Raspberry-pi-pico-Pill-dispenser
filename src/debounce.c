#include "pico/stdlib.h"
#include<stdio.h>
#include <stdbool.h>
#include "debounce.h"

#define N_IRQ_TRIGGERS 4
#define IRQ_MASK_MAX 8

static uint32_t time = 0;
static uint32_t delay = 20;
static gpio_irq_callback_t callbackptr = NULL;

/**
 * Checks for a time delay since the last recorded time and updates it if the delay condition is met.
 *
 * @param time Pointer to the time value to be checked and updated.
 * @return true if the delay condition is met, otherwise false.
 */
bool debounce(uint32_t *time) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time()); // Get current time in milliseconds
    if (current_time - *time > delay) { // Check if the delay condition is met
        *time = current_time; // Update the time value to the current time
        return true; // Return true indicating the delay condition is met
    }
    return false; // Return false indicating the delay condition is not yet met
}

/**
 * Callback function with debounce mechanism to prevent rapid triggering.
 *
 * @param gpio GPIO identifier.
 * @param mask Mask value.
 */
void callback_with_debounce(uint gpio, uint32_t mask) {
    if (debounce(&time)) { // Check for debounce delay
        callbackptr(gpio, mask); // Call the actual callback function
    }
}

/**
 * Initializes multiple buttons with callbacks and debounce for a specified GPIO range.
 *
 * @param gpio             GPIO pin identifier for the first button in the range.
 * @param number_of_buttons The number of buttons to be initialized.
 * @param event_mask       Mask value for the event triggering the callback.
 * @param callback         Callback function to be associated with the GPIO events.
 */
void init_button_with_callback(uint gpio, uint number_of_buttons, uint32_t event_mask, gpio_irq_callback_t callback) {
    for (int i = 0; i < number_of_buttons; i++) {
        gpio_init(gpio + i); // Initialize GPIO pins
        gpio_pull_up(gpio + i); // Set GPIO pins to pull-up
    }
    
    callbackptr = callback; // Set the callback function pointer
    gpio_set_irq_enabled_with_callback(gpio, event_mask, true, callback_with_debounce); // Enable IRQ for the first button with debounce
    for (int i = 1; i < number_of_buttons; i++) {
        gpio_set_irq_enabled(gpio + i, event_mask, true); // Enable IRQ for the remaining buttons
    }
}
