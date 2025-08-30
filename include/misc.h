#ifndef MISC_H
#define MISC_H

#include "linal_quat.h"
#include "linal.h"

typedef struct {
    vec3 pos;
    vec4 color;
} Vertex;

typedef struct {
    Vertex *vertices;
    size_t vertices_len;
    quaternion orientation;
    vec3 position; 
} Model;

#endif /* MISC_H */
