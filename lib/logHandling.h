#ifndef logHandling_h
#define logHandling_h

extern const char *logMessages[];
extern const char *pillDispenserStatus[];

typedef enum {
    IDLE,
    DISPENSING,
    FULL_CALIBRATION,
    HALF_CALIBRATION
} reboot_num;

typedef enum {
    LOG_IDLE,
    LOG_WATCHDOG_REBOOT,
    LOG_DISPENSE1,
    LOG_DISPENSE2,
    LOG_DISPENSE3,
    LOG_DISPENSE4,
    LOG_DISPENSE5,
    LOG_DISPENSE6,
    LOG_DISPENSE7,
    LOG_HALF_CALIBRATION,
    LOG_FULL_CALIBRATION,    
    LOG_BUTTON_PRESS,
    LOG_PILL_DISPENSED,
    LOG_PILL_ERROR,
    LOG_DISPENSER_EMPTY,
    LOG_CALIBRATION_FINISHED,
    LOG_DISPENSE1_ERROR,
    LOG_DISPENSE2_ERROR,
    LOG_DISPENSE3_ERROR,
    LOG_DISPENSE4_ERROR,
    LOG_DISPENSE5_ERROR,
    LOG_DISPENSE6_ERROR,
    LOG_DISPENSE7_ERROR,
    LOG_HALF_CALIBRATION_ERROR,
    LOG_FULL_CALIBRATION_ERROR,
    LOG_GREMLINS,
    LOG_DISPENSER_STATUS_READ_ERROR,
    LOG_BOOTFINISHED
} log_number;

typedef struct DeviceStatus
{
    uint8_t pillDispenseState;
    reboot_num rebootStatusCode;
    uint16_t prevCalibStepCount;
    uint16_t prevCalibEdgeCount;

    int unusedLogIndex; // index of log the program will use.
} DeviceStatus;

uint16_t crc16(const uint8_t *data, size_t length);
void appendCrcToBase8Array(uint8_t *base8Array, int *arrayLen);
int getChecksum(uint8_t *base8Array, int *arrayLen);
bool verifyDataIntegrity(uint8_t *base8Array, int *arrayLen);
void reboot_sequence(struct DeviceStatus *ptrToStruct, const uint32_t bootTimestamp);
void enterLogToEeprom(uint8_t *base8Array, int *arrayLen, int logAddr);
void zeroAllLogs();
int createLogArray(uint8_t *array, int messageCode, uint32_t timestamp);
int createPillDispenserStatusLogArray(uint8_t *array, uint8_t pillDispenseState, uint8_t rebootStatusCode, uint16_t prevCalibStepCount, uint16_t calibEdgeCount);
void updatePillDispenserStatus(struct DeviceStatus *ptrToStruct);
bool readPillDispenserStatus(struct DeviceStatus *ptrToStruct);
int findFirstAvailableLog();
uint32_t getTimestampSinceBoot(const uint64_t bootTimestamp);
void pushLogToEeprom(DeviceStatus *pillDispenserStatusStruct, log_number messageCode, uint32_t time_ms);
void updateUnusedLogIndex(struct DeviceStatus *pillDispenserStatusStruct);
void printValidLogs();
bool isValueInArray(int value, int *array, int size);

void devicestatus_change_reboot_num(DeviceStatus *dev, reboot_num num);
void devicestatus_change_dispense_state(DeviceStatus *dev, uint8_t num);
void devicestatus_change_steps(DeviceStatus *dev, uint16_t max_steps, uint16_t edge_steps);
void logger_device_status(DeviceStatus *dev);

#endif
