#ifndef LINAL_QUAT
#define LINAL_QUAT

/* 
 * Terribly-terrible header with stolen unoptimized code 
 * Performing most unintuitive operations ever
*/

#include "math.h"
#include "linal.h"

typedef struct {
    float x;
    float y;
    float z;
    float w;
} quaternion;

float quat_len(quaternion quat) {
    return sqrt(quat.x * quat.x + quat.y * quat.y + quat.z * quat.z + quat.w * quat.w);
}

float quat_norm(quaternion quat) {
    return quat.x * quat.x + quat.y * quat.y + quat.z * quat.z + quat.w * quat.w;
}

quaternion quat_normalized(quaternion quat) {
    float len = quat_len(quat);
    quaternion res = quat;

    res.x /= len;
    res.y /= len;
    res.z /= len;
    res.w /= len;

    return res;
}

quaternion quat_mult(quaternion q1, quaternion q2) {
    quaternion res; 

    res.x = q1.w*q2.x + q1.x*q2.w + q1.y*q2.z - q1.z*q2.y;
    res.y = q1.w*q2.y + q1.y*q2.w + q1.z*q2.x - q1.x*q2.z;
    res.z = q1.w*q2.z + q1.z*q2.w + q1.x*q2.y - q1.y*q2.x;
    res.w = q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z;

    return res;
}

quaternion quat_conjugate(quaternion quat) {
    quaternion res;

    res.x = -quat.x;
    res.y = -quat.y;
    res.z = -quat.z;
    res.w = quat.w;

    return res;
}

quaternion quat_inverse(quaternion quat) {
    float norm = quat_norm(quat);
    
    quaternion res = quat_conjugate(quat);
    res.x /= norm;
    res.y /= norm;
    res.z /= norm;
    res.w /= norm;

    return res;
}

quaternion quat_rotate(quaternion q1, quaternion q2) {
    quaternion i = quat_mult(q2, q1); 
    quaternion q2i = quat_inverse(q2);
    return quat_mult(i, q2i); 
}

quaternion quat_lerp(quaternion q1, quaternion q2, float t) {
    quaternion res;
    res.x = q1.x + ((q2.x) - (q1.x)) * t;
    res.y = q1.y + ((q2.y) - (q1.y)) * t;
    res.z = q1.z + ((q2.z) - (q1.z)) * t;
    res.w = q1.w + ((q2.w) - (q1.w)) * t;
    
    return res;
}

/* specified angle is from 0 to 1 */
quaternion quat_from_angle_axis(float angle, vec3 axis) {
    float s = sin(angle * M_PI);

    quaternion res;
    res.x = axis.x * s;
    res.y = axis.y * s;
    res.z = axis.z * s;
    res.w = cos(angle * M_PI);

    return res;
}

/* I have absolutely no idea why it works */
/* I also don't know if it works at all :-) */

mat4t quat_unit_to_mat4t(quaternion q) {
    (void)q;
    assert(0 && "Not implemented");
}

mat4t quat_to_mat4t(quaternion q) {
    float norm = quat_norm(q);
    float s = norm > 0.0f ? 2.0f / norm : 0.0f;

    float xs = q.x * s;
    float xx = q.x * xs;
    float wx = q.w * xs;

    float ys = q.y * s;
    float yy = q.y * ys;
    float wy = q.w * ys;

    float zs = q.z * s;
    float zz = q.z * zs;
    float wz = q.w * zs;

    float xy = q.x * ys;
    float xz = q.x * zs;
    float yz = q.y * zs;

    mat4t res = {0};
    res.m[0][0] = 1.0 - (yy + zz);
    res.m[0][1] = xy - wz;
    res.m[0][2] = xz + wy;

    res.m[1][0] = xy + wz;
    res.m[1][1] = 1.0 - (xx + zz);
    res.m[1][2] = yz - wx;

    res.m[2][0] = xz - wy;
    res.m[2][1] = yz + wx;
    res.m[2][2] = 1.0 - (xx + yy);

    res.m[3][3] = 1;

    return res;
}

vec3 vec_rotate_by_quat(vec3 v, quaternion q) {
    quaternion i;
	i.x =  q.w * v.x + q.y * v.z - q.z * v.y;
	i.y =  q.w * v.y + q.z * v.x - q.x * v.z;
	i.z =  q.w * v.z + q.x * v.y - q.y * v.x;	
	i.w = -q.x * v.x - q.y * v.y - q.z * v.z;

	// r = q P*
	vec3 res;
	res.x = i.w * -q.x + q.w * i.x - i.y * q.z + i.z * q.y;
	res.y = i.w * -q.y + q.w * i.y - i.z * q.x + i.x * q.z;
	res.z = i.w * -q.z + q.w * i.z - i.x * q.y + i.y * q.x;

	return res;
}


#endif /* LINAL_QUAT */
