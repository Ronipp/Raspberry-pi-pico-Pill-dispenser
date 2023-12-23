
# EEPROM Log Array Data Layout

### Byte 0: `logStatus`
- **Purpose**: Indicates the status of the log - whether it's available or in use.
- **Value Range**: 0 or 1
    - 0: Log is available for use
    - 1: Log is currently in use

### Byte 1: `messageCode`
- **Purpose**: Represents various messages logged by the system.
- **Value Range**: 0 to 17 (up to 255)
    - 0: "Boot Finished"
    - 1: "Button press"
    - 2: "Watchdog caused reboot"
    - 3: "Idle"
    - 4: "Dispensing pill 1"
    - 5: "Dispensing pill 2"
    - 6: "Dispensing pill 3"
    - 7: "Dispensing pill 4"
    - 8: "Dispensing pill 5"
    - 9: "Dispensing pill 6"
    - 10: "Dispensing pill 7"
    - 11: "Pill dispensed"
    - 12: "Pill drop not detected"
    - 13: "Rotating to home position"
    - 14: "Pill dispenser is empty"
    - 15: "Doing half calibration"
    - 16: "Doing full calibration"
    - 17: "Calibration finished"

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
| 6 - 61     | Undefined         |                                  |
| Final 2    | Reserved CRC      |                                  |

---

# Pill Dispenser Status EEPROM Array


### Byte 0: `pillDispenseState`
 - **Purpose**: Indicates the current state of pill dispensing. 
 - **Value Range**: 0 to 7, represents how many pills have currently been dropped.

### Byte 1: `deviceStatusCode`
- **Purpose**: Describes the circumstances leading to device reboot.
- **Value Range**: 0 to 12 (up to 255)
    - 0: Device was idle pre-boot
    - 1: Watchdog caused boot
    - 2: Dispenser was attempting to dispense pill 1
    - 3: Dispenser was attempting to dispense pill 2
    - 4: Dispenser was attempting to dispense pill 3
    - 5: Dispenser was attempting to dispense pill 4
    - 6: Dispenser was attempting to dispense pill 5
    - 7: Dispenser was attempting to dispense pill 6
    - 8: Dispenser was attempting to dispense pill 7
    - 9: Dispenser was rotating to home position
    - 10: Dispenser was doing calibration
    - 11: Dispenser was doing half calibration

### Bytes 2 and 3: `prevCalibStepCount`
- **Purpose**: Stores the previous calibration step count.
- **Value**: A 16-bit unsigned integer split into two bytes:
    - Byte 2 (MSB): Most Significant Byte (Higher bits)
    - Byte 3 (LSB): Least Significant Byte (Lower bits)

| Byte Index | Information       | Value Range                      |
|------------|-------------------|----------------------------------|
| 0          | pillDispenseState | 0 to 7                           |
| 1          | rebootStatusCode  | Value representing device status before reboot|
| 2          | prevCalibStepCount| MSB of a uint16_t  |
| 3          | prevCalibStepCount| LSB of a uint16_t  |
| 4 - 61     | Undefined         |                                  |
| Final 2    | Reserved CRC      |                                  |
