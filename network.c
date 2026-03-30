//
// Created by hyprayzen on 3/30/26.
//
#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <openssl/err.h>
#include <sys/time.h>


ConnectionContext* ConnectToServer(const char* host, const char* port, int use_https) {
    ConnectionContext * context = (ConnectionContext *) malloc(sizeof(ConnectionContext));
    if (context == NULL) {
        fprintf(stderr, "Critical Error: Memory allocation for connection context failed!");
        return NULL;
    }
    context->ssl_ctx = NULL; context->ssl = NULL;
    context->is_https = use_https;

    struct addrinfo hints, *res;
    char ipstr[INET6_ADDRSTRLEN];
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC; // IPv4 or IPv6 doesn't matter.
    hints.ai_socktype = SOCK_STREAM; // TCP flow socket request.


    int status = getaddrinfo(host, port, &hints, &res);
    if (status != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        free(context);
        return NULL;
    }

    printf("for %s IP addresses:\n\n", host);

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
                fprintf(stderr, "Error: Cannot connect to address: %s\n", host);
            }
            else
                printf("Connection Successful! Socket FD: %d\n", socketRes);

        }
    }
    freeaddrinfo(res);
    if (socketRes == -1) {
        free(context);
        return NULL;
    }
    context->socket_fd = socketRes;

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    if (setsockopt(socketRes, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        fprintf(stderr, "Critical Error: Cannot set timeout!\n");
        close(socketRes);
        free(context);
        return NULL;
    }
    printf("System Protection: Socket reading timeout has been locked as %ld sec.\n", timeout.tv_sec);

    if (use_https) {
        // Creating TLS context
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        context->ssl_ctx = SSL_CTX_new(TLS_client_method());
        if (context->ssl_ctx == NULL) {
            fprintf(stderr, "Critical Error: Cannot create SSL Context.\n");
            ERR_print_errors_fp(stderr);
            close(socketRes);
            free(context);
            return NULL;
        }

        // Create SSL objects and connect to TCP socket
        context->ssl = SSL_new(context->ssl_ctx);
        SSL_set_fd(context->ssl, socketRes);

        // Server name indiction (SNI)
        SSL_set_tlsext_host_name(context->ssl, host);

        // TLS Handshake
        if (SSL_connect(context->ssl) <= 0) {
            fprintf(stderr, "Critical Error: TLS Handshake failed!\n");
            ERR_print_errors_fp(stderr);
            SSL_free(context->ssl);
            SSL_CTX_free(context->ssl_ctx);
            close(socketRes);
            free(context);
            return NULL;
        }

        printf("TLS Handshake successfull. Encrypted tunnel active.\n");
    }
    else {
        printf("System Status: HTTP connection active.\n");
    }

    return context;
}

ssize_t SendData(ConnectionContext *ctx, const void *buf, size_t len) {
    if (ctx == NULL || ctx->socket_fd < 0)
        return -1;
    if (ctx->is_https)
        return SSL_write(ctx->ssl, buf, len);
    return send(ctx->socket_fd, buf, len, 0);
}

ssize_t ReceiveData(ConnectionContext *ctx, void *buf, size_t len) {
    if (ctx == NULL || ctx->socket_fd < 0)
        return -1;
    if (ctx->is_https)
        return SSL_read(ctx->ssl, buf, len);
    return recv(ctx->socket_fd, buf, len, 0);
}

void CloseConnection(ConnectionContext *ctx) {
    if (ctx == NULL)
        return;

    if (ctx->is_https) {
        if (ctx->ssl) {
            SSL_shutdown(ctx->ssl);
            SSL_free(ctx->ssl);
        }
        if (ctx->ssl_ctx)
            SSL_CTX_free(ctx->ssl_ctx);
    }

    if (ctx->socket_fd >= 0) close(ctx->socket_fd);

    free(ctx);
}
