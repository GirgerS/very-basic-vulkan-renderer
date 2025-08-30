#include "linal.h"
#include "stdio.h"

// These are not actual unit tests, just some code I HAVE USED to test some functions

int main() {
#if 0   
    /* mult test */ {
        mat3t m1 = {
            {{0.1, 2, 3},
            {42, 5, 6},
            {7, 8, 9}}
        };
    
        mat3t m2 = {
            {{1, 2, 3},
            {4, 0.5, 69},
            {0.7, 111, 9}}
        };
    
        mat3t res = mat3t_mult(m1, m2);
        
        printf("Mult test-------------------\n");
        dump_mat3t(res);
        printf("-----------------------\n");
    }
#endif

    /* rotation */ {
        mat4t rotation_matrix = linal_rotation_matrix(
            (vec3){-0.125, 0.5, 0.125}
        );
    
        printf("Rotation matrix\n");
        dump_mat4t(rotation_matrix);
        printf("-----------------------\n");

        vec4 vec = {0, 2.5, 0, 0};
        vec4 rotated_vec = vec4_mult_mat4t(vec, rotation_matrix);
        printf("Rotated vector:\n");
        vec4_dump(rotated_vec); 
        printf("\n-----------------------\n"); 
    }

    return 0;
}
