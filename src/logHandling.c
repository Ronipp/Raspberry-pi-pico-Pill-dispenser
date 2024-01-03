
#include "eeprom.h"
#include "hardware/watchdog.h"
#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "logHandling.h"
#include "lora.h"

#define LOG_LEN 6                     // Does not include CRC
#define LOG_ARR_LEN LOG_LEN + CRC_LEN // Includes CRC
#define CRC_LEN 2

#define DISPENSER_STATE_LEN 4                                 // Does not include CRC
#define DISPENSER_STATE_ARR_LEN DISPENSER_STATE_LEN + CRC_LEN // Includes CRC
#define PILL_DISPENSE_STATE 0
#define REBOOT_STATUS_CODE 1
#define PREV_CALIB_STEP_COUNT_MSB 2
#define PREV_CALIB_STEP_COUNT_LSB 3
#define REBOOT_STATUS_ADDR LOG_END_ADDR + LOG_SIZE

#define LOG_START_ADDR 0
#define LOG_END_ADDR 2048
#define LOG_SIZE 8
#define MAX_LOGS LOG_END_ADDR / LOG_SIZE

const char *logMessages[] = {
    "Boot Finished",
    "Watchdog caused reboot",
    "Dispensing pill 1",
    "Dispensing pill 2",
    "Dispensing pill 3",
    "Dispensing pill 4",
    "Dispensing pill 5",
    "Dispensing pill 6",
    "Dispensing pill 7",
    "Doing half calibration",
    "Doing full calibration",
    "Button press",
    "pill dispensed",
    "pill drop not detected",
    "Pill dispenser is empty",
    "Calibration finished",
    "Reboot during pill 1 dispensing",
    "Reboot during pill 2 dispensing",
    "Reboot during pill 3 dispensing",
    "Reboot during pill 4 dispensing",
    "Reboot during pill 5 dispensing",
    "Reboot during pill 6 dispensing",
    "Reboot during pill 7 dispensing",
    "Reboot during half calibration",
    "Reboot during full calibration",
    "Gremlins in the code",
    "Failed to read pill dispenser status from EEPROM"};

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
 * Appends CRC to the given base 8 array and updates the array length.
 *
 * @param base8Array  Pointer to the base 8 array to which the CRC will be appended.
 * @param arrayLen    Pointer to the length of the base 8 array.
 */
void appendCrcToBase8Array(uint8_t *base8Array, int *arrayLen)
{
    uint16_t crc = crc16(base8Array, *arrayLen); // Calculate CRC for the base8Array

    // Append the CRC as two bytes to the base8Array
    base8Array[*arrayLen] = crc >> 8;       // MSB
    base8Array[*arrayLen + 1] = crc & 0xFF; // LSB

    *arrayLen += CRC_LEN; // Update the array length to reflect the addition of the CRC
}

/**
 * Computes the checksum (using CRC) for the provided base 8 array.
 *
 * @param base8Array  Pointer to the base 8 array to calculate the checksum for.
 * @param arrayLen    Pointer to the length of the base 8 array.
 * @return            The calculated checksum (CRC value) of the array.
 */
int getChecksum(uint8_t *base8Array, int *arrayLen)
{
    // Calculate and return the CRC as the checksum
    return crc16(base8Array, *arrayLen);
}

/**
 * Uses CRC to verify the integrity of the given array.
 *
 * @param base8Array  Pointer to the base 8 array to compute the checksum for.
 * @param arrayLen    Pointer to the length of the base 8 array. Updated if a terminating zero is found.
 * @return            True if integrity is verified, false otherwise.
 */
bool verifyDataIntegrity(uint8_t *base8Array, int *arrayLen)
{
    // Check if the checksum for the array matches the expected value.
    if (getChecksum(base8Array, arrayLen) == 0)
    {
        return true; // Data integrity verified
    }
    else
    {
        return false; // Data integrity verification failed
    }
}

/**
 * Manages the reboot sequence, including reading previous status from EEPROM, finding an available log,
 * writing reboot causes to logs, and recording a boot message upon sequence completion.
 *
 * @param ptrToStruct    Pointer to the DeviceStatus struct to be updated with reboot sequence details.
 * @param bootTimestamp  Boot timestamp for log recording purposes.
 */
void reboot_sequence(struct DeviceStatus *ptrToStruct, const uint64_t bootTimestamp)
{
    if (readPillDispenserStatus(ptrToStruct) == false)
    {
        // TODO: Create EEPROM log for failed status read.
        ptrToStruct->pillDispenseState = 0;
        ptrToStruct->rebootStatusCode = 0;
        ptrToStruct->prevCalibStepCount = 0;
        ptrToStruct->unusedLogIndex = 0; // TODO: change value.
        pushLogToEeprom(ptrToStruct, GREMLINS, bootTimestamp); //TODO: create function to combine these two functions.
        lora_message(logMessages[GREMLINS]);
    }

    // Find the first available log, empties all logs if all are full.
    ptrToStruct->unusedLogIndex = findFirstAvailableLog();

    // Write reboot cause to log if applicable.
    uint8_t logArray[LOG_ARR_LEN];
    if (watchdog_caused_reboot() == true) // If watchdog caused reboot
    {
        pushLogToEeprom(ptrToStruct, WATCHDOG_REBOOT, bootTimestamp); // Log the reboot cause
        lora_message(logMessages[WATCHDOG_REBOOT]);
    }
    else if (ptrToStruct->rebootStatusCode != 0) // If reboot status code is not 0
    {
        switch (ptrToStruct->rebootStatusCode)
        {
        case DISPENSE1:
            pushLogToEeprom(ptrToStruct, DISPENSE1_ERROR, bootTimestamp);
            lora_message(logMessages[DISPENSE1_ERROR]);
            break;
        case DISPENSE2:
            pushLogToEeprom(ptrToStruct, DISPENSE2_ERROR, bootTimestamp);
            lora_message(logMessages[DISPENSE2_ERROR]);
            break;
        case DISPENSE3:
            pushLogToEeprom(ptrToStruct, DISPENSE3_ERROR, bootTimestamp);
            lora_message(logMessages[DISPENSE3_ERROR]);
            break;
        case DISPENSE4:
            pushLogToEeprom(ptrToStruct, DISPENSE4_ERROR, bootTimestamp);
            lora_message(logMessages[DISPENSE4_ERROR]);
            break;
        case DISPENSE5:
            pushLogToEeprom(ptrToStruct, DISPENSE5_ERROR, bootTimestamp);
            lora_message(logMessages[DISPENSE5_ERROR]);
            break;
        case DISPENSE6:
            pushLogToEeprom(ptrToStruct, DISPENSE6_ERROR, bootTimestamp);
            lora_message(logMessages[DISPENSE6_ERROR]);
            break;
        case DISPENSE7:
            pushLogToEeprom(ptrToStruct, DISPENSE7_ERROR, bootTimestamp);
            lora_message(logMessages[DISPENSE7_ERROR]);
            break;
        case FULL_CALIBRATION:
            pushLogToEeprom(ptrToStruct, FULL_CALIBRATION_ERROR, bootTimestamp);
            lora_message(logMessages[FULL_CALIBRATION_ERROR]);
            break;
        case HALF_CALIBRATION:
            pushLogToEeprom(ptrToStruct, HALF_CALIBRATION_ERROR, bootTimestamp);
            lora_message(logMessages[HALF_CALIBRATION_ERROR]);
            break;
        default:
            pushLogToEeprom(ptrToStruct, GREMLINS, bootTimestamp);
            lora_message(logMessages[GREMLINS]);
            printf("There's gremlins in the code.\n");
            break;
        }
    }
}

/**
 * Writes the provided array (assumed to be pre-formatted with appended CRC) to the specified EEPROM address.
 * Assumes the array is ready for writing and includes appended CRC for data integrity.
 *
 * @param base8Array  Pointer to the pre-formatted base 8 array with appended CRC.
 * @param arrayLen    Pointer to the length of the array.
 * @param logAddr     Address in EEPROM where the array will be written.
 */
void enterLogToEeprom(uint8_t *base8Array, int *arrayLen, int logAddr)
{
    // Create a new array and append CRC
    uint8_t crcAppendedArray[*arrayLen + CRC_LEN];
    memcpy(crcAppendedArray, base8Array, *arrayLen);   // Copy the original array
    appendCrcToBase8Array(crcAppendedArray, arrayLen); // Append CRC to the copied array

    // Write the array with appended CRC to the specified EEPROM address
    eeprom_write_page(logAddr, crcAppendedArray, *arrayLen);
}

/**
 * Sets the first byte of every log in EEPROM to 0, indicating that it is not in use.
 */
void zeroAllLogs()
{
    int count = 0;
    uint16_t logAddr = 0;

    // Iterate through logs and set the first byte to 0 for each log
    while (count <= MAX_LOGS)
    {
        eeprom_write_byte(logAddr, 0); // Write 0 to indicate log not in use
        logAddr += LOG_SIZE;
        count++;
    }
}

/**
 * Fills a log array based on the given message code and timestamp.
 * The array must have a minimum length of 8.
 *
 * @param array        Pointer to the array to be filled with log information.
 * @param messageCode  Message code to store in the log array.
 * @param timestamp    Timestamp to store in the log array.
 * @return             The length of the filled log array.
 */
int createLogArray(uint8_t *array, int messageCode, uint32_t timestamp)
{
    array[0] = 1;           // Indicates log in use
    array[1] = messageCode; // Store the message code

    array[5] = (uint8_t)(timestamp & 0xFF); // LSB of timestamp
    array[4] = (uint8_t)((timestamp >> 8) & 0xFF);
    array[3] = (uint8_t)((timestamp >> 16) & 0xFF);
    array[2] = (uint8_t)((timestamp >> 24) & 0xFF); // MSB of timestamp

    return LOG_LEN; // Return the length of the filled log array (LOG_LEN)
}

/**
 * Fills a pill dispenser status log array based on the given pill dispenser state, reboot status code,
 * and previous calibration step count. The array must have a minimum length of 7.
 *
 * @param array                Pointer to the array to fill with the pill dispenser status log.
 * @param pillDispenseState    Pill dispenser state to store in the log array.
 * @param rebootStatusCode     Reboot status code to store in the log array.
 * @param prevCalibStepCount   Previous calibration step count to store in the log array.
 * @return                     The length of the filled log array (4).
 */
int createPillDispenserStatusLogArray(uint8_t *array, uint8_t pillDispenseState, uint8_t rebootStatusCode, uint16_t prevCalibStepCount)
{
    array[0] = pillDispenseState;
    array[1] = rebootStatusCode;
    array[2] = (uint8_t)(prevCalibStepCount & 0xFF);        // Store LSB of prevCalibStepCount
    array[3] = (uint8_t)((prevCalibStepCount >> 8) & 0xFF); // Store MSB of prevCalibStepCount
    return DISPENSER_STATE_LEN;
}

/**
 * Updates the pill dispenser status in EEPROM based on the provided struct.
 *
 * @param ptrToStruct Pointer to the struct containing the updated pill dispenser status.
 */
void updatePillDispenserStatus(DeviceStatus *ptrToStruct)
{
    uint8_t array[LOG_ARR_LEN]; // Buffer to hold the log array
    // Create a log array based on the provided status information
    int arrayLen = createPillDispenserStatusLogArray(array, ptrToStruct->pillDispenseState,
                                                     ptrToStruct->rebootStatusCode, ptrToStruct->prevCalibStepCount);
    // Write the log array to EEPROM at the designated address for pill dispenser status
    enterLogToEeprom(array, &arrayLen, REBOOT_STATUS_ADDR);
}

/**
 * Reads the previous pill dispenser status from EEPROM and updates the provided struct.
 * Performs an EEPROM CRC check and returns true if the check succeeds, false otherwise.
 *
 * @param ptrToStruct Pointer to the struct to update with the pill dispenser status.
 * @return Boolean indicating the success (true) or failure (false) of the EEPROM read operation.
 */
bool readPillDispenserStatus(DeviceStatus *ptrToStruct)
{
    bool eepromReadSuccess = true;   // Initialize EEPROM read status as successful
    uint8_t valuesRead[LOG_ARR_LEN]; // Buffer to hold EEPROM values

    // Read EEPROM values into the array.
    eeprom_read_page(REBOOT_STATUS_ADDR, valuesRead, DISPENSER_STATE_ARR_LEN);

    // Verify data integrity.
    int len = DISPENSER_STATE_ARR_LEN;
    if (verifyDataIntegrity(valuesRead, &len) == true)
    {
        // Data integrity verified, extract and assign values to struct fields.
        ptrToStruct->pillDispenseState = valuesRead[PILL_DISPENSE_STATE];
        ptrToStruct->rebootStatusCode = valuesRead[REBOOT_STATUS_CODE];
        ptrToStruct->prevCalibStepCount = (uint16_t)valuesRead[PREV_CALIB_STEP_COUNT_MSB] << 8; // Extract MSB
        ptrToStruct->prevCalibStepCount |= (uint16_t)valuesRead[PREV_CALIB_STEP_COUNT_LSB];     // Extract LSB
    }
    else
    {
        eepromReadSuccess = false; // Data integrity verification failed
    }

    return eepromReadSuccess; // Return the success/failure status of the EEPROM read operation
}

/**
 * Finds the first available log entry in EEPROM.
 * If an available log entry is found, returns its index.
 * If all log entries are full, empties all logs and returns the index of the first log.
 *
 * @return The index of the first available log entry if found, else returns 0 after clearing all logs.
 */
int findFirstAvailableLog()
{
    int count = 0;        // Initialize counter
    uint16_t logAddr = 0; // Initialize EEPROM address

    // Find the first available log entry
    while (count <= MAX_LOGS)
    {
        if (eeprom_read_byte(logAddr) == 0)
        {
            return count; // Return index if an available log entry is found
        }
        logAddr += LOG_SIZE; // Move to the next log entry
        count++;
    }

    zeroAllLogs(); // Clear all logs if all entries are full
    return 0;      // Return 0 after clearing logs
}

/**
 * Calculates the elapsed time in seconds since the system boot using a boot timestamp.
 *
 * @param bootTimestamp The timestamp representing the system boot time.
 * @return The elapsed time in seconds since the system boot based on the provided boot timestamp.
 */
uint32_t getTimestampSinceBoot(const uint64_t bootTimestamp)
{
    // Calculate the elapsed time in microseconds since boot, then convert it to seconds
    return (uint32_t)((time_us_64() - bootTimestamp) / 1000000);
}

/**
 * Writes a log entry to EEPROM using information from the device status structure,
 *
 * @param pillDispenserStatusStruct Pointer to the device status structure.
 * @param messageCode               Message code indicating the type of log entry.
 * @param bootTimestamp             Boot timestamp representing the time when the log was created.
 */
void pushLogToEeprom(DeviceStatus *pillDispenserStatusStruct, log_number messageCode, uint32_t time_ms)
{
    uint8_t logArray[LOG_LEN]; // Buffer to hold log data
    // Create a log array with the provided message code and boot timestamp
    int arrayLen = createLogArray(logArray, messageCode, time_ms);

    // Write the log array to EEPROM at the appropriate index based on the log size and unused log index
    enterLogToEeprom(logArray, &arrayLen, (pillDispenserStatusStruct->unusedLogIndex * LOG_SIZE));

    // Update the unused log index in the device status structure
    updateUnusedLogIndex(pillDispenserStatusStruct);
}

/**
 * Updates the unused log index in the device status structure.
 *
 * @param pillDispenserStatusStruct Pointer to the device status structure containing log-related information.
 */
void updateUnusedLogIndex(struct DeviceStatus *pillDispenserStatusStruct)
{
    if (pillDispenserStatusStruct->unusedLogIndex < MAX_LOGS)
    {
        pillDispenserStatusStruct->unusedLogIndex++;
    }
    else
    {
        zeroAllLogs();                                 // Reset all logs when the unused log index reaches the maximum limit
        pillDispenserStatusStruct->unusedLogIndex = 0; // Reset unused log index to 0
    }
}

/**
 * Prints valid logs stored in EEPROM by reading and interpreting log data.
 */
void printValidLogs()
{
    for (int i = 0; i <= MAX_LOGS; i++)
    {
        uint16_t logAddr = i * LOG_SIZE; // Calculate the EEPROM address for the log entry
        uint8_t logData[LOG_LEN];        // Buffer to hold log data

        eeprom_read_page(logAddr, logData, LOG_LEN); // Read log data from EEPROM

        if (logData[0] == 0)
        {                                     // Check if the log entry is valid (non-zero message code)
            uint8_t messageCode = logData[1]; // Extract the message code
            uint32_t timestamp = (logData[2] << 24) | (logData[3] << 16) | (logData[4] << 8) | logData[5];
            uint16_t timestamp_s = timestamp / 1000;
            // Construct timestamp from individual bytes

            printf("%d: %s %u seconds after last boot.\n", i, logMessages[messageCode], timestamp_s);
            // Print the log message corresponding to the message code and the timestamp
        }
    }
}

/**
 * Checks if a given value exists within an array.
 *
 * @param value The value to search for within the array.
 * @param array Pointer to the array to be searched.
 * @param size The size of the array.
 * @return true if the value is found in the array, otherwise false.
 */
bool isValueInArray(int value, int *array, int size)
{
    for (int i = 0; i < size; i++)
    {
        if (array[i] == value)
        {
            return true; // Value found in the array
        }
    }
    return false; // Value not found in the array
}


void devicestatus_change_reboot_num(DeviceStatus *dev, log_number num) {
    if (dev->rebootStatusCode == num) return;
    dev->rebootStatusCode = num;
    dev->changed = true;
    
}

void devicestatus_change_dispense_state(DeviceStatus *dev, uint8_t num) {
    if (dev->pillDispenseState == num) return;
    dev->pillDispenseState = num;
    dev->changed = true;
}

void devicestatus_change_steps(DeviceStatus *dev, uint16_t max_steps, uint16_t edge_steps) {
    if ((dev->prevCalibStepCount == max_steps) && (dev->prevCalibEdgeCount == edge_steps)) return;
    dev->prevCalibStepCount = max_steps;
    dev->prevCalibEdgeCount = edge_steps;
    dev->changed = true;
}

void logger_device_status(DeviceStatus *dev) {
    if (!(dev->changed)) return;
    updatePillDispenserStatus(dev);
}
