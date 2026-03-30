//
// Created by hyprayzen on 3/30/26.
//

#ifndef HTTP_CLIENT_URL_PARSER_H
#define HTTP_CLIENT_URL_PARSER_H

typedef struct {
    char scheme[16];
    char host[256];
    char port[6];
    char path[2048];
} ParsedURL;

ParsedURL * checkArguments(int count, char * argv[]);

#endif //HTTP_CLIENT_URL_PARSER_H
