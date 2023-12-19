#ifndef STEPPER_H
#define STEPPER_H

#include "hardware/pio.h"

#define STEPPER_CLOCKWISE true
#define STEPPER_ANTICLOCKWISE false

#define RPM_MAX 15.
#define RPM_MIN 1.8

typedef enum _stepper_pins{
    BLUE = 2,
    PINK = 3,
    YELLOW = 6,
    ORANGE = 13
} stepper_pins;

typedef struct stepper_ctx{
    uint pins[4];
    uint opto_fork_pin;
    int8_t sequence_counter;
    int16_t step_counter;
    uint64_t step_memory; // magic buffer... :D
    uint16_t step_max;
    uint16_t edge_steps;
    bool direction;
    float speed;
    bool stepper_calibrated;
    bool stepper_calibrating;
    bool running;
    PIO pio_instance;
    uint state_machine;
    uint program_offset;
} stepper_ctx;

stepper_ctx stepper_get_ctx(void);
void stepper_init(stepper_ctx *ctx, PIO pio, const uint *stepper_pins, const uint opto_fork_pin, const float rpm, const bool clockwise);

void stepper_turn_steps(stepper_ctx *ctx, const uint16_t steps);
void stepper_turn_one_revolution(stepper_ctx *ctx);
void stepper_set_speed(stepper_ctx *ctx, float rpm);
void stepper_stop(stepper_ctx *ctx);
void stepper_set_direction(stepper_ctx *ctx, bool clockwise);
void stepper_calibrate(stepper_ctx *ctx);
void stepper_half_calibrate(stepper_ctx *ctx, uint16_t max_steps, uint16_t edge_steps, uint pills_dispensed);

bool stepper_is_running(const stepper_ctx *ctx);
bool stepper_is_calibrated(const stepper_ctx *ctx);
bool stepper_is_calibrating(const stepper_ctx *ctx);
uint16_t stepper_get_max_steps(const stepper_ctx *ctx);
int16_t stepper_get_step_count(const stepper_ctx *ctx);
bool stepper_get_direction(const stepper_ctx *ctx);
uint16_t stepper_get_edge_steps(const stepper_ctx *ctx);

#endif