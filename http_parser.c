//
// Created by hyprayzen on 3/30/26.
//
#include "http_parser.h"
#include <string.h>

ssize_t FindBody (ByteBuffer * buffer) {
    if (buffer->size < 4)
        return -1; // nothing to check
    for (size_t i = 0; i <= buffer->size - 4; i ++) {
        if (buffer->data[i] == '\r' && buffer->data[i + 1] == '\n' &&
            buffer->data[i + 2] == '\r' && buffer->data[i + 3] == '\n')
            return (ssize_t) i + 4; // body starting index.
    }
    return -1; // Doesn't include "\r\n\r\n"
}
int DecodeChunkedBody(ByteBuffer *raw_response, size_t body_start, ByteBuffer *clean_body) {
    if (body_start >= raw_response->size) {
        return -1;
    }

    uint8_t *ptr = raw_response->data + body_start;
    size_t remaining = raw_response->size - body_start;

    while (remaining > 0) {
        // Find where \r\n (CRLF) starts
        uint8_t *crlf_pos = NULL;
        for (size_t i = 0; i < remaining - 1; i++) {
            if (ptr[i] == '\r' && ptr[i+1] == '\n') {
                crlf_pos = ptr + i;
                break;
            }
        }

        if (crlf_pos == NULL) {
            fprintf(stderr, "Kritik Hata: Chunk boyutu okunurken CRLF bulunamadı (Eksik veri).\n");
            return -1;
        }

        // 2. Hexadecimal sayıyı ayrıştır
        // strtol standart C-string (null-terminated) beklediği için geçici bir tampon oluşturulmalıdır.
        size_t hex_len = crlf_pos - ptr;
        char hex_str[32] = {0}; // 32 bayt bir boyut tanımı için fazlasıyla yeterlidir

        if (hex_len >= sizeof(hex_str) - 1) {
            fprintf(stderr, "Critical Error: Hexadecimal length arr is too long (Buffer Overflow risk).\n");
            return -1;
        }

        memcpy(hex_str, ptr, hex_len);

        char *endptr;
        long chunk_size = strtol(hex_str, &endptr, 16);

        if (endptr == hex_str || chunk_size < 0) {
            fprintf(stderr, "Critical Error: Cannot decrypt a valid hexadecimal size.\n");
            return -1;
        }

        // Ending state (0\r\n\r\n)
        if (chunk_size == 0) {
            break;
        }

        size_t header_offset = hex_len + 2;

        if (remaining < header_offset + chunk_size + 2) {
            fprintf(stderr, "Critical Error: In the buffer, there is not enough data to satisfy expected chunk size.\n");
            return -1;
        }

        uint8_t *data_start = ptr + header_offset;

        append_buffer(clean_body, data_start, chunk_size);

        // shift the pointer to the amount of head + data + CRLF at the end.
        size_t total_chunk_block_size = header_offset + chunk_size + 2;
        ptr += total_chunk_block_size;
        remaining -= total_chunk_block_size;
    }

    return 0;
}