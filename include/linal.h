#ifndef LINAL
#define LINAL

#include "assert.h"
#include "stdio.h"
#include "math.h"
#include "stdbool.h"

typedef struct {
    float x;
    float y;
} vec2;

typedef struct {
    float x;
    float y;
    float z;
} vec3;

typedef struct {
    float x;
    float y;
    float z;
    float w;
} vec4;

#define VEC2(x, y)       (vec2){(x), (y)}
#define VEC3(x, y, z)    (vec3){(x), (y), (z)}
#define VEC4(x, y, z, w) (vec4){(x), (y), (z), (w)}

typedef struct {
    float m[2][2];
} mat2t;

typedef struct {
    float m[3][3];
} mat3t;

typedef struct {
    float m[4][4];
} mat4t;


const mat4t linal_mat4t_identity = {
    {{1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 1}}
};

// @TODO: rederive the matrix to get rid of the minus signs
// @TODO: more like make it work
mat4t linal_mat4t_frustum(float near, float far, float left, float right, float bottom, float top) {
    assert(left < right);
    assert(bottom < top);
    assert(near < far);
    
    mat4t result = {0};

    result.m[0][0] = (-2.0f * near) / (left - right);
    result.m[2][0] = (right + left) / (left - right);

    result.m[1][1] = (-2.0f * near) / (bottom - top);
    result.m[2][1] = (top + bottom) / (bottom - top);

    result.m[2][2] = -(far / (near - far));
    result.m[3][2] = ((near * far) / (near - far));

    result.m[2][3] = 1;

    return result;
}

vec3 vec3_scale(vec3 vec, float scalar) {
    return (vec3){vec.x * scalar, vec.y * scalar, vec.z * scalar};
}

vec4 vec4_scale(vec4 vec, float scalar) {
    return (vec4){vec.x * scalar, vec.y * scalar, vec.z * scalar, vec.w * scalar};
}

vec3 vec3_mult_mat3t(vec3 vec, mat3t mat) {
    (void)vec;
    (void)mat;
    assert(0 && "Not implemented");
}

vec4 vec4_mult_mat4t(vec4 v, mat4t m) { 
    return (vec4) {
        v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + v.w * m.m[3][0],
        v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + v.w * m.m[3][1],
        v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + v.w * m.m[3][2],
        v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + v.w * m.m[3][3],
    };
}

mat3t mat3t_mult(mat3t m1, mat3t m2) {
    mat3t res = {0};

    // @TODO: unroll this loop?
    // @TODO: smd?
    for (size_t i=0;i<3;++i) {
        for (size_t j=0; j<3; ++j) {
            for (size_t k=0; k<3; ++k) {
                res.m[i][j] += m1.m[i][k] * m2.m[k][j];
            }
        }
    }

    return res;
}

mat4t mat4t_mult(mat4t m1, mat4t m2) {
    mat4t res = {0};

    // @TODO: unroll this loop?
    // @TODO: smd?
    for (size_t i=0;i<4;++i) {
        for (size_t j=0; j<4; ++j) {
            for (size_t k=0; k<4; ++k) {
                res.m[i][j] += m1.m[i][k] * m2.m[k][j];
            }
        }
    }

    return res;
}

mat4t mat4t_transpose(mat4t m) {
    mat4t r = {0};
    for (size_t i=0;i<4;++i) {
        for(size_t j=0; j<4; ++j) {
            r.m[i][j] = m.m[j][i];
        }
    }
    return r;
}

void vec3_dump(vec3 v) {
    printf("vec3(%f, %f, %f)", v.x, v.y, v.z);
}

void vec4_dump(vec4 v) {
    printf("vec4(%f, %f, %f, %f)", v.x, v.y, v.z, v.w);
}

vec3 vec3_add(vec3 a, vec3 b) {
    return (vec3) {a.x + b.x, a.y + b.y, a.z + b.z};
}

vec4 vec4_add(vec4 a, vec4 b) {
    return (vec4) {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}

vec3 vec3_sub(vec3 a, vec3 b) {
    return (vec3) {a.x - b.x, a.y - b.y, a.z - b.z};
}

vec4 vec4_sub(vec4 a, vec4 b) {
    return (vec4) {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}

void dump_mat3t(mat3t m) {
    for (size_t i=0; i<3; ++i) {
        for (size_t j=0; j<3; ++j) {
            printf("%f\t", m.m[i][j]);
        }
        printf("\n");
    }
}

void dump_mat4t(mat4t m) {
    for (size_t i=0; i<4; ++i) {
        for (size_t j=0; j<4; ++j) {
            printf("%f\t", m.m[i][j]);
        }
        printf("\n");
    }
}

mat4t linal_rotation_matrix_translated(vec3 angles, vec3 translation) {
    mat4t x = {{
        {1, 0, 0, 0},
        {0, cos(angles.x * 2 * M_PI), -sin(angles.x * 2 * M_PI), 0},
        {0, sin(angles.x * 2 * M_PI), cos(angles.x * 2 * M_PI), 0},
        {0, 0, 0, 1}
    }};

    mat4t y = {{
        {cos(angles.y * 2 * M_PI), 0, sin(angles.y * 2 * M_PI), 0},
        {0, 1, 0, 0},
        {-sin(angles.y * 2 * M_PI), 0, cos(angles.y * 2 * M_PI), 0},
        {0, 0, 0, 1}
    }};

    mat4t z = {{
        {cos(angles.z * 2 * M_PI), -sin(angles.z * 2 * M_PI), 0, 0},
        {sin(angles.z * 2 * M_PI), cos(angles.z * 2 * M_PI), 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1}
    }};
    
    mat4t xy = mat4t_mult(x, y); 
    mat4t xyz = mat4t_mult(xy, z);

    xyz.m[3][0] = translation.x;
    xyz.m[3][1] = translation.y;
    xyz.m[3][2] = translation.z;

    return xyz;
}

mat4t linal_rotation_matrix_xz_translated(float angle, vec3 translation) {
    mat4t xz = {{
        {cos(angle * 2 * M_PI), 0, sin(angle * 2 * M_PI), 0},
        {0, 1, 0, 0},
        {-sin(angle * 2 * M_PI), 0, cos(angle * 2 * M_PI), 0},
        {translation.x, translation.y, translation.z, 1}
    }};

    return xz;
}

mat4t linal_rotation_matrix(vec3 angles) {
    mat4t x = {{
        {1, 0, 0, 0},
        {0, cos(angles.x * 2 * M_PI), -sin(angles.x * 2 * M_PI), 0},
        {0, sin(angles.x * 2 * M_PI), cos(angles.x * 2 * M_PI), 0},
        {0, 0, 0, 1}
    }};

    mat4t y = {{
        {cos(angles.y * 2 * M_PI), 0, sin(angles.y * 2 * M_PI), 0},
        {0, 1, 0, 0},
        {-sin(angles.y * 2 * M_PI), 0, cos(angles.y * 2 * M_PI), 0},
        {0, 0, 0, 1}
    }};

    mat4t z = {{
        {cos(angles.z * 2 * M_PI), -sin(angles.z * 2 * M_PI), 0, 0},
        {sin(angles.z * 2 * M_PI), cos(angles.z * 2 * M_PI), 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1}
    }};
    
    mat4t xy = mat4t_mult(x, y); 
    mat4t xyz = mat4t_mult(xy, z);

    return xyz;
}

mat4t linal_rotation_matrix_xy(float angle) {
    mat4t xy = {{
        {cos(angle * 2 * M_PI), -sin(angle * 2 * M_PI), 0, 0},
        {sin(angle * 2 * M_PI), cos(angle * 2 * M_PI), 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1}
    }};
    
    return xy;
}

mat4t linal_rotation_matrix_xz(float angle) {
    mat4t xz = {{
        {cos(angle * 2 * M_PI), 0, sin(angle * 2 * M_PI), 0},
        {0, 1, 0, 0},
        {-sin(angle * 2 * M_PI), 0, cos(angle * 2 * M_PI), 0},
        {0, 0, 0, 1}
    }};

    return xz;
}

mat4t linal_rotation_matrix_yz(float angle) {
    mat4t yz = {{
        {1, 0, 0, 0},
        {0, cos(angle * 2 * M_PI), -sin(angle * 2 * M_PI), 0},
        {0, sin(angle * 2 * M_PI), cos(angle * 2 * M_PI), 0},
        {0, 0, 0, 1}
    }};

    return yz;
}

mat4t mat4t_inverse(mat4t source) {
    float gj[4][8] = {0};

    for (size_t i=0; i<4; ++i) {
        for (size_t j=0; j<4; ++j) {
            gj[i][j] = source.m[i][j];
        }
    }

    gj[0][4] = 1.0f;
    gj[1][5] = 1.0f;
    gj[2][6] = 1.0f;
    gj[3][7] = 1.0f;

    for (size_t i=0;i<4;++i) {
        float first = gj[i][i];
        for (size_t j=0;j<8;++j) {
            gj[i][j] /= first;
        }

        for (size_t j=i+1;j<4;++j) {
            float first_row = gj[j][i];
            
            for (size_t k=0;k<8;++k) {
                gj[j][k] -= first_row  * gj[i][k];
            }
        }
    } 

    for (int64_t i=3;i>=0;--i) {
        /*
        float last = gj[i][i];

        for (size_t j=0;j<8;++j) {
            gj[i][j] /= last;
        }
        */
        
        for (int64_t j=i-1; j>=0;--j) {
            float last_row = gj[j][i];

            for (size_t k=0;k<8;++k) {
                gj[j][k] -= last_row * gj[i][k];
            }
        }
    }

    mat4t res;

    for (size_t i=0;i<4;++i) {
        for (size_t j=0;j<4;++j) {
            res.m[i][j] = gj[i][4+j];
        }
    }
   
#if 0 /* in case of bugs related to irreversable matrix, i guess */
    printf("res)-----------------------------\n");
    for (size_t i=0;i<4;++i) {
        for (size_t j=0;j<4;++j) {
            printf("%f\t", res.m[i][j]);
        }
        printf("\n");
    }
    printf("-------------------------------\n");
#endif

    return res;
}
#endif /* LINAL */
