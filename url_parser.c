//
// Created by hyprayzen on 3/30/26.
//

#include "url_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ParsedURL * checkArguments(int count, char * argv[])
{
    if (count != 2) {
        printf("Wrong program arguments please try again! [Correct form: programName <url>]");
        exit(1);
    }
    char * url = argv[1];
    ParsedURL * parsed_url = (ParsedURL *) calloc(1, sizeof(ParsedURL));
    if (parsed_url == NULL) {
        fprintf(stderr, "Critical Error: Cannot allocate memory!");
        exit(1);
    }

    // scheme
    char * scheme = strstr(url, "://");
    if (scheme != NULL) {
        memcpy(&(parsed_url->scheme), url, scheme - url);
        scheme = scheme + 3;
    }
    else {
        strcpy(parsed_url->scheme, "http"); // default
        scheme = url;
    }


    char * path = strchr(scheme, '/');
    if (path != NULL) {
        strcpy(parsed_url->path, path);
        *path = '\0'; // to find host and port crop the string from here
    }
    else {
        strcpy(parsed_url->path, "/");
    }


    char * colon = strchr(scheme, ':');
    if (colon != NULL) {
        memcpy(&(parsed_url->host), scheme, colon - scheme);
        strcpy(parsed_url->port, colon + 1);
    }
    else {
        strcpy(parsed_url->host, scheme);
        if (!strcmp(parsed_url->scheme, "https"))
            strcpy(parsed_url->port, "443"); // https default port
        else
            strcpy(parsed_url->port, "80"); // http default port
    }
    return parsed_url;
}