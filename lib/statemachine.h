#ifndef STATEMACHINE_H
#define STATEMACHINE_H

typedef enum state {
    CALIBRATE,
    HALF_CALIBRATE,
    DISPENSE,
    CHECK_IF_DISPENSED,
    
} state_enum;

typedef struct sm {
    state_enum state;
    uint pills_dropped;
} state_machine;

inline state_machine statemachine_get(uint pills_dispensed) {
    state_machine sm;
    sm.pills_dropped = pills_dispensed;
    sm.state = (pills_dispensed == 0) ? CALIBRATE : HALF_CALIBRATE;
    return sm;
}

#endif