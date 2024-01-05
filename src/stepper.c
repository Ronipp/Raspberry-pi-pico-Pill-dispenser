#include "pico/stdlib.h"
#include "hardware/pio.h"
#include <stdio.h>

#include "stepper.h"
#include "stepper.pio.h"

#define NPINS 4


#define _arg(x) (x)
#define _remainder(x, y) (_arg(x) % _arg(y))
#define _lessthan(x) (_arg(x) < 0)
#define _ternary(x, y) (_lessthan(_remainder(x, y)) ? _arg(y) : 0)
#define stepper_modulo(x, y) (_remainder(x, y) + _ternary(x, y))
#define SYS_CLK_KHZ 125000 // this is defined in picosdk but this shit is broken and wont import it... asddas.sdgagk.... 

/**
 * Initializes a PIO state machine to control a stepper motor.
 *
 * @param ctx Pointer to the stepper context containing PIO instance, pins, and program offset.
 * @param div The clock divider value for the state machine to set the stepping speed.
 */
static void stepper_pio_init(stepper_ctx *ctx, float div) {
    // Get the default PIO state machine configuration for a clockwise stepper motor program
    pio_sm_config conf = stepper_clockwise_program_get_default_config(ctx->program_offset);
    
    uint32_t pin_mask = 0x0; // Initialize pin mask for setting pins
    
    // Set up the pin mask and initialize GPIO for each pin used by the stepper
    for (int i = 0; i < NPINS; i++) { 
        pin_mask |= 1 << (ctx->pins[i]); // Set the bit for the current pin in the mask
        pio_gpio_init(ctx->pio_instance, ctx->pins[i]); // Initialize GPIO for the pin
    }
    
    // Set the direction of the pins used by the state machine
    pio_sm_set_pindirs_with_mask(ctx->pio_instance, ctx->state_machine, pin_mask, pin_mask);
    
    // Configure the set pins (first 3 pins) and the sideset pin (last pin)
    sm_config_set_set_pins(&conf, ctx->pins[0], 5);
    sm_config_set_sideset_pins(&conf, ctx->pins[3]);
    
    // Set the clock divider, stepping direction, and enable the state machine
    sm_config_set_clkdiv(&conf, div); // Set the clock divider for stepping speed
    sm_config_set_out_shift(&conf, true, false, 0); // Set output shift characteristics
    pio_sm_init(ctx->pio_instance, ctx->state_machine, ctx->program_offset, &conf); // Initialize state machine
    pio_sm_set_enabled(ctx->pio_instance, ctx->state_machine, true); // Enable the state machine
}

/**
 * Calculates the clock divider value required for a specific RPM (Rotations Per Minute)
 * to control the speed of a stepper motor.
 *
 * @param rpm Desired motor speed in RPM (Rotations Per Minute).
 * @return The calculated clock divider value for the given RPM.
 */
static float stepper_calculate_clkdiv(float rpm) {
    if (rpm > RPM_MAX) rpm = RPM_MAX; // Cap the RPM value at the maximum RPM
    if (rpm < RPM_MIN) rpm = RPM_MIN; // Ensure the RPM value is not below the minimum RPM
    
    // Calculate and return the clock divider value based on the desired RPM
    return (SYS_CLK_KHZ * 1000) / (16000 / (((1 / rpm) * 60 * 1000) / 4096));
}

/**
 * Retrieves a stepper motor context with default or initialized values.
 * The stepper context contains information about pins, states, counters, speed, and calibration status.
 *
 * @return Stepper motor context initialized with default values.
 */
stepper_ctx stepper_get_ctx(void) {
    stepper_ctx ctx;
    for (int i=0; i< NPINS; i++) {
        ctx.pins[i] = 0; // Initialize pin values
    }
    ctx.opto_fork_pin = 0;
    ctx.direction = true; 
    ctx.pio_instance = NULL; 
    ctx.program_offset = 0; 
    ctx.state_machine = 0; 
    ctx.speed = 0; 
    ctx.sequence_counter = 0; 
    ctx.step_counter = 0; 
    ctx.step_max = 6000; 
    ctx.edge_steps = 0; 
    ctx.step_memory = 0; 
    ctx.stepper_calibrated = false; 
    ctx.stepper_calibrating = false; 
    return ctx; 
}

/**
 * Initializes a stepper motor context with the provided configuration.
 * Configures the stepper pins, opto fork pin, direction, speed, and initializes the PIO state machine.
 *
 * @param ctx Pointer to the stepper motor context to be initialized.
 * @param pio PIO instance to control the stepper motor.
 * @param stepper_pins Array of stepper motor pins.
 * @param opto_fork_pin Pin connected to the opto fork sensor.
 * @param rpm Speed of the stepper motor in rotations per minute (RPM).
 * @param clockwise Direction of rotation (true for clockwise, false for anti-clockwise).
 */
void stepper_init(
    stepper_ctx *ctx,
    PIO pio,
    const uint *stepper_pins,
    const uint opto_fork_pin,
    const float rpm,
    const bool clockwise
) {
    ctx->pio_instance = pio; // Set PIO instance

    // Configure and initialize the opto fork pin if provided
    if (opto_fork_pin) {
        ctx->opto_fork_pin = opto_fork_pin;
        gpio_set_dir(ctx->opto_fork_pin, GPIO_IN);
        gpio_set_function(ctx->opto_fork_pin, GPIO_FUNC_SIO);
        gpio_pull_up(ctx->opto_fork_pin);
    } else {
        ctx->opto_fork_pin = 0;
    }

    // Copy stepper pins to context
    for (int j = 0; j < NPINS; j++) {
        ctx->pins[j] = stepper_pins[j];
    }

    ctx->direction = clockwise; // Set rotation direction
    ctx->state_machine = pio_claim_unused_sm(ctx->pio_instance, true); // Claim a PIO state machine

    // Add the appropriate program based on the direction
    if (clockwise) {
        ctx->program_offset = pio_add_program(ctx->pio_instance, &stepper_clockwise_program);
    } else {
        ctx->program_offset = pio_add_program(ctx->pio_instance, &stepper_anticlockwise_program);
    }

    ctx->speed = rpm; // Set motor speed
    float div = stepper_calculate_clkdiv(rpm); // Calculate clock divider based on RPM
    stepper_pio_init(ctx, div); // Initialize the stepper PIO
}

/**
 * Turns the stepper motor by the specified number of steps.
 * Uses the PIO state machine to control the stepper motor's movement and updates the step counters.
 *
 * @param ctx Pointer to the stepper motor context.
 * @param steps Number of steps to turn the stepper motor.
 */
void stepper_turn_steps(stepper_ctx *ctx, const uint16_t steps) {
    // Construct a word to send to the PIO state machine
    uint32_t word = ((ctx->program_offset + stepper_clockwise_offset_loop + 3 * ctx->sequence_counter) << 16) | (steps);
    pio_sm_put_blocking(ctx->pio_instance, ctx->state_machine, word); // Send the word to the PIO state machine

    // Update sequence counter to control sequence loops
    ctx->sequence_counter = stepper_modulo(ctx->sequence_counter + steps, 8);

    int16_t steps_to_add = steps;
    if (ctx->direction == STEPPER_ANTICLOCKWISE) {
        steps_to_add = -steps_to_add;
    }

    // Update step counter and memory based on direction and steps moved
    ctx->step_counter = stepper_modulo(ctx->step_counter + steps_to_add, ctx->step_max);
    ctx->step_memory = (ctx->step_memory << 16) | (uint16_t)steps_to_add;
}

/**
 * Rotates the stepper motor by one complete revolution.
 *
 * @param ctx Pointer to the stepper motor context.
 */
void stepper_turn_one_revolution(stepper_ctx *ctx) {
    stepper_turn_steps(ctx, ctx->step_max); // Turns the stepper motor by the number of steps in one complete revolution
}

/**
 * Sets the speed of the stepper motor.
 *
 * @param ctx Pointer to the stepper motor context.
 * @param rpm Desired revolutions per minute (RPM) for the motor.
 */
void stepper_set_speed(stepper_ctx *ctx, const float rpm) {
    pio_sm_set_enabled(ctx->pio_instance, ctx->state_machine, false); // Disable the state machine temporarily
    ctx->speed = rpm; // Update the desired speed in the stepper motor context
    float div = stepper_calculate_clkdiv(rpm); // Calculate the clock divider based on the new speed
    pio_sm_set_clkdiv(ctx->pio_instance, ctx->state_machine, div); // Set the new clock divider
    pio_sm_set_enabled(ctx->pio_instance, ctx->state_machine, true); // Re-enable the state machine with the new speed
}

/**
 * Retrieves the current step position of the stepper motor based on the state of its pins.
 *
 * @param ctx Pointer to the stepper motor context.
 * @return The current step position of the stepper motor.
 */
static uint8_t stepper_get_current_step(const stepper_ctx *ctx) {
    // Read the state of the stepper motor pins to determine the current step position
    uint8_t pins_on_off = gpio_get(ctx->pins[3]) << 3 | gpio_get(ctx->pins[2]) << 2 | gpio_get(ctx->pins[1]) << 1 | gpio_get(ctx->pins[0]);

    // Match the pin configuration to the corresponding step position
    switch (pins_on_off) {
        case 0x01:
            return 0;
        case 0x03:
            return 1;
        case 0x02:
            return 2;
        case 0x06:
            return 3;
        case 0x04:
            return 4;
        case 0x0c:
            return 5;
        case 0x08:
            return 6;
        case 0x09:
            return 7;
    }
}

/**
 * Reads the number of steps remaining in the stepper motor's execution.
 * 
 * This function attempts to calculate the number of steps remaining by analyzing the state
 * of the PIO state machine and the program counter. It retrieves the current
 * program counter value and, based on its position, tries to determine the remaining steps.
 * 
 * @param ctx Pointer to the stepper motor context.
 * @return The number of steps remaining in the stepper motor's execution. Positive values indicate
 * remaining steps in a clockwise direction, negative values in an anticlockwise direction, and 0 when idle.
 */
static int32_t stepper_read_steps_left(const stepper_ctx *ctx) {
    uint8_t pc = pio_sm_get_pc(ctx->pio_instance, ctx->state_machine); // Get the current program counter
    uint8_t loop_offset = ctx->program_offset + stepper_clockwise_offset_loop;
    uint32_t steps_left = 0;

    // Check the program counter value for different states
    if (pc == 0 || pc == 1) {
        // If the program counter is 0 or 1, the program hasn't pulled from the RX FIFO, so no steps executed
        return 0;
    } else if (pc == 2) {
        // If the program counter is 2, steps to take haven't been pushed to the X register yet
        pio_sm_exec(ctx->pio_instance, ctx->state_machine, pio_encode_out(pio_x, 16)); // Push steps to take to the X register
    } else if ((pc >= loop_offset)) {
        // If the program counter is in the loop region
        if (((pc - loop_offset) % 3) == 0) {
            // Depending on the program counter position, it might require adding one more step
            if (((pc - loop_offset) / 3) == stepper_modulo(ctx->sequence_counter + (ctx->direction ? 1 : -1), 8)) {
                steps_left++; // If the PC is on the SET instruction but hasn't decremented X yet
            }
        }
    }

    // Move the X register contents to the ISR, then push ISR contents to the RX FIFO
    pio_sm_exec(ctx->pio_instance, ctx->state_machine, pio_encode_in(pio_x, 32));
    pio_sm_exec(ctx->pio_instance, ctx->state_machine, pio_encode_push(false, false));

    // Read the RX FIFO to determine the number of executed steps
    steps_left += pio_sm_get(ctx->pio_instance, ctx->state_machine);

    // Return the calculated number of steps remaining, considering the motor's direction
    return ctx->direction ? steps_left : -steps_left;
}

/**
 * Stops the stepper motor's operation and resets the state machine to an idle state.
 * 
 * This function halts the stepper motor's execution by disabling the PIO state machine,
 * ensuring that any remaining steps in the execution are accounted for and removed from
 * the step counter. It adjusts the sequence counter and step counter to reflect the current
 * motor state accurately. Additionally, it clears any pending commands in the FIFO, resets
 * the state machine's program counter to 0, and re-enables the state machine for further operation.
 * 
 * @param ctx Pointer to the stepper motor context.
 */
void stepper_stop(stepper_ctx *ctx) {
    pio_sm_set_enabled(ctx->pio_instance, ctx->state_machine, false); // Disable the state machine

    // Adjust sequence counter based on the current step and direction
    ctx->sequence_counter = stepper_modulo(stepper_get_current_step(ctx) + 1, 8);
    if (ctx->direction == STEPPER_ANTICLOCKWISE) {
        ctx->sequence_counter = stepper_modulo(7 - (ctx->sequence_counter - 1) + 1, 8);
    }

    // Retrieve the number of commands in the FIFO and the number of steps left in execution
    uint fifo_level = pio_sm_get_tx_fifo_level(ctx->pio_instance, ctx->state_machine);
    int32_t steps_left = stepper_read_steps_left(ctx);

    // Adjust the step counter based on the executed and remaining steps
    ctx->step_counter = stepper_modulo(ctx->step_counter - steps_left, ctx->step_max);

    // Subtract the steps added by the commands in the FIFO from the step counter
    for (int i = 0; i < fifo_level; i++) {
        ctx->step_counter = stepper_modulo(ctx->step_counter - (int16_t)(ctx->step_memory & 0xffff), ctx->step_max);
        ctx->step_memory >>= 16;
    }

    // Clear any pending commands in the FIFO
    pio_sm_clear_fifos(ctx->pio_instance, ctx->state_machine);

    // Reset the state machine's program counter to 0 and re-enable the state machine
    pio_sm_exec(ctx->pio_instance, ctx->state_machine, pio_encode_jmp(0));
    pio_sm_set_enabled(ctx->pio_instance, ctx->state_machine, true);
}

/**
 * Sets the direction of the stepper motor and updates the associated state machine.
 * 
 * This function changes the direction of the stepper motor to the specified direction (clockwise or anticlockwise).
 * If the direction is different from the current one, it stops the motor, reconfigures the state machine's program
 * according to the new direction, and resets the sequence counter accordingly. It ensures the state machine is disabled
 * before reconfiguration to avoid any potential conflicts. Finally, it re-enables the state machine.
 * 
 * @param ctx Pointer to the stepper motor context.
 * @param clockwise Boolean indicating the direction: true for clockwise, false for anticlockwise.
 */
void stepper_set_direction(stepper_ctx *ctx, bool clockwise) {
    if (clockwise != stepper_get_direction(ctx)) { // Check if the direction needs to be changed
        if (stepper_is_running(ctx)) stepper_stop(ctx); // Stop the motor before changing direction

        pio_sm_set_enabled(ctx->pio_instance, ctx->state_machine, false); // Disable the state machine
        ctx->direction = clockwise; // Update the direction

        pio_clear_instruction_memory(ctx->pio_instance); // Clear the current program in the PIO

        // Set the new program based on the direction and adjust the sequence counter
        if (clockwise) {
            ctx->program_offset = pio_add_program(ctx->pio_instance, &stepper_clockwise_program);
            ctx->sequence_counter = stepper_modulo(7 - (ctx->sequence_counter - 1), 8);
        } else {
            ctx->program_offset = pio_add_program(ctx->pio_instance, &stepper_anticlockwise_program);
            ctx->sequence_counter = stepper_modulo(7 - (ctx->sequence_counter - 1) + 1, 8);
        }

        pio_sm_set_enabled(ctx->pio_instance, ctx->state_machine, true); // Re-enable the state machine
    }
}

static float original_speed;

bool stage;
static stepper_ctx *tmp_ctx = NULL;

/**
 * Calibration handler function to manage stepper motor calibration based on opto fork sensor signals.
 * 
 * This function is invoked when edge-rise or edge-fall interrupts are detected on the opto fork pin.
 * It handles the calibration process by adjusting the stepper's steps, directions, and max steps to calibrate
 * the motor's position. It stops and restarts the motor as needed and eventually sets the motor as calibrated.
 * 
 * @note The function uses a temporary context (`tmp_ctx`) that likely represents the stepper motor context.
 */
static void calibration_handler(void) {
    pio_sm_set_enabled(tmp_ctx->pio_instance, tmp_ctx->state_machine, false);
    if (gpio_get_irq_event_mask(tmp_ctx->opto_fork_pin) & GPIO_IRQ_EDGE_RISE) {
        gpio_acknowledge_irq(tmp_ctx->opto_fork_pin, GPIO_IRQ_EDGE_RISE);
        if (stage == true) { // this should happen second.
            stepper_stop(tmp_ctx);
            if (tmp_ctx->direction == STEPPER_CLOCKWISE) { // we are over the second edge of the "hole"
                tmp_ctx->edge_steps = tmp_ctx->step_counter; // so we keep that for later
            } else { // the math is a bit different if the stepper is going anticlockwise
                tmp_ctx->edge_steps = tmp_ctx->step_max - tmp_ctx->step_counter;
            }
            stepper_turn_steps(tmp_ctx, tmp_ctx->step_max);
        } else { // in case something weird happens just let the stepper run
            pio_sm_set_enabled(tmp_ctx->pio_instance, tmp_ctx->state_machine, true);
        }
        return;
    } else if (gpio_get_irq_event_mask(tmp_ctx->opto_fork_pin) & GPIO_IRQ_EDGE_FALL) {
        gpio_acknowledge_irq(tmp_ctx->opto_fork_pin, GPIO_IRQ_EDGE_FALL);
        if (stage == false) { // this should happen first
            stepper_stop(tmp_ctx);
            tmp_ctx->step_counter = 0; // we are at the edge of the "hole" so step counter gets set to 0.
            stage = true;
            stepper_turn_steps(tmp_ctx, tmp_ctx->step_max); // set the motor in motion again.
        } else { // this is the last step in calibration
            stepper_stop(tmp_ctx);
            if (tmp_ctx->direction == STEPPER_CLOCKWISE) {
                tmp_ctx->step_max = tmp_ctx->step_counter; // motor has made one whole turn so set the max steps equal to step counter
                tmp_ctx->step_counter = tmp_ctx->step_max - (tmp_ctx->edge_steps / 2); // use the second edge to calculate where the "true" zero should be.
            } else { // different math for different direction
                tmp_ctx->step_max = tmp_ctx->step_max - tmp_ctx->step_counter;
                tmp_ctx->step_counter = tmp_ctx->edge_steps / 2; 
                
            }
            stepper_set_speed(tmp_ctx, original_speed);
            stepper_turn_steps(tmp_ctx, tmp_ctx->edge_steps / 2); // go to "true" zero
            tmp_ctx->stepper_calibrated = true;
            tmp_ctx->stepper_calibrating = false;
            gpio_set_irq_enabled(tmp_ctx->opto_fork_pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);
            gpio_remove_raw_irq_handler(tmp_ctx->opto_fork_pin, calibration_handler);
        }
    } else {
        pio_sm_set_enabled(tmp_ctx->pio_instance, tmp_ctx->state_machine, true); // incase optofork didnt call this function
    }
}

/*
DO NOT CHANGE CTX WHILE CALIBRATION IS RUNNING!!
DO NOT REMOVE OR LET CONTEXT FALL OUT OF SCOPE WHILE CALIBRATION IS RUNNING!!
Calling functions where ctx is marked const is fine.
*/
void stepper_calibrate(stepper_ctx *ctx) {
    // if stepper is already calibrating we dont want to calibrate again until its not calibrating
    if (ctx->stepper_calibrating) return;
    ctx->step_max = 6000; // 6000 is a safe number we need this to be more than the actual max steps.
    original_speed = ctx->speed;
    stage = false;
    ctx->stepper_calibrated = false;
    ctx->stepper_calibrating = true;
    tmp_ctx = ctx; // setting the main program ctx to be used with callback.
    // set interrupts on opto fork pin.
    gpio_add_raw_irq_handler_with_order_priority(ctx->opto_fork_pin, calibration_handler, PICO_HIGHEST_IRQ_PRIORITY);
    gpio_set_irq_enabled(ctx->opto_fork_pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    if (!irq_is_enabled(IO_IRQ_BANK0)) irq_set_enabled(IO_IRQ_BANK0, true);
    // set the stepper in motion and wait for interrupts
    stepper_set_speed(ctx, RPM_MAX);
    stepper_turn_steps(ctx, ctx->step_max);
}

static uint dispensed_pills;

/**
 * Handler function for half calibration using an opto fork sensor signal.
 * 
 * This function manages a specific phase of stepper motor calibration using opto fork sensor signals.
 * It handles interrupts generated by the opto fork pin (both edge-fall and edge-rise) and adjusts the motor's
 * position and calibration state accordingly. It stops and restarts the motor, sets the direction, dispenses pills,
 * and eventually sets the motor as calibrated.
 * 
 * @note The function uses a temporary context (`tmp_ctx`) representing the stepper motor context.
 */
static void half_calibration_handler(void) {
    pio_sm_set_enabled(tmp_ctx->pio_instance, tmp_ctx->state_machine, false);
    if (gpio_get_irq_event_mask(tmp_ctx->opto_fork_pin) & GPIO_IRQ_EDGE_FALL) {
        gpio_acknowledge_irq(tmp_ctx->opto_fork_pin, GPIO_IRQ_EDGE_FALL);
        stepper_stop(tmp_ctx);
        stage = true;
        stepper_set_direction(tmp_ctx, STEPPER_CLOCKWISE);
        stepper_turn_steps(tmp_ctx, tmp_ctx->step_max);
    } else {
        gpio_acknowledge_irq(tmp_ctx->opto_fork_pin, GPIO_IRQ_EDGE_RISE);
        if (stage == true) {
            stepper_stop(tmp_ctx);
            tmp_ctx->step_counter = tmp_ctx->edge_steps / 2;
            if (dispensed_pills != 0) {
                stepper_turn_steps(tmp_ctx, (dispensed_pills * tmp_ctx->step_max / 8) - tmp_ctx->step_counter);
            }
            tmp_ctx->stepper_calibrated = true;
            tmp_ctx->stepper_calibrating = false;
            gpio_set_irq_enabled(tmp_ctx->opto_fork_pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);
            gpio_remove_raw_irq_handler(tmp_ctx->opto_fork_pin, half_calibration_handler);
        } else {
            pio_sm_set_enabled(tmp_ctx->pio_instance, tmp_ctx->state_machine, true);
        }
    }
    
}

/**
 * Initiates a half calibration routine for the stepper motor using an opto fork sensor signal.
 * 
 * This function sets up and triggers a half calibration process for a stepper motor using an opto fork sensor.
 * It sets various parameters required for calibration, such as step limits, edge steps, dispensed pills, and
 * handles the process via an interrupt-driven mechanism with the half_calibration_handler function.
 * 
 * @param ctx             The context representing the stepper motor to be calibrated.
 * @param max_steps       The maximum steps of the motor.
 * @param edge_steps      The number of steps at the edge of the "hole" in the opto fork sensor.
 * @param pills_dispensed The number of pills dispensed during calibration.
 * 
 * @note This function assumes a temporary context `tmp_ctx` representing the stepper motor context.
 */
void stepper_half_calibrate(stepper_ctx *ctx, uint16_t max_steps, uint16_t edge_steps, uint pills_dispensed) {
    if (ctx->stepper_calibrating) return;
    ctx->step_max = max_steps;
    ctx->edge_steps = edge_steps;
    original_speed = ctx->speed;
    dispensed_pills = pills_dispensed;
    stage = false;
    tmp_ctx = ctx;
    ctx->stepper_calibrated = false;
    ctx->stepper_calibrating = true;

    gpio_add_raw_irq_handler_with_order_priority(ctx->opto_fork_pin, half_calibration_handler, PICO_HIGHEST_IRQ_PRIORITY);
    gpio_set_irq_enabled(ctx->opto_fork_pin, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);
    if (!irq_is_enabled(IO_IRQ_BANK0)) irq_set_enabled(IO_IRQ_BANK0, true);

    stepper_set_direction(ctx, STEPPER_ANTICLOCKWISE);
    stepper_set_speed(ctx, RPM_MAX);
    stepper_turn_steps(ctx, max_steps);
}

/**
 * Checks if the stepper motor is currently running.
 * 
 * This function checks whether the stepper motor associated with the provided context is currently running by inspecting
 * the program counter and the TX FIFO level of the PIO state machine.
 * 
 * @param ctx The context representing the stepper motor to be checked.
 * @return True if the stepper motor is running, false otherwise.
 */
bool stepper_is_running(const stepper_ctx *ctx) {
    // Check if the program counter is 0 and the TX FIFO level is 0
    return !((pio_sm_get_pc(ctx->pio_instance, ctx->state_machine) == 0) && 
             (pio_sm_get_tx_fifo_level(ctx->pio_instance, ctx->state_machine) == 0));
}

/**
 * Checks if the stepper motor is calibrated.
 * 
 * This function checks whether the stepper motor associated with the provided context has been calibrated.
 * 
 * @param ctx The context representing the stepper motor to be checked.
 * @return True if the stepper motor is calibrated, false otherwise.
 */
bool stepper_is_calibrated(const stepper_ctx *ctx) {
    return ctx->stepper_calibrated;
}

/**
 * Checks if the stepper motor is currently in the process of calibration.
 * 
 * This function checks whether the stepper motor associated with the provided context is
 * currently being calibrated.
 * 
 * @param ctx The context representing the stepper motor to be checked.
 * @return True if the stepper motor is currently calibrating, false otherwise.
 */
bool stepper_is_calibrating(const stepper_ctx *ctx) {
    return ctx->stepper_calibrating;
}

/**
 * Retrieves the maximum steps configured for the stepper motor.
 * 
 * This function retrieves the maximum steps configured for the stepper motor
 * associated with the provided context.
 * 
 * @param ctx The context representing the stepper motor.
 * @return The maximum steps configured for the stepper motor.
 */
uint16_t stepper_get_max_steps(const stepper_ctx *ctx) {
    return ctx->step_max;
}

/**
 * Retrieves the number of edge steps configured for the stepper motor.
 * 
 * This function retrieves the number of edge steps configured for the stepper motor
 * associated with the provided context.
 * 
 * @param ctx The context representing the stepper motor.
 * @return The number of edge steps configured for the stepper motor.
 */
uint16_t stepper_get_edge_steps(const stepper_ctx *ctx) {
    return ctx->edge_steps;
}

/**
 * Retrieves the current step count of the stepper motor.
 * 
 * This function retrieves the current step count of the stepper motor
 * associated with the provided context.
 * 
 * @param ctx The context representing the stepper motor.
 * @return The current step count of the stepper motor.
 */
int16_t stepper_get_step_count(const stepper_ctx *ctx) {
    return ctx->step_counter;
}

/**
 * Retrieves the current direction of the stepper motor.
 * 
 * This function retrieves the current direction of the stepper motor
 * associated with the provided context.
 * 
 * @param ctx The context representing the stepper motor.
 * @return The current direction of the stepper motor (true for clockwise, false for anticlockwise).
 */
bool stepper_get_direction(const stepper_ctx *ctx) {
    return ctx->direction;
}
