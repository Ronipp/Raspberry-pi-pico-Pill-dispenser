#pragma once

bool lora_init(uart_inst_t *uart, uint TX_pin, uint RX_pin);
bool lora_write(char *string);
int lora_wait();
int lora_read_uart(char *dst, int size);
void lora_message(char *string);