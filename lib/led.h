#ifndef LED_H
#define LED_H

void led_init(void);
void led_wait_toggle(uint32_t time);
void led_calibration_toggle(uint32_t time);
void led_off(void);
void led_on(void);
bool led_timer(uint32_t time);


#endif