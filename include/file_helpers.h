#ifndef FILE_HELPERS
#define FILE_HELPERS

#include "stdio.h"
#include "stdbool.h"

size_t fileSize(FILE *file) {
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    return size;
}

char *readEntireFile(const char *filename, size_t *out_size) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "%s: failed to open file\n", __FUNCTION__);
        return NULL;
    }
    
    size_t size = fileSize(file);
    char *data = malloc(size * sizeof(char));
    
    if (data == NULL) {
        fclose(file);
        return data;
    }

    size_t res = fread(data, sizeof(char), size, file);

    if (res != size) {
        fclose(file);
        free(data);
        return NULL;
    }

    if (out_size != NULL) {
        *out_size = size;
    }

    return data;
}

#endif /* FILE_HELPERS */
