#ifndef STATEMACHINE_H
#define STATEMACHINE_H

typedef enum state {
    CALIBRATE,
    DISPENSE,
    CHECK_IF_DISPENSED,
    
} state_enum;

typedef struct sm {
    state_enum state;
    uint pills_dispensed;
} state_machine;

#endif