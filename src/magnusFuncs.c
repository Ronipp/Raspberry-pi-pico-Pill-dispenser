
#include "C:\Users\Mage\Documents\GitHub\embedded_project\lib\eeprom.h"
#include "watchdog.h"
#include "stdint.h"
#include "stdbool.h"

#define EEPROM_ARR_LENGTH 64
#define MIN_LOG_LEN 3
#define MAX_LOG_LEN 61
#define PILL_DISPENSE_STATE 0
#define REBOOT_STATUS_CODE 1
#define PREV_CALIB_STEP_COUNT_MSB 2
#define PREV_CALIB_STEP_COUNT_LSB 3

struct rebootValues
{
    uint8_t pillDispenseState;
    uint8_t rebootStatusCode;
    uint16_t prevCalibStepCount;
};

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
 * Appends a 16-bit CRC to base 8 array and updates the array length.
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

    *arrayLen += 3; // Update the array length to reflect the addition of the CRC
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
int getChecksum(uint8_t *base8Array, int *arrayLen)
{
    int zeroIndex = 0;

    // Find the terminating zero
    for (int i = 0; i < *arrayLen; i++)
    {
        if (base8Array[i] == 0)
        {
            zeroIndex = i;
            break;
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
    *arrayLen = zeroIndex + 2;

    // Calculate and return the CRC as the checksum
    return crc16(base8Array, *arrayLen);
}

/**
 * Verifies the data integrity of an array by checking its checksum.
 *
 * @param valuesRead Pointer to the array whose data integrity needs to be verified.
 * @return           True if the checksum validation passes, otherwise false.
 */
bool verifyDataIntegrity(uint8_t *valuesRead)
{
    // Check if the checksum for the array matches the expected value (0 for OK)
    if (getChecksum(valuesRead, EEPROM_ARR_LENGTH) == 0)
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
    eeprom_read_page(64, valuesRead, EEPROM_ARR_LENGTH);

    // Verify data integrity.
    if (verifyDataIntegrity(valuesRead) == 0)
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
