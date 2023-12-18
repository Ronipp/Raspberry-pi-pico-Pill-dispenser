#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "debounce.h"
#include "stepper.h"
#include "lora.h"

#include "hardware/pio.h"




#define USER_STRING_SIZE 20

#define UART_TX_PIN 4
#define UART_RX_PIN 5


bool user_input(char *dst, size_t size) {
    if (uart_is_readable(uart0)) {
        char c = 0;
        int i = 0;
        while (c != '\n' && i < size) {
            c = fgetc(stdin);
            dst[i] = c;
            i++;
        }
        dst[--i] = '\0';
        return true;
    }
    return false;
}


int main() {
    stdio_init_all();
    lora_init(uart1, UART_TX_PIN, UART_RX_PIN);

    char buf[100];
    while (1) {
        lora_message("hello");
        lora_read_uart(buf, 100);
        printf("msg %s", buf);
    }
}
