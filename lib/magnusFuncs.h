#ifndef magnusFuncs_h
#define magnusFuncs_h

typedef struct rebootValues
{
    uint8_t pillDispenseState;
    uint8_t rebootStatusCode;
    uint16_t prevCalibStepCount;
} rebootValues;


uint16_t crc16(const uint8_t *data, size_t length);
void appendCrcToBase8Array(uint8_t *base8Array, int *arrayLen);
int getChecksum(uint8_t *base8Array, int *arrayLen, bool flagArrayLenAsTerminatingZero);
bool verifyDataIntegrity(uint8_t *base8Array, int *arrayLen, bool flagArrayLenAsTerminatingZero);
bool reboot_sequence(struct rebootValues *ptrToEepromStruct, struct rebootValues *ptrToWatchdogStruct);
void enterLogToEeprom(uint8_t *base8Array, int *arrayLen, int logAddr);
void zeroAllLogs();
void convertStringToBase8(const char *string, const int stringLen, uint8_t *base8String);
void createLogArray(uint8_t *array, int messageCode, uint32_t timestamp);
void createPillDispenserStatusLogArray(uint8_t *array, uint8_t pillDispenseState, uint8_t rebootStatusCode, uint16_t prevCalibStepCount);

#endif
