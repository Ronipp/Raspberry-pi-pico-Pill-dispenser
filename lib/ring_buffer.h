//
// Created by keijo on 4.11.2023.
//

#ifndef UART_IRQ_RING_BUFFER_H
#define UART_IRQ_RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int num;
    uint32_t timestamp;
} logdata;

typedef struct  {
    int head;
    int tail;
    int size;
    logdata *buffer;
} ring_buffer;

void rb_init(ring_buffer *rb, logdata *buffer, int size);
bool rb_empty(ring_buffer *rb);
bool rb_full(ring_buffer *rb);
bool rb_put(ring_buffer *rb, logdata data);
logdata rb_get(ring_buffer *rb);

void rb_alloc(ring_buffer *rb, int size);
void rb_free(ring_buffer *rb);

#endif //UART_IRQ_RING_BUFFER_H
