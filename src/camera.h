#pragma once

#include <stdbool.h>

typedef enum
{
    CAMERA_TYPE_PERSPECTIVE,
    CAMERA_TYPE_ORTHO_2D,
    CAMERA_TYPE_ORTHO_3D,
}
camera_type_t;

typedef struct
{
    camera_type_t type;
    float matrix[4][4];
    float view[4][4];
    float proj[4][4];
    float inverse[4][4];
    float bounds[4][2];
    float x;
    float y;
    float z;
    float tx;
    float tz;
    float pitch;
    float yaw;
    float width;
    float height;
    float fov;
    float near;
    float far;
    float speed;
}
camera_t;

void camera_init(
    camera_t* camera,
    const camera_type_t type,
    const float y,
    const float width,
    const float height,
    const float pitch,
    const float yaw,
    const float speed);
void camera_update(
    camera_t* camera);
void camera_set_target(
    camera_t* camera,
    const float x,
    const float z);
void camera_set_viewport(
    camera_t* camera,
    const float width,
    const float height);
void camera_get_vector(
    const camera_t* camera,
    float* x,
    float* y,
    float* z);
void camera_project( 
    const camera_t* camera,
    float* x,
    float* z,
    const float y);
void camera_get_bounds(
    const camera_t* camera,
    float* x1,
    float* z1,
    float* x2,
    float* z2);