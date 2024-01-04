#include "hardware/uart.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "lora.h"

#define MAX_TRIES 5
#define BAUDRATE 9600
#define BUF_LEN 20  

static uart_inst_t *uart_instance;
static bool lora_available = false;

bool lora_init(uart_inst_t *uart, uint TX_pin, uint RX_pin) {
    gpio_set_function(TX_pin, GPIO_FUNC_UART);
    gpio_set_function(RX_pin, GPIO_FUNC_UART);

    uart_init(uart, BAUDRATE);
    uart_instance = uart;
    char buf[BUF_LEN];
    if (lora_wait()) {
        if (!lora_write("AT+MODE=LWOTAA\r\n")) return false;
        lora_read_uart(buf, BUF_LEN);
        printf("mode %s\n", buf);
        // "+KEY=APPKEY,\"1AEF109988E296E7D46DDB456C77B208\"\r\n"
        if (!lora_write("AT+KEY=APPKEY,\"1AEF109988E296E7D46DDB456C77B208\"\r\n")) return false;
        lora_read_uart(buf, BUF_LEN);
        printf("key %s\n", buf);
        if (!lora_write("AT+CLASS=A\r\n")) return false;
        lora_read_uart(buf, BUF_LEN);
        printf("class %s\n", buf);
        if (!lora_write("AT+PORT\r\n")) return false;
        lora_read_uart(buf, BUF_LEN);
        printf("port %s\n", buf);
        if (!lora_write("AT+JOIN\r\n")) return false;
        lora_read_uart(buf, BUF_LEN);
        printf("join %s\n", buf);
    }
    lora_available = true;
    return true;
}

void lora_message(const char *string) {
    if (!lora_available) return;
    size_t len = strlen(string)+11;
    char new_str[len];
    new_str[0] = '\0';
    strncat(new_str, "AT+MSG=\"", len);
    strncat(new_str, string, len);
    strncat(new_str, "\"\r\n", len);
    lora_write(new_str);
}

void lora_empty_buffer() {
    while (uart_is_readable_within_us(uart_instance, 5000)) uart_getc(uart_instance) ;
}

int lora_read_uart(char *dst, int size) {
    int read = 0;
    char c = 0;
    while (c != '\n' && read < size && uart_is_readable_within_us(uart1, 5000)) {
        c = uart_getc(uart1);
        dst[read] = c;
        read++;
    }
    // replace carriage return with zero
    read -= 2;
    dst[read] = '\0';
    printf("Response received.\n");
    return read;
}

bool lora_write(char *string) {
    lora_empty_buffer();
    size_t size = strlen(string);
    for (int i=0; i<MAX_TRIES; i++) {
        if (uart_is_writable(uart_instance)) uart_puts(uart_instance, string);
        if (uart_is_readable_within_us(uart_instance, 500000)) {
            return true;
        } else if (i != 5) {
            printf("No response. Retrying...\n");
        }
    }
    printf("Couldn't get a response.\n");
    return false;
}

int lora_wait() {
    printf("Waiting for response from lora...\n");
    if (lora_write("AT\r\n")) {
        lora_empty_buffer();
        return 1;
    }
    return 0;
}

int get_lora_firmware(char *dst, int size) {
    printf("Getting firmware version from lora module...\n");
    if (lora_write("AT+VER\r\n")) {
        return lora_read_uart(dst, size);
    }
    return 0;
}

int get_lora_deveui(char *dst, int size) {
    printf("Getting DevEui from lora module...\n");
    if (lora_write("AT+ID=DevEui\r\n")) {
        return lora_read_uart(dst, size);
    }
    return 0;
}

void parse_deveui(char *src, char *dst, int size) {
    if (strlen(src) > 13) src += 13;
    int i = 0;
    int next = 0;
    while (i < strlen(src) && next < size) {
        if (isalpha(src[i])) dst[next++] = tolower(src[i]);
        if (isdigit(src[i])) dst[next++] = src[i];
        i++;
    }
    dst[next] = '\0';
}