
#include "../lib/eeprom.h"
#include "hardware/watchdog.h"
#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "../lib/logHandling.h"

#define LOG_LEN 6 // Does not include CRC
#define LOG_ARR_LEN LOG_LEN + 2 // Includes CRC


#define DISPENSER_STATE_LEN 4 // Does not include CRC
#define DISPENSER_STATE_ARR_LEN DISPENSER_STATE_LEN + 2 // Includes CRC
#define PILL_DISPENSE_STATE 0
#define REBOOT_STATUS_CODE 1
#define PREV_CALIB_STEP_COUNT_MSB 2
#define PREV_CALIB_STEP_COUNT_LSB 3
#define REBOOT_STATUS_ADDR 2112 // 2048 + 64

#define LOG_START_ADDR 0
#define LOG_END_ADDR 2048
#define LOG_SIZE 64
#define MAX_LOGS 32

const char *rebootStatusCodes[] = {
    "Boot",                                                   // 1
    "Button press",                                           // 2
    "Watchdog caused reboot.",                                // 3
    "Kremlins in the code",                                   // 4
    "Blood for the blood god, skulls for the skull throne."}; // 5

// TODO: UPDATE FUNCTION COMMENTS!!!!!!!!!!!!!!!!!!!!

uint16_t crc16(const uint8_t *data, size_t length)
{
    uint8_t x;
    uint16_t crc = 0xFFFF;

    while (length--)
    {
        x = crc >> 8 ^ *data++;
        x ^= x >> 4;
        crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x << 5)) ^ ((uint16_t)x);
    }

    printf("crc16(): Calculated CRC\n");
    printf("crc: %d\n", crc);
    return crc;
}

void appendCrcToBase8Array(uint8_t *base8Array, int *arrayLen)
{
    uint16_t crc = crc16(base8Array, *arrayLen); // Calculate CRC for the base8Array

    // Append the CRC as two bytes to the base8Array
    base8Array[*arrayLen] = crc >> 8;       // MSB
    base8Array[*arrayLen + 1] = crc & 0xFF; // LSB

    *arrayLen += 1; // Update the array length to reflect the addition of the CRC
}

int getChecksum(uint8_t *base8Array, int *arrayLen)
{
    printf("getChecksum(): Calculating checksum\n");
    printf("base8Array: ");
    for (int i = 0; i <= *arrayLen + 2; i++)
    {
        printf("%d ", base8Array[i]);
    }
    printf("\n");

    // Calculate and return the CRC as the checksum
    return crc16(base8Array, *arrayLen + 2);
}

// Uses CRC to verify the integrity of the given array.
bool verifyDataIntegrity(uint8_t *base8Array, int *arrayLen)
{
    printf("verifyDataIntegrity(): Verifying data integrity\n");

    // Check if the checksum for the array matches the expected value (0 for OK)
    if (getChecksum(base8Array, arrayLen) == 0)
    {
        return true; // Data integrity verified
    }
    else
    {
        return false; // Data integrity verification failed
    }
}

void reboot_sequence(struct DeviceStatus *ptrToStruct, const uint64_t bootTimestamp)
{
    // Read previous status from Eeprom
    if (readPillDispenserStatus(ptrToStruct) == false)
    {
        ptrToStruct->pillDispenseState = 0;
        ptrToStruct->rebootStatusCode = 0;
        ptrToStruct->prevCalibStepCount = 0;
    }

    // Find the first available log, emties all logs if all are full.
    ptrToStruct->unusedLogIndex = findFirstAvailableLog();

    // Write reboot cause to log if applicable.
    uint8_t logArray[LOG_ARR_LEN];
    int arrayLen = 0;
    if (watchdog_caused_reboot() == true) // watchdog caused reboot.
    {
        arrayLen = createLogArray(logArray, 3, getTimestampSinceBoot(bootTimestamp));
        pushLogToEeprom(ptrToStruct, 3, bootTimestamp);
    }

    // Write boot message upon completion of reboot sequence.
    arrayLen = createLogArray(logArray, 0, getTimestampSinceBoot(bootTimestamp));
    pushLogToEeprom(ptrToStruct, 0, bootTimestamp);
}

// Writes the given array to the given EEPROM address.
// Assumes the array is pre formatted, appends CRC.
bool enterLogToEeprom(uint8_t *base8Array, int *arrayLen, int logAddr)
{
    // Create new array and append CRC
    uint8_t crcAppendedArray[LOG_ARR_LEN];
    memcpy(crcAppendedArray, base8Array, *arrayLen);
    appendCrcToBase8Array(crcAppendedArray, arrayLen);

    printf("enterLogToEeprom(): Entering log to EEPROM\n");
    printf("array: ");
    for (int i = 0; i <= *arrayLen; i++)
    {
        printf("%d ", crcAppendedArray[i]);
    }
    printf("\n");

    printf("arrayLen: %d\n", *arrayLen);
    ;

    // Write the array to EEPROM
    eeprom_write_page(logAddr, crcAppendedArray, *arrayLen);
}

// sets the first byte of every log to 0 indicating that it is not in use.
void zeroAllLogs()
{
    // printf("Clearing all logs\n");
    int count = 0;
    uint16_t logAddr = 0;

    while (count <= MAX_LOGS)
    {
        eeprom_write_byte(logAddr, 0);
        logAddr += LOG_SIZE;
        count++;
    }
    // printf("Logs cleared\n");
}

// Fills a log array from the given message code and timestamp.
// Array must have a minimum len of 8.
// returns the length of the array
int createLogArray(uint8_t *array, int messageCode, uint32_t timestamp)
{
    array[0] = 1; // log in use.
    array[1] = messageCode;
    array[5] = (uint8_t)(timestamp & 0xFF); // LSB
    array[4] = (uint8_t)((timestamp >> 8) & 0xFF);
    array[3] = (uint8_t)((timestamp >> 16) & 0xFF);
    array[2] = (uint8_t)((timestamp >> 24) & 0xFF); // MSB
    return LOG_LEN;
}

// Fills a pill dispenser status log array from the given pill dispenser state, reboot status code, and previous calibration step count.
// Array must have a minimum len of 7.
// returns the length of the array
int createPillDispenserStatusLogArray(uint8_t *array, uint8_t pillDispenseState, uint8_t rebootStatusCode, uint16_t prevCalibStepCount)
{
    array[0] = pillDispenseState;
    array[1] = rebootStatusCode;
    array[2] = (uint8_t)(prevCalibStepCount & 0xFF);        // LSB
    array[3] = (uint8_t)((prevCalibStepCount >> 8) & 0xFF); // MSB
    return 4;
}

// updates status to Eeprom
void updatePillDispenserStatus(struct DeviceStatus *ptrToStruct)
{
    uint8_t array[LOG_ARR_LEN];
    int arrayLen = createPillDispenserStatusLogArray(array, ptrToStruct->pillDispenseState, ptrToStruct->rebootStatusCode, ptrToStruct->prevCalibStepCount);
    enterLogToEeprom(array, &arrayLen, REBOOT_STATUS_ADDR);
}

// reads previous status from Eeprom.
// returns false if eeprom CRC check fails, true otherwise.
bool readPillDispenserStatus(struct DeviceStatus *ptrToStruct)
{
    printf("readPillDispenserStatus(): Reading pill dispenser status from EEPROM\n");
    bool eepromReadSuccess = true;
    uint8_t valuesRead[LOG_ARR_LEN];

    // Read EEPROM values into the array.
    eeprom_read_page(REBOOT_STATUS_ADDR, valuesRead, DISPENSER_STATE_ARR_LEN); // TODO: address is hardcoded, rework later.

    printf("readPillDispenserStatus(): EEPROM values read\n");
    printf("valuesRead: ");
    for (int i = 0; i < DISPENSER_STATE_ARR_LEN; i++)
    {
        printf("%d ", valuesRead[i]);
    }
    printf("\n");

    // Verify data integrity.
    int len = DISPENSER_STATE_ARR_LEN;
    if (verifyDataIntegrity(valuesRead, &len) == true)
    {
        printf("readPillDispenserStatus(): Data integrity verified\n");
        // Extract and assign values from the array to the struct fields.
        ptrToStruct->pillDispenseState = valuesRead[PILL_DISPENSE_STATE];
        ptrToStruct->rebootStatusCode = valuesRead[REBOOT_STATUS_CODE];
        ptrToStruct->prevCalibStepCount = (uint16_t)valuesRead[PREV_CALIB_STEP_COUNT_MSB] << 8; // Extract MSB
        ptrToStruct->prevCalibStepCount |= (uint16_t)valuesRead[PREV_CALIB_STEP_COUNT_LSB];     // Extract LSB
    }
    else
    {
        printf("readPillDispenserStatus(): Failed to verify data integrity\n");
        eepromReadSuccess = false; // Data integrity verification failed
    }

    return eepromReadSuccess;
}

// Finds the first available log, empties all logs if all are full.
int findFirstAvailableLog()
{
    int count = 0;
    uint16_t logAddr = 0;

    // Find the first available log
    while (count <= MAX_LOGS)
    {
        if (eeprom_read_byte(logAddr) == 0)
        {
            return logAddr;
        }
        logAddr += LOG_SIZE;
        count++;
    }

    zeroAllLogs(); // Empry all logs
    return 0;
}

// Calculates and returns the timestamp since boot in seconds.
uint32_t getTimestampSinceBoot(const uint64_t bootTimestamp)
{
    return ((uint32_t)(time_us_64() - bootTimestamp) / 1000);
}

// Creates and pushes a log to EEPROM.
void pushLogToEeprom(struct DeviceStatus *pillDispenserStatusStruct, int messageCode, uint64_t bootTimestamp)
{
    uint8_t logArray[LOG_ARR_LEN];
    int arrayLen = createLogArray(logArray, messageCode, getTimestampSinceBoot(bootTimestamp));
    enterLogToEeprom(logArray, &arrayLen, (pillDispenserStatusStruct->unusedLogIndex * LOG_SIZE));
    updateUnusedLogIndex(pillDispenserStatusStruct);
}

// Updates unusedLogIndex to the next available log.
void updateUnusedLogIndex(struct DeviceStatus *pillDispenserStatusStruct)
{
    if (pillDispenserStatusStruct->unusedLogIndex < MAX_LOGS)
    {
        pillDispenserStatusStruct->unusedLogIndex++;
    }
    else
    {
        zeroAllLogs();
        pillDispenserStatusStruct->unusedLogIndex = 0;
    }
}