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
int getChecksum(uint8_t *base8Array, int *arrayLen);
bool verifyDataIntegrity(uint8_t *valuesRead);
bool reboot_sequence(struct rebootValues *ptrToEepromStruct, struct rebootValues *ptrToWatchdogStruct);

#endif
