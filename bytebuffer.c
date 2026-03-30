//
// Created by hyprayzen on 3/30/26.
//

#include "bytebuffer.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void init_buffer(ByteBuffer *buf) {
    buf->data = (uint8_t *)malloc(INITIAL_CAPACITY);
    if (buf->data == NULL) {
        fprintf(stderr, "Critical error: Memory allocation fault!\n");
        exit(EXIT_FAILURE);
    }
    buf->size = 0;
    buf->capacity = INITIAL_CAPACITY;
}

void append_buffer(ByteBuffer *buf, const uint8_t *new_data, size_t length) {
    if (length == 0) return;

    if (buf->size + length > buf->capacity) {
        size_t new_capacity = buf->capacity * 2;
        while (buf->size + length > new_capacity) {
            new_capacity *= 2;
        }

        uint8_t *temp = (uint8_t *)realloc(buf->data, new_capacity);
        if (temp == NULL) {
            fprintf(stderr, "Critical error: Memory allocation fault!\n");
            free(buf->data);
            exit(EXIT_FAILURE);
        }
        buf->data = temp;
        buf->capacity = new_capacity;
    }

    memcpy(buf->data + buf->size, new_data, length);
    buf->size += length;
}

void free_buffer(ByteBuffer *buf) {
    if (buf->data != NULL) {
        free(buf->data);
        buf->data = NULL;
    }
    buf->size = 0;
    buf->capacity = 0;
}