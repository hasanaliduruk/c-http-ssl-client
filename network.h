//
// Created by hyprayzen on 3/30/26.
//

#ifndef HTTP_CLIENT_NETWORK_H
#define HTTP_CLIENT_NETWORK_H

#include <openssl/ssl.h>
#include <sys/types.h>

typedef struct {
    int socket_fd;
    SSL_CTX *ssl_ctx;
    SSL *ssl;
    int is_https;
} ConnectionContext;

ConnectionContext* ConnectToServer(const char* host, const char* port, int use_https);
ssize_t SendData(ConnectionContext *ctx, const void *buf, size_t len);
ssize_t ReceiveData(ConnectionContext *ctx, void *buf, size_t len);
void CloseConnection(ConnectionContext *ctx);

#endif //HTTP_CLIENT_NETWORK_H
