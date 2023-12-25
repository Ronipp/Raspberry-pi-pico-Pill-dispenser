
#include "../lib/eeprom.h"
#include "hardware/watchdog.h"
#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "../lib/magnusFuncs.h"

#define EEPROM_ARR_LENGTH 64
#define MIN_LOG_LEN 3
#define MAX_LOG_LEN 61

#define EEPROM_STATE_LEN 4
#define PILL_DISPENSE_STATE 0
#define REBOOT_STATUS_CODE 1
#define PREV_CALIB_STEP_COUNT_MSB 2
#define PREV_CALIB_STEP_COUNT_LSB 3

#define LOG_START_ADDR 0
#define LOG_END_ADDR 2048
#define LOG_SIZE 64
#define MAX_LOGS 32


// TODO: UPDATE FUNCTION COMMENTS!!!!!!!!!!!!!!!!!!!!

/**
 * Computes a 16-bit CRC for the given data.
 *
 * @param data   Pointer to the data array for which the CRC is calculated.
 * @param length The length of the data array.
 * @return       The computed 16-bit CRC for the data.
 */
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

/**
 * Appends a 16-bit CRC to base 8 array and updates the array length to match the new len.
 *
 * @param base8Array Pointer to the base 8 array to which the CRC will be appended.
 * @param arrayLen   Pointer to the length of the base 8 array.
 */
void appendCrcToBase8Array(uint8_t *base8Array, int *arrayLen)
{
    base8Array[*arrayLen] = 0;                   // Null-terminate the base8Array
    uint16_t crc = crc16(base8Array, *arrayLen); // Calculate CRC for the base8Array

    // Append the CRC as two bytes to the base8Array
    base8Array[*arrayLen + 1] = crc >> 8;   // MSB
    base8Array[*arrayLen + 2] = crc & 0xFF; // LSB

    *arrayLen += 2; // Update the array length to reflect the addition of the CRC
}

/**
 * Retrieves the checksum for a base 8 array to ensure data integrity.
 * Identifies the terminating zero, checks array length validity,
 * removes the zero, and computes the checksum based on the modified array content.
 *
 * @param base8Array Pointer to the base 8 array to compute the checksum for.
 * @param arrayLen   Pointer to the length of the base 8 array.
 *                    Updated if the terminating zero is found.
 * @return           The calculated checksum if array length is valid (0 = OK, else checksum fail), else returns -1 for errors.
 */
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

    // Calculate and return the CRC as the checksum
    return crc16(base8Array, zeroIndex + 2);
}

/**
 * Verifies the data integrity of an array by checking its checksum.
 *
 * @param valuesRead Pointer to the array whose data integrity needs to be verified.
 * @return           True if the checksum validation passes, otherwise false.
 */
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

/**
 * Retrieves data from EEPROM, verifies its integrity, and updates the provided struct with specific values.
 * Also retrieves data from the Watchdog scratch register and updates another struct with its values.
 *
 * @param ptrToEepromStruct    Pointer to the struct to update with EEPROM data.
 * @param ptrToWatchdogStruct  Pointer to the struct to update with Watchdog scratch register values.
 * @return                     Boolean indicating if the Eeprom data was verified or not.
 */
bool reboot_sequence(struct rebootValues *ptrToEepromStruct, struct rebootValues *ptrToWatchdogStruct)
{
    bool eepromReadSuccess = false;

    uint8_t valuesRead[EEPROM_ARR_LENGTH];
    // Read EEPROM values into the array.
    eeprom_read_page(64, valuesRead, EEPROM_ARR_LENGTH); // TODO: address is hardcoded, rework later.
    // Verify data integrity.
    int len = EEPROM_ARR_LENGTH;
    if (verifyDataIntegrity(valuesRead, &len, true) == true)
    {
        // Extract and assign values from the array to the struct fields.
        ptrToEepromStruct->pillDispenseState = valuesRead[PILL_DISPENSE_STATE];
        ptrToEepromStruct->rebootStatusCode = valuesRead[REBOOT_STATUS_CODE];
        ptrToEepromStruct->prevCalibStepCount = (uint16_t)valuesRead[PREV_CALIB_STEP_COUNT_MSB] << 8; // Extract MSB
        ptrToEepromStruct->prevCalibStepCount |= (uint16_t)valuesRead[PREV_CALIB_STEP_COUNT_LSB];     // Extract LSB
        eepromReadSuccess = true;                                                                     // Data integrity verified
    }
    // Read watchdog values into the struct from the scratch register.
    ptrToWatchdogStruct->pillDispenseState = watchdog_hw->scratch[0];
    ptrToWatchdogStruct->rebootStatusCode = watchdog_hw->scratch[1];
    ptrToWatchdogStruct->prevCalibStepCount = watchdog_hw->scratch[2];

    return eepromReadSuccess;
}

/**
 * Writes a log array to EEPROM after appending a CRC value at the end.
 *
 * @param array Pointer to the log array to be written to EEPROM.
 */
void enterLogToEeprom(uint8_t *base8Array, int *arrayLen, int logAddr)
{

    // Create new array and append CRC
    uint8_t crcAppendedArray[EEPROM_ARR_LENGTH];
    memcpy(crcAppendedArray, base8Array, *arrayLen);
    appendCrcToBase8Array(crcAppendedArray, arrayLen);
    // Write the array to EEPROM
    eeprom_write_page(logAddr, crcAppendedArray, *arrayLen);
}

void zeroAllLogs()
{
    int count = 0;
    uint16_t logAddr = 0;

    while (count <= MAX_LOGS)
    {
        eeprom_write_byte(logAddr, 0);
        logAddr += LOG_SIZE;
        count++;
    }
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
