//
// Created by hyprayzen on 3/30/26.
//

#ifndef HTTP_CLIENT_HTTP_PARSER_H
#define HTTP_CLIENT_HTTP_PARSER_H

#include <stdio.h>
#include "bytebuffer.h"

ssize_t FindBody (ByteBuffer * buffer);
int DecodeChunkedBody(ByteBuffer *raw_response, size_t body_start, ByteBuffer *clean_body);

#endif //HTTP_CLIENT_HTTP_PARSER_H
