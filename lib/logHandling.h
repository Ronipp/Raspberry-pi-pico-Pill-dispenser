#ifndef logHandling_h
#define logHandling_h

extern const char *logMessages[];
extern const char *pillDispenserStatus[];

typedef enum {
    BOOTFINISHED,
    WATCHDOG_REBOOT,    
    DISPENSE1,
    DISPENSE2,
    DISPENSE3,
    DISPENSE4,
    DISPENSE5,
    DISPENSE6,
    DISPENSE7,
    HALF_CALIBRATION,
    FULL_CALIBRATION,    
    BUTTON_PRESS,
    PILL_DISPENSED,
    PILL_ERROR,
    DISPENSER_EMPTY,
    CALIBRATION_FINISHED,
    DISPENSE1_ERROR,
    DISPENSE2_ERROR,
    DISPENSE3_ERROR,
    DISPENSE4_ERROR,
    DISPENSE5_ERROR,
    DISPENSE6_ERROR,
    DISPENSE7_ERROR,
    HALF_CALIBRATION_ERROR,
    FULL_CALIBRATION_ERROR,
    GREMLINS,
    DISPENSER_STATUS_READ_ERROR
} log_number;

typedef struct DeviceStatus
{
    uint8_t pillDispenseState;
    log_number rebootStatusCode;
    uint16_t prevCalibStepCount;
    uint16_t prevCalibEdgeCount;

    int unusedLogIndex; // index of log the program will use.
} DeviceStatus;

uint16_t crc16(const uint8_t *data, size_t length);
void appendCrcToBase8Array(uint8_t *base8Array, int *arrayLen);
int getChecksum(uint8_t *base8Array, int *arrayLen);
bool verifyDataIntegrity(uint8_t *base8Array, int *arrayLen);
void reboot_sequence(struct DeviceStatus *ptrToStruct, const uint64_t bootTimestamp);
void enterLogToEeprom(uint8_t *base8Array, int *arrayLen, int logAddr);
void zeroAllLogs();
int createLogArray(uint8_t *array, int messageCode, uint32_t timestamp);
int createPillDispenserStatusLogArray(uint8_t *array, uint8_t pillDispenseState, uint8_t rebootStatusCode, uint16_t prevCalibStepCount);
void updatePillDispenserStatus(struct DeviceStatus *ptrToStruct);
bool readPillDispenserStatus(struct DeviceStatus *ptrToStruct);
int findFirstAvailableLog();
uint32_t getTimestampSinceBoot(const uint64_t bootTimestamp);
void pushLogToEeprom(DeviceStatus *pillDispenserStatusStruct, log_number messageCode, uint32_t time_ms);
void updateUnusedLogIndex(struct DeviceStatus *pillDispenserStatusStruct);
void printValidLogs();
bool isValueInArray(int value, int *array, int size);

// wrappers
void devicestatus_change_reboot_num(DeviceStatus *dev, log_number num);
void devicestatus_change_dispense_state(DeviceStatus *dev, uint8_t num);
void devicestatus_change_steps(DeviceStatus *dev, uint16_t max_steps, uint16_t edge_steps);

#endif
