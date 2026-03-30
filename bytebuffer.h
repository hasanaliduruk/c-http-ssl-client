//
// Created by hyprayzen on 3/30/26.
//
#ifndef HTTP_CLIENT_BYTEBUFFER_H
#define HTTP_CLIENT_BYTEBUFFER_H

#include <stdlib.h>
#include <stdint.h>

#define INITIAL_CAPACITY 8192

typedef struct {
    uint8_t *data;
    size_t size;
    size_t capacity;
} ByteBuffer;

void init_buffer(ByteBuffer *buf);

void append_buffer(ByteBuffer *buf, const uint8_t *new_data, size_t length);

void free_buffer(ByteBuffer *buf);

#endif //HTTP_CLIENT_BYTEBUFFER_H
