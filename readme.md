# Eeprom log array data layout
#### Byte 0: `pillDispenseState`

-   **Purpose**: Indicates the current state of pill dispensing.
-   **Value Range**: 0 to 7, representing different states of the pill dispensing mechanism.

#### Byte 1: `rebootStatusCode`

-   **Purpose**: Indicates the circumstances of the device reboot.
-   **Value**: 0 or 1
    -   `0`: The device wasn't performing critical operations when the reboot occurred.
    -   `1`: The device was executing essential code or was in the middle of a critical process when the reboot happened.

#### Byte 2 and Byte 3: `prevCalibStepCount`

-   **Purpose**: Represents the previous calibration step count.
-   **Value**: A 16-bit unsigned integer value split into two bytes:
    -   **Byte 2 (MSB)**: Most Significant Byte (Higher bits)
    -   **Byte 3 (LSB)**: Least Significant Byte (Lower bits)
    
    
| Byte Index | Information       | Value Range                      |
|------------|-------------------|----------------------------------|
| 0          | pillDispenseState | 0 to 7                           |
| 1          | rebootStatusCode  | valuecode for device status before reboot.|
| 2          | prevCalibStepCount| MSB of a uint16_t (16-bit value) |
| 3          | prevCalibStepCount| LSB of a uint16_t (16-bit value) |
| 4 - 61     | Undefined         |                                  |
| Final 3    | Reserved CRC      |                                  |
