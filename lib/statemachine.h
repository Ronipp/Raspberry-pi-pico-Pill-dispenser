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
    uint32_t time_ms;
    uint32_t time_pill_dropped_ms;
} state_machine;

state_machine statemachine_get(uint pills_dispensed) {
    state_machine sm;
    sm.pills_dropped = pills_dispensed;
    sm.state = (pills_dispensed == 0) ? CALIBRATE : HALF_CALIBRATE;
    sm.time_ms = to_ms_since_boot(get_absolute_time());
    sm.time_pill_dropped_ms = 0;
    return sm;
}

void state_machine_update_time(state_machine *sm) {
    sm->time_ms = to_ms_since_boot(get_absolute_time());
}

#endif