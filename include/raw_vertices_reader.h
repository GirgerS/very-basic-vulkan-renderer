/* Minimal reader for simple format */

#ifndef RAW_VERTICES_READER
#define RAW_VERTICES_READER

#include "stdlib.h"

#include "linal.h"
#include "file_helpers.h"

void dataTrimLeft(char *data, size_t *fp, size_t file_size) {
    while (data[*fp] == ' ' || data[*fp] == '\t' || data[*fp] == '\n') {
        if ((++(*fp)) >= file_size) {
            break;
        }
    } 
}

float readVertexComponent(char *data, size_t *fp, char *buffer, size_t file_size) {
    uint16_t buffer_size = 0;
    while(data[*fp] != ' ' && data[*fp] != '\n' && data[*fp] != '\t') {
        if (buffer_size >= 1023 || *fp >= file_size) {
            return NAN; 
        }
        buffer[buffer_size++] = data[(*fp)++];
    }
    buffer[buffer_size] = '\0';

    char *number_end;
    errno = 0;
    
    float res = strtof(buffer, &number_end);
    if (errno != 0 || (buffer + buffer_size) != number_end) {
        return NAN;
    }
    
    return res;
}

/* Specified file should have format like 'teapot_bezier0.tris' file */
void *readVerticesFromFile(const char* filename, size_t *out_size) {
    char buffer[1024];

    size_t file_size;
    char *data = readEntireFile(filename, &file_size); 
    if (data == NULL) {
        return NULL;
    }

    if (file_size == 0) {
        fprintf(stderr, "%s: given file(%s) is empty", __FUNCTION__, filename);
        return NULL;
    }

    size_t fp = 0;
    while(data[fp] != '\n') {
        buffer[fp] = data[fp];

        if (fp >= 1023 || fp >= file_size) {
            fprintf(stderr, "%s: Number character buffer size(1024 bytes) exceeded or file ended, last read symbol: %zu", __FUNCTION__, fp);
            free(data);
            return NULL;
        }
        ++fp;
    }
    buffer[fp] = '\0';
    char *number_end; 
    errno = 0; 
    int64_t vert_count = strtoll(buffer, &number_end, 10) * 3;

    if (vert_count < 0) {
        fprintf(stderr, "Negative amount of vertices");
        free(data);
        return NULL;
    }
    
    if (errno != 0 || (buffer + fp) != number_end) {
        fprintf(stderr, "%s: Failed to convert vertex count to int", __FUNCTION__);
        free(data);
        return NULL;
    }
 
    vec3 *res = malloc(vert_count * sizeof(vec3));   
    if (res == NULL) {
        free(data);
        return NULL;
    }

    for (int64_t i=0; i<vert_count; ++i) {
        dataTrimLeft(data, &fp, file_size);
        float x = readVertexComponent(data, &fp, buffer, file_size);
        dataTrimLeft(data, &fp, file_size);
        float y = readVertexComponent(data, &fp, buffer, file_size);
        dataTrimLeft(data, &fp, file_size);
        float z = readVertexComponent(data, &fp, buffer, file_size);

        if (isnan(x) || isnan(y) || isnan(z)) {
            fprintf(stderr, "%s: Failed to read float, last read symbol: %zu\n", __FUNCTION__, fp);
            free(data);
            free(res);
            return NULL;
        }

        res[i] = (vec3) {
            x, y, z 
        }; 
    }
  
    if (out_size != NULL) {
        *out_size = (size_t)vert_count;
    }

    dataTrimLeft(data, &fp, file_size);
    if (fp != file_size) {
        fprintf(stderr, "%s: read %lld chars, but have not reached eof", __FUNCTION__, fp);
    }

    free(data);

    return res;
}

#endif

