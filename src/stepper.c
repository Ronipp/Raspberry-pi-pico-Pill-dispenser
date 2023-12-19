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

static void stepper_pio_init(stepper_ctx *ctx, float div) {
    pio_sm_config conf = stepper_clockwise_program_get_default_config(ctx->program_offset);
    uint32_t pin_mask = 0x0;
    for (int i=0; i< NPINS; i++) { 
        pin_mask |= 1 << (ctx->pins[i]);
        pio_gpio_init(ctx->pio_instance, ctx->pins[i]);
    }
    pio_sm_set_pindirs_with_mask(ctx->pio_instance, ctx->state_machine, pin_mask, pin_mask);
    sm_config_set_set_pins(&conf, ctx->pins[0], 5); // first 3 pins are set pins
    sm_config_set_sideset_pins(&conf, ctx->pins[3]); // last pin as sideset.
    // float div = 65536;
    sm_config_set_clkdiv(&conf, div);
    sm_config_set_out_shift(&conf, true, false, 0);
    pio_sm_init(ctx->pio_instance, ctx->state_machine, ctx->program_offset, &conf);
    pio_sm_set_enabled(ctx->pio_instance, ctx->state_machine, true);
}

static float stepper_calculate_clkdiv(float rpm) {
    if (rpm > RPM_MAX) rpm = RPM_MAX;
    if (rpm < RPM_MIN) rpm = RPM_MIN;
    return (SYS_CLK_KHZ * 1000) / (16000 / (((1 / rpm) * 60 * 1000) / 4096));
}

stepper_ctx stepper_get_ctx(void) {
    stepper_ctx ctx;
    for (int i=0; i< NPINS; i++) {
        ctx.pins[i] = 0;
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
    ctx.running = false;
    return ctx;
}

static stepper_ctx *tmp_ctx = NULL;

void pio_interrupt_handler(void) {
    if (pio_interrupt_get(tmp_ctx->pio_instance, stepper_clockwise_irq_num)) {
        pio_interrupt_clear(tmp_ctx->pio_instance, stepper_clockwise_irq_num);
        tmp_ctx->running = false;
    }
}

void stepper_init(stepper_ctx *ctx, PIO pio, const uint *stepper_pins, const uint opto_fork_pin, const float rpm, const bool clockwise) {
    ctx->pio_instance = pio;
    if (opto_fork_pin) {
        ctx->opto_fork_pin = opto_fork_pin;
        gpio_set_dir(ctx->opto_fork_pin, GPIO_IN);
        gpio_set_function(ctx->opto_fork_pin, GPIO_FUNC_SIO);
        gpio_pull_up(ctx->opto_fork_pin);
    } else {
        ctx->opto_fork_pin = 0;
    }
    for (int j=0; j<NPINS; j++) {
        ctx->pins[j] = stepper_pins[j];
    }
    ctx->direction = clockwise;
    ctx->state_machine = pio_claim_unused_sm(ctx->pio_instance, true);
    if (clockwise) {
        ctx->program_offset = pio_add_program(ctx->pio_instance, &stepper_clockwise_program);
    } else {
        ctx->program_offset = pio_add_program(ctx->pio_instance, &stepper_anticlockwise_program);
    }
    ctx->speed = rpm;
    float div = stepper_calculate_clkdiv(rpm);
    stepper_pio_init(ctx, div);
    // set up pio interrupt to see when pio is stopped.
    tmp_ctx = ctx; // this is a pointer to the context so we can modify it via interrupt handlers.
    pio_set_irq0_source_enabled(ctx->pio_instance, pis_interrupt0, true);
    irq_set_exclusive_handler((pio == pio0) ? PIO0_IRQ_0 : PIO1_IRQ_0, pio_interrupt_handler);
    irq_set_enabled((pio == pio0) ? PIO0_IRQ_0 : PIO1_IRQ_0, true);
}

void stepper_turn_steps(stepper_ctx *ctx, const uint16_t steps) {
    ctx->running = true;
    uint32_t word = ((ctx->program_offset + stepper_clockwise_offset_loop + 3 * ctx->sequence_counter) << 16) | (steps);
    pio_sm_put_blocking(ctx->pio_instance, ctx->state_machine, word);
    ctx->sequence_counter = stepper_modulo(ctx->sequence_counter + steps, 8);
    int16_t steps_to_add = steps;
    if (ctx->direction == STEPPER_ANTICLOCKWISE) steps_to_add = -steps_to_add;
    ctx->step_counter = stepper_modulo(ctx->step_counter + steps_to_add, ctx->step_max);
    ctx->step_memory = (ctx->step_memory << 16) | (uint16_t)steps_to_add;
}

void stepper_go_to_step(stepper_ctx *ctx, const uint16_t step) {
    uint16_t steps = 
}

void stepper_turn_one_revolution(stepper_ctx *ctx) {
    stepper_turn_steps(ctx, ctx->step_max);
}

void stepper_set_speed(stepper_ctx *ctx, const float rpm) {
    pio_sm_set_enabled(ctx->pio_instance, ctx->state_machine, false);
    ctx->speed = rpm;
    float div = stepper_calculate_clkdiv(rpm);
    pio_sm_set_clkdiv(ctx->pio_instance, ctx->state_machine, div);
    pio_sm_set_enabled(ctx->pio_instance, ctx->state_machine, true);
}

static uint8_t stepper_get_current_step(const stepper_ctx *ctx) {
    uint8_t pins_on_off = gpio_get(ctx->pins[3]) << 3 | gpio_get(ctx->pins[2]) << 2 | gpio_get(ctx->pins[1]) << 1 | gpio_get(ctx->pins[0]);
    // 0x01, 0x03, 0x02, 0x06, 0x04, 0x0c, 0x08, 0x09
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

static int32_t stepper_read_steps_left(const stepper_ctx *ctx) {
    /*
    what the actual f is going on this does not work when stepping through using debugger...
    but when this is just ran normally it works??! wtf
    */
    uint8_t pc = pio_sm_get_pc(ctx->pio_instance, ctx->state_machine); // get pio program counter
    uint8_t loop_offset = ctx->program_offset + stepper_clockwise_offset_loop;
    uint32_t steps_left = 0;
    if (pc == 0 || pc == 1) { // if pc is 0 or 1 the program hasn't pulled from the rxfifo so no need to do anything
        return 0;
    } else if (pc == 2) {
        // if pc is 2 steps to take hasn't been pushed to x register yet, so we need to do it.
        pio_sm_exec(ctx->pio_instance, ctx->state_machine, pio_encode_out(pio_x, 16));
    } else if ((pc >= loop_offset)) {
        // im assuming program counter is incremented after delay cycles on set instructions.
        if (((pc - loop_offset) % 3) == 0) { // depending on where the pc is we may need to add one more step
            if (((pc - loop_offset) / 3) == stepper_modulo(ctx->sequence_counter + (ctx->direction) ? 1 : -1, 8)) {
                steps_left++; // if pc is on set instruction but hasnt decremented x yet.
            }
        }
    }
    pio_sm_exec(ctx->pio_instance, ctx->state_machine, pio_encode_in(pio_x, 32)); // move x register contents to isr
    pio_sm_exec(ctx->pio_instance, ctx->state_machine, pio_encode_push(false, false)); // push isr contents to rx fifo
    steps_left += pio_sm_get(ctx->pio_instance, ctx->state_machine); // read rx fifo.
    if (ctx->direction) {
        return steps_left;
    } else {
        return -steps_left;
    }
}

void stepper_stop(stepper_ctx *ctx) {
    pio_sm_set_enabled(ctx->pio_instance, ctx->state_machine, false);
    // get current step and increment it by one so the next pio instruction will be the next step.
    ctx->sequence_counter = stepper_modulo(stepper_get_current_step(ctx) + 1, 8);
    if (ctx->direction == STEPPER_ANTICLOCKWISE) {
        // if direction is counterclockwise sequence counter is fixed accordingly
        ctx->sequence_counter = stepper_modulo(7 - (ctx->sequence_counter - 1) + 1, 8);
    }
    // how many "commands" are there in the fifo.
    uint fifo_level = pio_sm_get_tx_fifo_level(ctx->pio_instance, ctx->state_machine);
    int32_t steps_left = stepper_read_steps_left(ctx);
    // steps were added to step counter when put command was issued. now we take away steps left from the counter.
    ctx->step_counter = stepper_modulo(ctx->step_counter - steps_left, ctx->step_max);
    for (int i=0; i<fifo_level; i++) {
        // if fifo not empty remove the steps added in those commands.
        ctx->step_counter = stepper_modulo(ctx->step_counter - (int16_t)(ctx->step_memory & 0xffff), ctx->step_max);
        ctx->step_memory >>= 16;
    }
    // clear any "run" commands that might be queued in fifos.
    pio_sm_clear_fifos(ctx->pio_instance, ctx->state_machine);
    // set state machines program counter to 0.
    pio_sm_exec(ctx->pio_instance, ctx->state_machine, pio_encode_jmp(0));
    pio_sm_set_enabled(ctx->pio_instance, ctx->state_machine, true);
}


void stepper_set_direction(stepper_ctx *ctx, bool clockwise) {
    if (clockwise != stepper_get_direction(ctx)) {
        // stop the stepper motor before changing directions.
        if (stepper_is_running(ctx)) stepper_stop(ctx);
        pio_sm_set_enabled(ctx->pio_instance, ctx->state_machine, false);
        ctx->direction = clockwise;
        // clear the current program in the pio
        pio_clear_instruction_memory(ctx->pio_instance);
        // set new program according to the direction.
        // also calculate sequence counter. based on the structure of pio programs.
        if (clockwise) {
            ctx->program_offset = pio_add_program(ctx->pio_instance, &stepper_clockwise_program);
            ctx->sequence_counter = stepper_modulo(7 - (ctx->sequence_counter - 1), 8);
        } else {
            ctx->program_offset = pio_add_program(ctx->pio_instance, &stepper_anticlockwise_program);
            ctx->sequence_counter = stepper_modulo(7 - (ctx->sequence_counter - 1) + 1, 8);
        }
        pio_sm_set_enabled(ctx->pio_instance, ctx->state_machine, true);
    }
}


static float original_speed;

bool stage;

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
    }
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
    gpio_set_irq_enabled(ctx->opto_fork_pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    irq_set_enabled(IO_IRQ_BANK0, true);
    gpio_add_raw_irq_handler(ctx->opto_fork_pin, calibration_handler);
    // set the stepper in motion and wait for interrupts
    stepper_set_speed(ctx, RPM_MAX);
    stepper_turn_steps(ctx, ctx->step_max);
}

static uint dispensed_pills;

static void half_calibration_handler(void) {
    pio_sm_set_enabled(tmp_ctx->pio_instance, tmp_ctx->state_machine, false);
    gpio_acknowledge_irq(tmp_ctx->opto_fork_pin, GPIO_IRQ_EDGE_FALL);
    stepper_stop(tmp_ctx);
    tmp_ctx->step_counter = tmp_ctx->edge_steps / 2;
    
}

void stepper_half_calibrate(stepper_ctx *ctx, uint16_t max_steps, uint16_t edge_steps, uint pills_dispensed) {
    if (ctx->stepper_calibrating) return;
    ctx->step_max = max_steps;
    ctx->edge_steps = edge_steps;
    original_speed = ctx->speed;
    dispensed_pills = pills_dispensed;
    ctx->stepper_calibrated = false;
    ctx->stepper_calibrating = true;

    gpio_set_irq_enabled(ctx->opto_fork_pin, GPIO_IRQ_EDGE_FALL, true);
    irq_set_enabled(IO_IRQ_BANK0, true);
    gpio_add_raw_irq_handler(ctx->opto_fork_pin, half_calibration_handler);

    stepper_set_direction(ctx, STEPPER_ANTICLOCKWISE);
    stepper_set_speed(ctx, RPM_MAX);
    stepper_turn_steps(ctx, max_steps);
}

bool stepper_is_running(const stepper_ctx *ctx) {
    return ctx->running;
}

bool stepper_is_calibrated(const stepper_ctx *ctx) {
    return ctx->stepper_calibrated;
}

bool stepper_is_calibrating(const stepper_ctx *ctx) {
    return ctx->stepper_calibrating;
}

uint16_t stepper_get_max_steps(const stepper_ctx *ctx) {
    if (ctx->stepper_calibrated) return ctx->step_max;
    return 0;
}

uint16_t stepper_get_edge_steps(const stepper_ctx *ctx) {
    if (ctx->stepper_calibrated) return ctx->edge_steps;
    return 0;
}

int16_t stepper_get_step_count(const stepper_ctx *ctx) {
    return ctx->step_counter;
}

bool stepper_get_direction(const stepper_ctx *ctx) {
    return ctx->direction;
}