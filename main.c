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
#include <time.h>
#include "stdlib.h"

#include "hardware/pio.h"

#define UART_TX_PIN
#define UART_RX_PIN
#define EEPROM_ARR_LENGTH 64
#define LOG_START_ADDR 0

typedef enum state
{
    a,
    b,
    c,
} state;

char *rebootStatusCodes[20] = {
    "Boot",
    "Button press",
    "Watchdog reset",
    "Kremlins in the code",
    "Blood for the blood god, skulls for the skull throne."};

int main()
{
    stdio_init_all();
    eeprom_init_i2c(i2c0, 9600, 5);
    // lora_init(uart1, UART_TX_PIN, UART_RX_PIN);
    stepper_ctx step_ctx = stepper_get_ctx();
    // stepper_init(&step_ctx, pio0, );

    uint64_t startTime = time_us_64(); // Initialize start time.
    uint64_t actionTime;

    int randomNum;
    srand((unsigned int)time(NULL));
    uint8_t logArray[EEPROM_ARR_LENGTH];
    int arrayLen = 0;
    int logAddr = LOG_START_ADDR;
    uint32_t timestampSec = 0;

    printf("Creating Logs:\n");
    for (int i = 0; i < 20; i++)
    {
        randomNum = rand() % 5;
        actionTime = time_us_64();
        timestampSec = (uint32_t)((actionTime - startTime) / 1000000);

        printf("Log %d: %s, Timestamp: %d\n", i, rebootStatusCodes[randomNum], timestampSec);
        arrayLen = createLogArray(logArray, randomNum, timestampSec);

        printf("array: ");
        for (int i = 0; i < 6; i++)
        {
            printf("%d ", logArray[i]);
        }
        printf("\n");

        enterLogToEeprom(logArray, &arrayLen, logAddr);
        logAddr += EEPROM_ARR_LENGTH;
        sleep_ms(1000);
    }

    printf("\n\nReading Logs:\n");
    logAddr = LOG_START_ADDR;
    for (int i = 0; i < 20; i++)
    {
        eeprom_read_page(logAddr, logArray, EEPROM_ARR_LENGTH);

        printf("array: ");
        for (int i = 0; i < 6; i++)
        {
            printf("%d ", logArray[i]);
        }
        printf("\n");

        timestampSec = 0;
        timestampSec |= logArray[2] << 24;
        timestampSec |= logArray[3] << 16;
        timestampSec |= logArray[4] << 8;
        timestampSec |= logArray[5];

        printf("Log %d: %s, Timestamp %d\n", i, rebootStatusCodes[logArray[1]], timestampSec);
        logAddr += EEPROM_ARR_LENGTH;
    }
}
