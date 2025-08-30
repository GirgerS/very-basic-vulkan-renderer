#ifndef CAMERA_H
#define CAMERA_H

#include "linal.h"
#include "linal_quat.h"

typedef struct {
    quaternion direction;
    vec3 position;
} Camera;

void fps_camera_rotate(Camera *camera, float angle_x, float angle_y) { 
    quaternion y_dir_diff = quat_from_angle_axis(angle_y, VEC3(0, 1, 0));
    camera->direction = quat_mult(camera->direction, y_dir_diff);

    vec3 adjusted_x_axis  = vec_rotate_by_quat(VEC3(-1, 0, 0), quat_conjugate(camera->direction));
    quaternion x_dir_diff = quat_from_angle_axis(angle_x, adjusted_x_axis);
    camera->direction = quat_mult(camera->direction, x_dir_diff);
}

Camera fps_camera_default() {
    return (Camera){0};
}


#endif /* CAMERA_H */ 
