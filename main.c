#include <stdio.h>
#include <stdlib.h>

#include "bytebuffer.h"
#include "url_parser.h"
#include "http_client.h"



int SaveBufferToFile(const char *filename, ByteBuffer *buffer);


int main(int argc, char * argv[]) {
    ParsedURL * parsed_url = checkArguments(argc, argv);

    ByteBuffer result_data;
    init_buffer(&result_data);

    printf("Process Starting: %s -> %s\n", parsed_url->scheme, parsed_url->host);

    if (FetchURL(parsed_url, &result_data) == 0) {
        SaveBufferToFile("output_file.dat", &result_data);
    } else {
        fprintf(stderr, "Critical Error: File downloading process failed.\n");
    }

    free_buffer(&result_data);
    free(parsed_url);
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
