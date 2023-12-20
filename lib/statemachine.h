#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include "pico/stdlib.h"

typedef enum state {
    CALIBRATE,
    HALF_CALIBRATE,
    DISPENSE,
    CHECK_IF_DISPENSED,
    
} state_enum;

typedef struct sm {
    state_enum state;
    uint pills_dropped;
    uint32_t time_ms;
} state_machine;

state_machine statemachine_get(uint pills_dispensed) {
    state_machine sm;
    sm.pills_dropped = pills_dispensed;
    sm.state = (pills_dispensed == 0) ? CALIBRATE : HALF_CALIBRATE;
    sm.time_ms = to_ms_since_boot(get_absolute_time());
    return sm;
}

void state_machine_update_time(state_machine *sm) {
    sm->time_ms = to_ms_since_boot(get_absolute_time());
}

#endif