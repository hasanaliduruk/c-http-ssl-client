#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> // System data structures.
#include <sys/socket.h> // Basic socket functions and data structures.
#include <netdb.h> // Network database processes.
#include <unistd.h> // Standard symbolic constants.
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "heaplist.h"
#include "bytebuffer.h"
#include "url_parser.h"







int SaveBufferToFile(const char *filename, ByteBuffer *buffer);


int main(int argc, char * argv[])
{
    ParsedURL * parsed_url = checkArguments(argc, argv);
    int is_https = (strcmp(parsed_url->scheme, "https") == 0);

    SSL_CTX *ctx = NULL;
    SSL *ssl = NULL;


    struct addrinfo hints, *res;
    char ipstr[INET6_ADDRSTRLEN];
    HeapList * heap_list = CreateList();
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC; // IPv4 or IPv6 doesn't matter.
    hints.ai_socktype = SOCK_STREAM; // TCP flow socket request.


    int status = getaddrinfo(parsed_url->host, parsed_url->port, &hints, &res);
    if (status != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        free(parsed_url);
        return 1;
    }

    printf("for %s IP addresses:\n\n", parsed_url->host);

    struct addrinfo * tmp; // temp variable for link list iteration.
    int socketRes = -1;
    for (tmp = res; tmp != NULL && socketRes == -1; tmp = tmp->ai_next)
    {
        void *addr;
        char *ipver;

        if (tmp->ai_family == AF_INET) // IPv4
        {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)tmp->ai_addr; // convert generic sockaddr struct to specific structure.
            addr = &(ipv4->sin_addr); // that is the data that hold ip address.
            ipver = "IPv4";
        }
        else
        {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)tmp->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }
        inet_ntop(tmp->ai_family, addr, ipstr, sizeof(ipstr));
        printf("%s: %s\n", ipver, ipstr);
        printf("\tTrying to open socket for: %s\n", ipstr);
        socketRes = socket(tmp->ai_family, tmp->ai_socktype, tmp->ai_protocol);
        if (socketRes != -1)
        {
            printf("Socket Opened\n");
            if (connect(socketRes, tmp->ai_addr, tmp->ai_addrlen) == -1) // Socket opened but connection failed.
            {
                close(socketRes);
                socketRes = -1; // continue to iteration (connection failed).
                fprintf(stderr, "Error: Cannot connect to address: %s\n", parsed_url->host);
            }
            else
                printf("Connection Successful! Socket FD: %d\n", socketRes);

        }
    }
    freeaddrinfo(res);

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    if (setsockopt(socketRes, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        fprintf(stderr, "Critical Error: Cannot set timeout!\n");
        close(socketRes);
        cleanUp(heap_list);
        exit(1);
    }
    printf("System Protection: Socket reading timeout has been locked as %ld sec.\n", timeout.tv_sec);

    if (is_https) {
        // Creating TLS context
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        ctx = SSL_CTX_new(TLS_client_method());
        if (ctx == NULL) {
            fprintf(stderr, "Critical Error: Cannot create SSL Context.\n");
            ERR_print_errors_fp(stderr);
            close(socketRes);
            cleanUp(heap_list);
            exit(1);
        }

        // Create SSL objects and connect to TCP socket
        ssl = SSL_new(ctx);
        SSL_set_fd(ssl, socketRes);

        // Server name indiction (SNI)
        SSL_set_tlsext_host_name(ssl, parsed_url->host);

        // TLS Handshake
        if (SSL_connect(ssl) <= 0) {
            fprintf(stderr, "Critical Error: TLS Handshake failed!\n");
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            SSL_CTX_free(ctx);
            close(socketRes);
            cleanUp(heap_list);
            exit(1);
        }

        printf("TLS Handshake successfull. Encrypted tunnel active.\n");
    }
    else {
        printf("System Status: HTTP connection active.\n");
    }

    // HTTP GET request identifier.
    const char *req_format = "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n";
    size_t req_len = strlen(req_format) + strlen(parsed_url->host) + strlen(parsed_url->path) + 1;

    char *request = (char *)malloc(req_len);
    if (request == NULL) {
        fprintf(stderr, "Critical Error: Memory allocation fault!\n");
        cleanUp(heap_list);
        exit(1);
    }
    InsertToLast(heap_list, request);

    snprintf(request, req_len, req_format, parsed_url->path, parsed_url->host);

    ssize_t request_len = (ssize_t) strlen(request);
    ssize_t total_sent = 0;
    ssize_t bytes_left = request_len;
    ssize_t n;

    // Partial send secured sending algorithm.
    while (total_sent < request_len)
    {
        // shift request starting index by total_sent value to find place where we left.
        //n = send(socketRes, request + total_sent, bytes_left, 0);
        if (is_https)
            n = SSL_write(ssl, request + total_sent, bytes_left);
        else
            n = send(socketRes, request + total_sent, bytes_left, 0);
        if (n == -1) {
            fprintf(stderr, "Critical Error: Socket error while sending data!");
            close(socketRes);
            free(parsed_url);
            return 3;
        }
        total_sent += n;
        bytes_left -= n;
    }

    printf("HTTP GET request (%ld byte) sent to target successfully.\n", total_sent);

    ByteBuffer raw_response;
    init_buffer(&raw_response);

    int buffer_size = 4096;
    uint8_t rec_buffer[buffer_size]; // A buffer to hold string that we receive.
    int receive_status;

    printf("Waiting for response...\n");

    do {
        //receive_status = recv(socketRes, rec_buffer, buffer_size, 0);
        if (is_https)
            receive_status = SSL_read(ssl, rec_buffer, buffer_size);
        else
            receive_status = recv(socketRes, rec_buffer, buffer_size, 0);
        if (receive_status > 0)
        {
            append_buffer(&raw_response, rec_buffer, (size_t)receive_status);
        }
        else if (receive_status == 0)
            printf("Connection closed by server (EOF)\n");
        else { // receive_status < 0 durumu
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                fprintf(stderr, "Timeout: Server isn't sending data for %ld seconds.\n", timeout.tv_sec);
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
            exit(1);
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
                SaveBufferToFile("output_file.dat", &clean_html);
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

                    SaveBufferToFile("output_file.dat", &clean_body);

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

                SaveBufferToFile("output_file.dat", &clean_body);

                free_buffer(&clean_body);
            }
        }
        free(headers_str);
    } else {
        fprintf(stderr, "Error: Valid HTTP response not found.\n");
    }
    cleanUp(heap_list);
    free(parsed_url);
    free_buffer(&raw_response);
    if (is_https) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
    }
    close(socketRes);
    return 0;
}

int SaveBufferToFile(const char *filename, ByteBuffer *buffer) {
    if (buffer == NULL || buffer->size == 0) {
        fprintf(stderr, "Error: No data to write.\n");
        return -1;
    }

    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        fprintf(stderr, "Critical Error: File '%s' has not been created. Check the permissions.\n", filename);
        return -1;
    }

    size_t written = fwrite(buffer->data, 1, buffer->size, file);
    fclose(file);

    if (written != buffer->size) {
        fprintf(stderr, "Critical Error: Missing I/O writing. (Expected: %zu, Written: %zu)\n", buffer->size, written);
        return -1;
    }

    printf("Successfull: %zu byte data saved to '%s'.\n", written, filename);
    return 0;
}
