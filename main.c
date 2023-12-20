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
        timestampSec = (to_ms_since_boot(get_absolute_time()) / 1000);

        printf("Log %d: %s, Timestamp: %lld\n", i, rebootStatusCodes[randomNum], timestampSec);
        arrayLen = createLogArray(logArray, randomNum, timestampSec);
        enterLogToEeprom(logArray, &arrayLen, logAddr);
        logAddr += EEPROM_ARR_LENGTH;
        sleep_ms(1000);
    }

    printf("\n\nReading Logs:\n");
    logAddr = LOG_START_ADDR;
    for (int i = 0; i < 20; i++)
    {
        eeprom_read_page(logAddr, logArray, EEPROM_ARR_LENGTH);

        timestampSec |= (uint32_t)logArray[2] << 24;
        timestampSec |= (uint32_t)logArray[3] << 16;
        timestampSec |= (uint32_t)logArray[4] << 8;
        timestampSec |= (uint32_t)logArray[5];

        printf("Log %d: %s, Timestamp %lld\n", i, rebootStatusCodes[logArray[1]], timestampSec);
        logAddr += EEPROM_ARR_LENGTH;
    }
}
