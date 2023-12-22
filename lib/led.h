#ifndef LED_H
#define LED_H

void led_init(void);
void led_off(void);
void led_on(void);
void led_toggle(void);
void led_wait_toggle(uint32_t time);
void led_error_toggle(uint32_t time);
void led_calibration_toggle(uint32_t time);
void led_run_toggle(uint32_t time);
bool led_timer(uint32_t time, uint32_t delay);


#endif