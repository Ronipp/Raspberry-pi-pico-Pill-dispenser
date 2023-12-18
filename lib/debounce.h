#ifndef debounce_h
#define debounce_h

void init_button_with_callback(uint gpio, uint number_of_buttons, uint32_t event_mask, gpio_irq_callback_t callback);
bool debounce(uint32_t *time);

#endif