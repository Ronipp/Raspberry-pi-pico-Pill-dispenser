
#include "../lib/eeprom.h"
#include "hardware/watchdog.h"
#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "../lib/logHandling.h"

#define EEPROM_ARR_LENGTH 64
#define MIN_LOG_LEN 3
#define MAX_LOG_LEN 61

#define EEPROM_STATE_LEN 4
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

int getChecksum(uint8_t *base8Array, int *arrayLen, bool flagArrayLenAsTerminatingZero)
{
    int zeroIndex = 0;

    if (flagArrayLenAsTerminatingZero)
    {
        zeroIndex = *arrayLen;
    }
    else
    {
        // Find the terminating zero
        for (int i = 0; i < *arrayLen; i++)
        {
            if (base8Array[i] == 0)
            {
                zeroIndex = i;
                break;
            }
        }
    }

    // Validate array length
    if (zeroIndex < MIN_LOG_LEN || zeroIndex > MAX_LOG_LEN)
    {
        return -1; // Array too long or too short to be valid
    }

    // Modify the array by removing the terminating zero and adjust the length
    base8Array[zeroIndex] = base8Array[zeroIndex + 1];
    base8Array[zeroIndex + 1] = base8Array[zeroIndex + 2];

    /*
    printf("base8Array: ");
    for (int i = 0; i <= zeroIndex + 2; i++)
    {
        printf("%d ", base8Array[i]);
    }
    printf("\n");
    */

    // Calculate and return the CRC as the checksum
    return crc16(base8Array, zeroIndex + 2);
}

// Uses CRC to verify the integrity of the given array.
bool verifyDataIntegrity(uint8_t *base8Array, int *arrayLen, bool flagArrayLenAsTerminatingZero)
{
    int arrlen = EEPROM_ARR_LENGTH;
    // Check if the checksum for the array matches the expected value (0 for OK)
    if (getChecksum(base8Array, arrayLen, flagArrayLenAsTerminatingZero) == 0)
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
        printf("reboot_sequence(): Failed to read pill dispenser status\n");
        ptrToStruct->pillDispenseState = 0;
        ptrToStruct->rebootStatusCode = 0;
        ptrToStruct->prevCalibStepCount = 0;
    }

    // Find the first available log, emties all logs if all are full.
    ptrToStruct->unusedLogIndex = findFirstAvailableLog();
    printf("Unused log index: %d\n", ptrToStruct->unusedLogIndex);

    // Write reboot cause to log if applicable.
    uint8_t logArray[EEPROM_ARR_LENGTH];
    int arrayLen = createLogArray(logArray, 3, getTimestampSinceBoot(bootTimestamp));
    if (watchdog_caused_reboot() == true) // watchdog caused reboot.
    {
        enterLogToEeprom(logArray, &arrayLen, (ptrToStruct->unusedLogIndex * LOG_SIZE));
    }

    // Write boot message upon completion of reboot sequence.
    arrayLen = createLogArray(logArray, 0, getTimestampSinceBoot(bootTimestamp));
    enterLogToEeprom(logArray, &arrayLen, (ptrToStruct->unusedLogIndex * LOG_SIZE));
}

// Writes the given array to the given EEPROM address.
// Assumes the array is pre formatted, appends CRC.
bool enterLogToEeprom(uint8_t *base8Array, int *arrayLen, int logAddr)
{
    // Create new array and append CRC
    uint8_t crcAppendedArray[EEPROM_ARR_LENGTH];
    memcpy(crcAppendedArray, base8Array, *arrayLen);
    appendCrcToBase8Array(crcAppendedArray, arrayLen);

    // Validate array length
    if (*arrayLen < MIN_LOG_LEN || *arrayLen > MAX_LOG_LEN)
    {
        printf("enterLogToEeprom(): Arraylen is invalid\n");
        return false; // Array too long or too short to be valid
    }

    printf("enterLogToEeprom(): Entering log to EEPROM\n");
    printf("array: ");
    for (int i = 0; i <= *arrayLen; i++)
    {
        printf("%d ", crcAppendedArray[i]);
    }
    printf("\n");

    printf("arrayLen: %d\n", *arrayLen);;


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
    return 6;
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
    uint8_t array[EEPROM_ARR_LENGTH];
    int arrayLen = createPillDispenserStatusLogArray(array, ptrToStruct->pillDispenseState, ptrToStruct->rebootStatusCode, ptrToStruct->prevCalibStepCount);
    enterLogToEeprom(array, &arrayLen, REBOOT_STATUS_ADDR);
}

// reads previous status from Eeprom.
// returns false if eeprom CRC check fails, true otherwise.
bool readPillDispenserStatus(struct DeviceStatus *ptrToStruct)
{
    printf("readPillDispenserStatus(): Reading pill dispenser status\n");
    bool eepromReadSuccess = true;
    uint8_t valuesRead[EEPROM_ARR_LENGTH];

    // Read EEPROM values into the array.
    eeprom_read_page(REBOOT_STATUS_ADDR, valuesRead, EEPROM_ARR_LENGTH); // TODO: address is hardcoded, rework later.
    printf("readPillDispenserStatus(): EEPROM values read\n");

    // Verify data integrity.
    int len = EEPROM_ARR_LENGTH;
    if (verifyDataIntegrity(valuesRead, &len, true) == true)
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

    printf("readPillDispenserStatus(): Pill dispenser status read\n");
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
    uint8_t logArray[EEPROM_ARR_LENGTH];
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