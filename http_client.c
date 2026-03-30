//
// Created by hyprayzen on 3/30/26.
//
#include "http_client.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "network.h"
#include "http_parser.h"

int FetchURL(ParsedURL *parsed_url, ByteBuffer *clean_output) {
    int is_https = (strcmp(parsed_url->scheme, "https") == 0);
    ConnectionContext * context = ConnectToServer(parsed_url->host, parsed_url->port, is_https);
    if (context == NULL) {
        return -1;
    }

    // RFC 7230
    char *request = NULL;
    int exact_len = 0;

    if (strcmp(parsed_url->method, "POST") == 0 || strcmp(parsed_url->method, "PUT") == 0) {
        size_t payload_len = strlen(parsed_url->payload);
        const char *req_format = "%s %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %zu\r\n\r\n%s";

        // Calculate required memory size.
        exact_len = snprintf(NULL, 0, req_format, parsed_url->method, parsed_url->path, parsed_url->host, payload_len, parsed_url->payload);

        // Allocate Mem
        request = (char *)malloc(exact_len + 1);
        if (request == NULL) {
            fprintf(stderr, "Critical Error: Memory allocation fault for request string!\n");
            CloseConnection(context);
            return -1;
        }

        // Parse the data
        snprintf(request, exact_len + 1, req_format, parsed_url->method, parsed_url->path, parsed_url->host, payload_len, parsed_url->payload);
    } else {
        // GET, DELETE without body
        const char *req_format = "%s %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n";

        exact_len = snprintf(NULL, 0, req_format, parsed_url->method, parsed_url->path, parsed_url->host);

        request = (char *)malloc(exact_len + 1);
        if (request == NULL) {
            fprintf(stderr, "Critical Error: Memory allocation fault for request string!\n");
            CloseConnection(context);
            return -1;
        }

        snprintf(request, exact_len + 1, req_format, parsed_url->method, parsed_url->path, parsed_url->host);
    }

    ssize_t request_len = (ssize_t) strlen(request);
    ssize_t total_sent = 0;
    ssize_t bytes_left = request_len;
    ssize_t n;

    while (total_sent < request_len)
    {
        // shift request starting index by total_sent value to find place where we left.
        n = SendData(context, request + total_sent, bytes_left);
        if (n == -1) {
            fprintf(stderr, "Critical Error: Socket error while sending data!");
            CloseConnection(context);
            free(request);
            return -1;
        }
        total_sent += n;
        bytes_left -= n;
    }
    printf("HTTP request (%ld byte) sent to target successfully.\n", total_sent);

    ByteBuffer raw_response;
    init_buffer(&raw_response);

    int buffer_size = 4096;
    uint8_t rec_buffer[buffer_size]; // A buffer to hold string that we receive.
    ssize_t receive_status;

    printf("Waiting for response...\n");

    do {
        //receive_status = recv(socketRes, rec_buffer, buffer_size, 0);
        receive_status = ReceiveData(context, rec_buffer, buffer_size);
        if (receive_status > 0)
        {
            append_buffer(&raw_response, rec_buffer, (size_t)receive_status);
        }
        else if (receive_status == 0)
            printf("Connection closed by server (EOF)\n");
        else { // receive_status < 0 condution
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                fprintf(stderr, "Timeout: Socket timed out.\n");
                break;
            } else {
                fprintf(stderr, "Critical Error: Socket reading process collapsed (errno: %d)\n", errno);
                break;
            }
        }
    } while (receive_status > 0);

    printf("Total %zu byte data read successfully.\n", raw_response.size);

    ssize_t body_start = FindBody(&raw_response);
    if (body_start > 0) {
        char *headers_str = (char *)malloc(body_start + 1);
        if (headers_str == NULL) {
            fprintf(stderr, "Critical Error: Memory allocation fault!\n");
            free_buffer(&raw_response);
            free(request);
            CloseConnection(context);
            return -1;
        }

        memcpy(headers_str, raw_response.data, body_start);
        headers_str[body_start] = '\0';

        if (strstr(headers_str, "Transfer-Encoding: chunked") != NULL) {
            printf("System detected chunked data flow. Chunked read algorithm will execute.\n");
            ByteBuffer clean_html;
            init_buffer(&clean_html);

            int decode_status = DecodeChunkedBody(&raw_response, body_start, &clean_html);

            if (decode_status == 0) {
                printf("Parsing successfull. pure HTML size: %zu Byte\n", clean_html.size);
                append_buffer(clean_output, clean_html.data, clean_html.size);
            }

            free_buffer(&clean_html);
        } else {
            char *cl_ptr = strstr(headers_str, "Content-Length:");

            if (cl_ptr != NULL) {
                cl_ptr += 15;
                // Pass the spaces if so
                while (*cl_ptr == ' ') {
                    cl_ptr++;
                }

                char *endptr;
                long expected_len = strtol(cl_ptr, &endptr, 10);

                if (endptr == cl_ptr || expected_len < 0) {
                    fprintf(stderr, "Critical Error: Content-Length value invalid or corrupted.\n");
                } else {
                    size_t expected_size = (size_t)expected_len;
                    size_t received_size = raw_response.size - body_start;

                    printf("Content-Length detected: %zu Byte.\n", expected_size);

                    if (received_size < expected_size) {
                        fprintf(stderr, "Critical Warning: Network error or partial read!\n");
                        fprintf(stderr, "Expected: %zu, Received: %zu\n", expected_size, received_size);
                        expected_size = received_size;
                    } else if (received_size > expected_size) {
                        printf("Warning: Server sent more than expected, rest will be cropped.\n");
                    } else {
                        printf("Data Verified.\n");
                    }

                    // Isolate pure data
                    ByteBuffer clean_body;
                    init_buffer(&clean_body);
                    append_buffer(&clean_body, raw_response.data + body_start, expected_size);

                    append_buffer(clean_output, clean_body.data, clean_body.size);

                    free_buffer(&clean_body);
                }
            } else {
                // HTTP/1.0 Fallback
                printf("Warning: Cannot locate Transfer-Encoding or Content-Length header.\n");
                printf("Until the connection closed, all the body will be received (EOF Reading mode).\n");

                size_t received_size = raw_response.size - body_start;
                ByteBuffer clean_body;
                init_buffer(&clean_body);
                append_buffer(&clean_body, raw_response.data + body_start, received_size);

                append_buffer(clean_output, clean_body.data, clean_body.size);

                free_buffer(&clean_body);
            }
        }
        free(headers_str);
    } else {
        fprintf(stderr, "Error: Valid HTTP response not found.\n");
        free_buffer(&raw_response);
        free(request);
        CloseConnection(context);
        return -1;
    }
    free(request);
    free_buffer(&raw_response);
    CloseConnection(context);
    return 0;
}