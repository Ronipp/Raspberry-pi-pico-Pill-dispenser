#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <stdio.h>
#include <stdbool.h>


#define EEPROM_ADDRESS 0x50

static uint64_t write_cycle_max = 0;
static absolute_time_t write_init_time;

static inline bool eeprom_write_cycle_check() {
    if (absolute_time_diff_us(write_init_time, get_absolute_time()) > write_cycle_max) {
        return true;
    }
    return false;
}

static inline void eeprom_write_cycle_block() {
    if (!eeprom_write_cycle_check()) {
        sleep_until(delayed_by_us(write_init_time, write_cycle_max));
    }
}

void eeprom_init_i2c(i2c_inst_t *i2c, uint baud, uint32_t write_cycle_max_ms) {
    // i2c0 uses pin 16 for sda 17 for scl, i2c1 uses 19 and 20.
    uint sda_pin = (i2c == i2c0) ? 16 : 19;
    uint scl_pin = (i2c == i2c0) ? 17 : 20;
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_set_dir(sda_pin, GPIO_OUT);
    gpio_set_dir(scl_pin, GPIO_OUT);
    i2c_init(i2c0, baud);
    write_cycle_max = (uint64_t) write_cycle_max_ms * 1000;
    write_init_time = nil_time;
}

void eeprom_write_address(uint16_t address) {
    eeprom_write_cycle_block();
    uint8_t out[2] = {address >> 4, address};
    i2c_write_blocking(i2c0, EEPROM_ADDRESS, out, 2, true);
}


void eeprom_write_byte(uint16_t address, char c) {
    uint8_t out[3] = {address >> 4, address, c};
    eeprom_write_cycle_block();
    i2c_write_blocking(i2c0, EEPROM_ADDRESS, out, 3, false);
    write_init_time = get_absolute_time();
}

void eeprom_write_page(uint16_t address, uint8_t *src, size_t size) {
    uint8_t out[size+2];
    out[0] = address >> 4;
    out[1] = address;
    for (int i=0; i<size; i++) {
        out[i+2] = src[i];
    }
    eeprom_write_cycle_block();
    i2c_write_blocking(i2c0, EEPROM_ADDRESS, out, size+2, false);
    write_init_time = get_absolute_time();
}

char eeprom_read_byte(uint16_t address) {
    char c = 0;
    eeprom_write_address(address);
    i2c_read_blocking(i2c0, EEPROM_ADDRESS, &c, 1, false);
    return c;
}

void eeprom_read_page(uint16_t address, uint8_t *dst, size_t size) {
    eeprom_write_address(address);
    i2c_read_blocking(i2c0, EEPROM_ADDRESS, dst, size, false);
}