#include "hardware/uart.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "lora.h"

#define MAX_TRIES 5
#define BAUDRATE 9600
#define BUF_LEN 50

static uart_inst_t *uart_instance;
static bool lora_available = false;

/**
 * Initializes LoRa communication on the specified UART with TX and RX pins.
 * Configures the LoRa module by sending a series of AT commands for setup.
 *
 * @param uart     UART instance for LoRa communication.
 * @param TX_pin   TX pin for UART communication with the LoRa module.
 * @param RX_pin   RX pin for UART communication with the LoRa module.
 * @return         true if LoRa initialization is successful, false otherwise.
 */
bool lora_init(uart_inst_t *uart, uint TX_pin, uint RX_pin) {
    // Configure pins for UART communication
    gpio_set_function(TX_pin, GPIO_FUNC_UART);
    gpio_set_function(RX_pin, GPIO_FUNC_UART);

    uart_init(uart, BAUDRATE);
    uart_instance = uart;

    char buf[BUF_LEN]; // Buffer for LoRa responses

    // Initialization sequence for LoRa module
    if (lora_wait()) {
        if (!lora_write("AT+MODE=LWOTAA\r\n")) return false;
        lora_read_uart(buf, BUF_LEN);
        printf("mode %s\n", buf);

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
    return true; // LoRa initialization successful
}

/**
 * Sends a message string via LoRa communication.
 * Constructs and sends an AT command with the provided string as a message payload.
 *
 * @param string The message string to be transmitted via LoRa.
 */
bool lora_message(const char *string) {
    if (!lora_available) return false; // Check if LoRa communication is available

    size_t len = strlen(string) + 11; // Calculate length for the AT command
    char new_str[len]; // Buffer to construct the AT command

    new_str[0] = '\0'; // Initialize the new string buffer
    strncat(new_str, "AT+MSG=\"", len); // Append AT command header
    strncat(new_str, string, len); // Append the message string
    strncat(new_str, "\"\r\n", len); // Append AT command termination

    lora_write(new_str); // Send the constructed AT command for message transmission
    char buf[BUF_LEN];
    lora_read_uart(buf, BUF_LEN);
    printf("%s\n", buf);
    if (strcmp(buf, "+MSG: Start") == 0) {
        return true;
    }
    return false;
}

/**
 * Empties the UART receive buffer for the LoRa communication.
 * Reads and discards any available data in the UART receive buffer.
 * Useful to clear out any residual data before initiating new LoRa operations.
 */
void lora_empty_buffer() {
    // Continuously read and discard UART data while it's available within the specified time
    while (uart_is_readable_within_us(uart_instance, 5000)) {
        uart_getc(uart_instance); // Read and discard UART data
    }
}

/**
 * Reads data from the UART for LoRa communication and stores it in the provided buffer.
 *
 * @param dst  Pointer to the buffer where the received data will be stored.
 * @param size The maximum size of the buffer for storing received data.
 * @return The number of characters read and stored in the buffer.
 */
int lora_read_uart(char *dst, int size) {
    int read = 0;
    char c = 0;

    // Read characters until a newline, reaching buffer size, or timeout
    while (c != '\n' && read < size && uart_is_readable_within_us(uart_instance, 5000)) {
        c = uart_getc(uart_instance);
        dst[read] = c;
        read++;
    }

    // Replace carriage return with zero for string termination
    read -= 2; // Adjust for CR and actual terminator
    dst[read] = '\0'; // Null-terminate the received string
    printf("Response received.\n"); // Log that a response was received

    return read; // Return the number of characters read and stored in the buffer
}

/**
 * Writes data to the LoRa UART interface.
 *
 * @param string The string data to be written to the LoRa UART.
 * @return True if the write operation succeeds, false otherwise.
 */
bool lora_write(char *string) {
    lora_empty_buffer(); // Flush the UART buffer

    size_t size = strlen(string);
    for (int i = 0; i < MAX_TRIES; i++) {
        // Check if UART is ready for writing and perform write operation
        if (uart_is_writable(uart_instance)) {
            uart_puts(uart_instance, string);
        }

        // Check for a response within the specified timeout
        if (uart_is_readable_within_us(uart_instance, 500000)) {
            return true; // Return true if a response is received
        } else if (i != 5) {
            printf("No response. Retrying...\n");
        }
    }

    printf("Couldn't get a response.\n");
    return false; // Return false if no response is received within the maximum attempts
}

/**
 * Waits for a response from the LoRa module after sending an AT command.
 *
 * @return 1 if a response is received within the specified timeout, otherwise returns 0.
 */
int lora_wait() {
    printf("Waiting for response from LoRa...\n");
    
    // Send an AT command to the LoRa module and check for a response
    if (lora_write("AT\r\n")) {
        lora_empty_buffer(); // Flush the UART buffer to prepare for a new response
        return 1; // Return 1 if a response is received
    }
    
    return 0; // Return 0 if no response is received within the timeout
}

/**
 * Requests the firmware version from the LoRa module.
 *
 * @param dst  Pointer to a character array to store the firmware version.
 * @param size The maximum size of the destination array.
 * @return The number of characters read into dst if successful, otherwise returns 0.
 */
int get_lora_firmware(char *dst, int size) {
    printf("Getting firmware version from LoRa module...\n");
    
    // Send a command to retrieve the firmware version and read the UART response
    if (lora_write("AT+VER\r\n")) {
        return lora_read_uart(dst, size); // Return the number of characters read into dst
    }
    
    return 0; // Return 0 if the firmware version retrieval fails
}

/**
 * Requests the DevEui from the LoRa module.
 *
 * @param dst  Pointer to a character array to store the DevEui.
 * @param size The maximum size of the destination array.
 * @return The number of characters read into dst if successful, otherwise returns 0.
 */
int get_lora_deveui(char *dst, int size) {
    printf("Getting DevEui from LoRa module...\n");
    
    // Send a command to retrieve the DevEui and read the UART response
    if (lora_write("AT+ID=DevEui\r\n")) {
        return lora_read_uart(dst, size); // Return the number of characters read into dst
    }
    
    return 0; // Return 0 if the DevEui retrieval fails
}

/**
 * Parses the received DevEui string to extract valid characters.
 *
 * @param src  Pointer to the source DevEui string.
 * @param dst  Pointer to the destination character array for parsed DevEui.
 * @param size The maximum size of the destination array.
 */
void parse_deveui(char *src, char *dst, int size) {
    if (strlen(src) > 13) src += 13; // Skip characters until the DevEui part starts
    
    int i = 0;
    int next = 0;
    
    // Loop through the source DevEui string and extract valid characters
    while (i < strlen(src) && next < size) {
        if (isalpha(src[i])) dst[next++] = tolower(src[i]); // Convert alphabetic characters to lowercase
        if (isdigit(src[i])) dst[next++] = src[i]; // Copy numeric characters directly
        i++;
    }
    
    dst[next] = '\0'; // Null-terminate the parsed DevEui string
}
