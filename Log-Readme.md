# EEPROM Log Array Data Layout

### Byte 0: `logStatus`
- **Purpose**: Indicates the status of the log - whether it's available or in use.
- **Value Range**: 0 or 1
    - 0: Log is available for use
    - 1: Log is currently in use

### Byte 1: `messageCode`
- **Purpose**: Represents various messages logged by the system.
- **Value Range**: 0 to up to 27
    - 0: "Shutdown while motor was idle"
    - 1: "Watchdog caused reboot"
    - 2: "Dispensing pill 1"
    - 3: "Dispensing pill 2"
    - 4: "Dispensing pill 3"
    - 5: "Dispensing pill 4"
    - 6: "Dispensing pill 5"
    - 7: "Dispensing pill 6"
    - 8: "Dispensing pill 7"
    - 9: "Doing half calibration"
    - 10: "Doing full calibration"
    - 11: "Dispensing button pressed"
    - 12: "Pill dispensed"
    - 13: "Pill drop not detected"
    - 14: "Pill dispenser is empty"
    - 15: "Calibration finished"
    - 16: "Reboot during pill 1 dispensing"
    - 17: "Reboot during pill 2 dispensing"
    - 18: "Reboot during pill 3 dispensing"
    - 19: "Reboot during pill 4 dispensing"
    - 20: "Reboot during pill 5 dispensing"
    - 21: "Reboot during pill 6 dispensing"
    - 22: "Reboot during pill 7 dispensing"
    - 23: "Reboot during half calibration"
    - 24: "Reboot during full calibration"
    - 25: "Gremlins in the code"
    - 26: "Failed to read pill dispenser status from EEPROM"
    - 27: "Boot Finished"

### Bytes 2 to 5: `timestamp`
- **Purpose**: Stores a 32-bit timestamp value.
- **Value**: A 32-bit unsigned integer split across four bytes:
    - Byte 2 (MSB): Most Significant Byte (Higher bits)
    - Byte 5 (LSB): Least Significant Byte (Lower bits)

| Byte Index | Information       | Value Range                      |
|------------|-------------------|----------------------------------|
| 0          | logStatus         | 0 or 1                           |
| 1          | messageCode       | Value representing log messages  |
| 2          | Timestamp         | MSB of timestamp                 |
| 5          | Timestamp         | LSB of timestamp                 |
| Final 2    | Reserved CRC      |                                  |

---

# Pill Dispenser Status EEPROM Array

### Byte 0: `pillDispenseState`
 - **Purpose**: Indicates the current state of pill dispensing. 
 - **Value Range**: 0 to 7, represents how many pills have currently been dropped.

### Byte 1: `deviceStatusCode`
- **Purpose**: Describes the circumstances leading to device reboot.
- **Value Range**: 0 to 3
    - 0: Device was idle pre-boot
    - 1: Watchdog caused boot
    - 2: Dispenser was attempting to dispense pills
    - 3: Dispenser was in calibration

### Bytes 2 and 3: `prevCalibStepCount`
- **Purpose**: Stores the previous calibration step count.
- **Value**: A 16-bit unsigned integer split into two bytes:
    - Byte 2 (MSB): Most Significant Byte (Higher bits)
    - Byte 3 (LSB): Least Significant Byte (Lower bits)

### Bytes 4 and 5: `prevCalibEdgeCount`
- **Purpose**: Stores the previous calibration edge count.
- **Value**: A 16-bit unsigned integer split into two bytes:
    - Byte 4 (MSB): Most Significant Byte (Higher bits)
    - Byte 5 (LSB): Least Significant Byte (Lower bits)

| Byte Index | Information       | Value Range                      |
|------------|-------------------|----------------------------------|
| 0          | pillDispenseState | 0 to 7                           |
| 1          | deviceStatusCode  | 0 to 3                           |
| 2          | prevCalibStepCount| LSB of a uint16_t                |
| 3          | prevCalibStepCount| MSB of a uint16_t                |
| 4          | prevCalibEdgeCount| LSB of a uint16_t                |
| 5          | prevCalibEdgeCount| MSB of a uint16_t                |
| Final 2    | Reserved CRC      |                                  |
