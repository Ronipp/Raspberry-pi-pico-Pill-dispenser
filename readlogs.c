#include "pico/stdlib.h"
#include "eeprom.h"
#include "logHandling.h"

int main() {

    stdio_init_all();
    eeprom_init_i2c(i2c0, 1000000, 5);

    // printValidLogs();
    zeroAllLogs();

    while (1) {
        sleep_ms(100);
    }

}