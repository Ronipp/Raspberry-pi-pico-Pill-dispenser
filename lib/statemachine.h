#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include "pico/stdlib.h"

typedef enum state {
    CALIBRATE,
    HALF_CALIBRATE,
    WAIT_FOR_DISPENSE,
    DISPENSE,
    CHECK_IF_DISPENSED,
    PILL_NOT_DROPPED,
    
} state_enum;

typedef struct sm {
    state_enum state;
    uint pills_dropped;
    uint pills_error; // when pill was supposed to drop but didnt increment this
    uint32_t time_ms;
    uint32_t time_drop_started_ms;
    uint8_t error_blink_counter;
} state_machine;

state_machine statemachine_get(uint pills_dispensed, uint pills_dispensed_with_error) {
    state_machine sm;
    sm.pills_dropped = pills_dispensed;
    sm.pills_error = pills_dispensed_with_error;
    sm.state = ((pills_dispensed + pills_dispensed_with_error) == 0) ? CALIBRATE : HALF_CALIBRATE;
    sm.time_ms = to_ms_since_boot(get_absolute_time());
    sm.time_drop_started_ms = 0;
    sm.error_blink_counter = 0;
    return sm;
}

void state_machine_update_time(state_machine *sm) {
    sm->time_ms = to_ms_since_boot(get_absolute_time());
}

#endif