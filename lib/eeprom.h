#ifndef EEPROM_H
#define EEPROM_H

void eeprom_init_i2c(i2c_inst_t *i2c, uint baud, uint32_t write_cycle_max_ms);
void eeprom_write_byte(uint16_t address, char c);
char eeprom_read_byte(uint16_t address);
void eeprom_write_page(uint16_t address, uint8_t *src, size_t size);
void eeprom_read_page(uint16_t address, uint8_t *dst, size_t size);

#endif