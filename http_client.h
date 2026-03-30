//
// Created by hyprayzen on 3/30/26.
//

#ifndef HTTP_CLIENT_HTTP_CLIENT_H
#define HTTP_CLIENT_HTTP_CLIENT_H

#include "url_parser.h"
#include "bytebuffer.h"

// Success 0, Failed -1.
int FetchURL(ParsedURL *parsed_url, ByteBuffer *clean_output);

#endif //HTTP_CLIENT_HTTP_CLIENT_H
