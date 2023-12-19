#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "debounce.h"
#include "stepper.h"
#include "lora.h"
#include "eeprom.h"
#include "magnusFuncs.h"

#include "hardware/pio.h"

#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define STEPPER_OPTO_FORK_PIN 28

typedef enum state
{
    a,
    b,
    c,
} state;

int main()
{
    stdio_init_all();
    eeprom_init_i2c(i2c0, 1000000, 5);
    // lora_init(uart1, UART_TX_PIN, UART_RX_PIN);

    uint stepperpins[4] = {BLUE, PINK, YELLOW, ORANGE};
    stepper_ctx step_ctx = stepper_get_ctx();
    // stepper_init(&step_ctx, pio0, );

    struct rebootValues eepromRebootValues;   // Holds values read from EEPROM
    struct rebootValues watchdogRebootValues; // Holds values read from watchdog

    uint8_t arr[4] = {0, 1, 5, 255};

    enterLogToEeprom(arr);

    if (reboot_sequence(&eepromRebootValues, &watchdogRebootValues) == true)
    {
        printf("crc true\n");
        printf("Pill Dispense State: %d\n", eepromRebootValues.pillDispenseState);
        printf("Reboot Status Code: %d\n", eepromRebootValues.rebootStatusCode);
        printf("Previous Calibration Step Count: %d\n", eepromRebootValues.prevCalibStepCount);
    }
    else
    {
        printf("crc false\n");
    }

    state sm = a;

    while (1)
    {
        switch (sm)
        {
        case a:
            break;
        }
    }
}