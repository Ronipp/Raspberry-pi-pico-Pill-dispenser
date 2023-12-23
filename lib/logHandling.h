#ifndef logHandling_h
#define logHandling_h

extern const char *rebootStatusCodes[];

typedef struct DeviceStatus
{
    uint8_t pillDispenseState;
    uint8_t rebootStatusCode;
    uint16_t prevCalibStepCount;

    int unusedLogIndex; // index of log the program will use.
} DeviceStatus;

uint16_t crc16(const uint8_t *data, size_t length);
void appendCrcToBase8Array(uint8_t *base8Array, int *arrayLen);
int getChecksum(uint8_t *base8Array, int *arrayLen, bool flagArrayLenAsTerminatingZero);
bool verifyDataIntegrity(uint8_t *base8Array, int *arrayLen, bool flagArrayLenAsTerminatingZero);
void reboot_sequence(struct DeviceStatus *ptrToStruct, const bool readWatchdogStatus, const uint64_t bootTimestamp);
bool enterLogToEeprom(uint8_t *base8Array, int *arrayLen, int logAddr);
void zeroAllLogs();
int createLogArray(uint8_t *array, int messageCode, uint32_t timestamp);
int createPillDispenserStatusLogArray(uint8_t *array, uint8_t pillDispenseState, uint8_t rebootStatusCode, uint16_t prevCalibStepCount);
void updatePillDispenserStatus(struct DeviceStatus *ptrToStruct);
bool readPillDispenserStatus(struct DeviceStatus *ptrToStruct, const bool readWatchdogStatus);
int findFirstAvailableLog();
uint32_t getTimestampSinceBoot(const uint64_t bootTimestamp);
void pushLogToEeprom(struct DeviceStatus *pillDispenserStatusStruct, int messageCode, uint64_t bootTimestamp);
void updateUnusedLogIndex(struct DeviceStatus *pillDispenserStatusStruct);

#endif
