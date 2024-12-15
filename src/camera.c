#include <math.h>
#include <stdbool.h>
#include "camera.h"
#include "config.h"
#include "helpers.h"

enum
{
    TOP_LEFT,
    BOTTOM_LEFT,
    TOP_RIGHT,
    BOTTOM_RIGHT,
};

static void multiply(
    float matrix[4][4],
    const float a[4][4],
    const float b[4][4])
{
    float c[4][4];
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            c[i][j] = 0.0f;
            for (int k = 0; k < 4; k++)
            {
                c[i][j] += a[k][j] * b[i][k];
            }
        }
    }
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            matrix[i][j] = c[i][j];
        }
    }
}

static void multiply2(
    float vector[4],
    const float a[4][4],
    const float b[4])
{
    float c[4];
    for (int i = 0; i < 4; i++)
    {
        c[i] = 0.0f;
        for (int j = 0; j < 4; j++)
        {
            c[i] += a[j][i] * b[j];
        }
    }
    for (int i = 0; i < 4; i++)
    {
        vector[i] = c[i];
    }
}

static void translate(
    float matrix[4][4],
    const float x,
    const float y,
    const float z)
{
    matrix[0][0] = 1.0f;
    matrix[0][1] = 0.0f;
    matrix[0][2] = 0.0f;
    matrix[0][3] = 0.0f;
    matrix[1][0] = 0.0f;
    matrix[1][1] = 1.0f;
    matrix[1][2] = 0.0f;
    matrix[1][3] = 0.0f;
    matrix[2][0] = 0.0f;
    matrix[2][1] = 0.0f;
    matrix[2][2] = 1.0f;
    matrix[2][3] = 0.0f;
    matrix[3][0] = x;
    matrix[3][1] = y;
    matrix[3][2] = z;
    matrix[3][3] = 1.0f;
}

static void rotate(
    float matrix[4][4],
    const float x,
    const float y,
    const float z,
    const float angle)
{
    const float s = sinf(angle);
    const float c = cosf(angle);
    const float i = 1.0f - c;
    matrix[0][0] = i * x * x + c;
    matrix[0][1] = i * x * y - z * s;
    matrix[0][2] = i * z * x + y * s;
    matrix[0][3] = 0.0f;
    matrix[1][0] = i * x * y + z * s;
    matrix[1][1] = i * y * y + c;
    matrix[1][2] = i * y * z - x * s;
    matrix[1][3] = 0.0f;
    matrix[2][0] = i * z * x - y * s;
    matrix[2][1] = i * y * z + x * s;
    matrix[2][2] = i * z * z + c;
    matrix[2][3] = 0.0f;
    matrix[3][0] = 0.0f;
    matrix[3][1] = 0.0f;
    matrix[3][2] = 0.0f;
    matrix[3][3] = 1.0f;
}

static void perspective(
    float matrix[4][4],
    const float fov,
    const float aspect,
    const float near,
    const float far)
{
    const float f = 1.0f / tanf(fov / 2.0f);
    matrix[0][0] = f / aspect;
    matrix[0][1] = 0.0f;
    matrix[0][2] = 0.0f;
    matrix[0][3] = 0.0f;
    matrix[1][0] = 0.0f;
    matrix[1][1] = f;
    matrix[1][2] = 0.0f;
    matrix[1][3] = 0.0f;
    matrix[2][0] = 0.0f;
    matrix[2][1] = 0.0f;
    matrix[2][2] = -(far + near) / (far - near);
    matrix[2][3] = -1.0f;
    matrix[3][0] = 0.0f;
    matrix[3][1] = 0.0f;
    matrix[3][2] = -(2.0f * far * near) / (far - near);
    matrix[3][3] = 0.0f;
}

static void ortho(
    float matrix[4][4],
    const float left,
    const float right,
    const float bottom,
    const float top,
    const float near,
    const float far)
{
    matrix[0][0] = 2.0f / (right - left);
    matrix[0][1] = 0.0f;
    matrix[0][2] = 0.0f;
    matrix[0][3] = 0.0f;
    matrix[1][0] = 0.0f;
    matrix[1][1] = 2.0f / (top - bottom);
    matrix[1][2] = 0.0f;
    matrix[1][3] = 0.0f;
    matrix[2][0] = 0.0f;
    matrix[2][1] = 0.0f;
    matrix[2][2] = -1.0f / (far - near);
    matrix[2][3] = 0.0f;
    matrix[3][0] = -(right + left) / (right - left);
    matrix[3][1] = -(top + bottom) / (top - bottom);
    matrix[3][2] = -near / (far - near);
    matrix[3][3] = 1.0f;
}

static void inverse(
    float matrix[4][4],
    const float z[4][4])
{
    float t[6];
    const float
        a = z[0][0],
        b = z[0][1],
        c = z[0][2],
        d = z[0][3],
        e = z[1][0],
        f = z[1][1],
        g = z[1][2],
        h = z[1][3],
        i = z[2][0],
        j = z[2][1],
        k = z[2][2],
        l = z[2][3],
        m = z[3][0],
        n = z[3][1],
        o = z[3][2],
        p = z[3][3];
    t[0] = k * p - o * l;
    t[1] = j * p - n * l;
    t[2] = j * o - n * k;
    t[3] = i * p - m * l;
    t[4] = i * o - m * k;
    t[5] = i * n - m * j;
    matrix[0][0] = f * t[0] - g * t[1] + h * t[2];
    matrix[1][0] = -(e * t[0] - g * t[3] + h * t[4]);
    matrix[2][0] = e * t[1] - f * t[3] + h * t[5];
    matrix[3][0] = -(e * t[2] - f * t[4] + g * t[5]);
    matrix[0][1] = -(b * t[0] - c * t[1] + d * t[2]);
    matrix[1][1] = a * t[0] - c * t[3] + d * t[4];
    matrix[2][1] = -(a * t[1] - b * t[3] + d * t[5]);
    matrix[3][1] = a * t[2] - b * t[4] + c * t[5];
    t[0] = g * p - o * h;
    t[1] = f * p - n * h;
    t[2] = f * o - n * g;
    t[3] = e * p - m * h;
    t[4] = e * o - m * g;
    t[5] = e * n - m * f;
    matrix[0][2] = b * t[0] - c * t[1] + d * t[2];
    matrix[1][2] = -(a * t[0] - c * t[3] + d * t[4]);
    matrix[2][2] = a * t[1] - b * t[3] + d * t[5];
    matrix[3][2] = -(a * t[2] - b * t[4] + c * t[5]);
    t[0] = g * l - k * h;
    t[1] = f * l - j * h;
    t[2] = f * k - j * g;
    t[3] = e * l - i * h;
    t[4] = e * k - i * g;
    t[5] = e * j - i * f;
    matrix[0][3] = -(b * t[0] - c * t[1] + d * t[2]);
    matrix[1][3] = a * t[0] - c * t[3] + d * t[4];
    matrix[2][3] = -(a * t[1] - b * t[3] + d * t[5]);
    matrix[3][3] = a * t[2] - b * t[4] + c * t[5];
    const float determinant =
        a * matrix[0][0] +
        b * matrix[1][0] +
        c * matrix[2][0] +
        d * matrix[3][0];
    for (int s = 0; s < 4; s++)
    {
        for (int r = 0; r < 4; r++)
        {
            matrix[s][r] /= determinant;
        }
    }
}

void camera_init(
    camera_t* camera,
    const camera_type_t type,
    const float y,
    const float width,
    const float height,
    const float pitch,
    const float yaw,
    const float speed)
{
    assert(camera);
    camera->type = type;
    camera->x = 0.0f;
    camera->y = y;
    camera->z = 0.0f;
    camera->tx = 0.0f;
    camera->tz = 0.0f;
    camera->pitch = pitch;
    camera->yaw = yaw;
    camera->width = width;
    camera->height = height;
    camera->fov = rad(60.0f);
    camera->near = 1.0f;
    camera->far = 1000.0f;
    camera->speed = speed;
}

void camera_update(
    camera_t* camera)
{
    assert(camera);
    if (camera->type == CAMERA_TYPE_ORTHO_2D)
    {
        ortho(
            camera->matrix,
            0.0f,
            camera->width,
            0.0f,
            camera->height,
            -1.0f,
            1.0f);
        return;
    }
    camera->x = lerp(camera->x, camera->tx, camera->speed);
    camera->z = lerp(camera->z, camera->tz, camera->speed);
    const float s = sinf(camera->yaw);
    const float c = cosf(camera->yaw);
    translate(camera->view, -camera->x, -camera->y, -camera->z);
    rotate(camera->proj, c, 0.0f, s, camera->pitch);
    multiply(camera->view, camera->proj, camera->view);
    rotate(camera->proj, 0.0f, 1.0f, 0.0f, -camera->yaw);
    multiply(camera->view, camera->proj, camera->view);
    if (camera->type == CAMERA_TYPE_PERSPECTIVE)
    {
        perspective(
            camera->proj,
            camera->fov,
            camera->width / camera->height,
            camera->near,
            camera->far);
    }
    else
    {
        ortho(
            camera->proj,
            -camera->width / 2.0f,
            camera->width / 2.0f,
            -camera->height / 2.0f,
            camera->height / 2.0f,
            -camera->far,
            camera->far);
    }
    multiply(camera->matrix, camera->proj, camera->view);
    if (camera->type != CAMERA_TYPE_PERSPECTIVE)
    {
        return;
    }
    inverse(camera->inverse, camera->matrix);
    camera->bounds[TOP_LEFT][0] = -1.0f;
    camera->bounds[TOP_LEFT][1] = 1.0f;
    camera->bounds[BOTTOM_LEFT][0] = -1.0f;
    camera->bounds[BOTTOM_LEFT][1] = -1.0f;
    camera->bounds[TOP_RIGHT][0] = 1.0f;
    camera->bounds[TOP_RIGHT][1] = 1.0f;
    camera->bounds[BOTTOM_RIGHT][0] = 1.0f;
    camera->bounds[BOTTOM_RIGHT][1] = -1.0f;
    const float y[4] =
    {
        [TOP_LEFT] = 0.0f,
        [BOTTOM_LEFT] = MODEL_MAX_HEIGHT,
        [TOP_RIGHT] = 0.0f,
        [BOTTOM_RIGHT] = MODEL_MAX_HEIGHT,
    };
    for (int i = 0; i < 4; i++)
    {
        camera_project(
            camera,
            &camera->bounds[i][0],
            &camera->bounds[i][1],
            y[i]);
    }
}

void camera_set_target(
    camera_t* camera,
    const float x,
    const float z)
{
    assert(camera);
    if (camera->type == CAMERA_TYPE_PERSPECTIVE)
    {
        const float h = camera->y / tanf(camera->pitch);
        const float a = h * sinf(camera->yaw);
        const float b = h * cosf(camera->yaw);
        camera->tx = x + a;
        camera->tz = z - b;
    }
    else
    {
        camera->tx = x;
        camera->tz = z;
    }
}

void camera_set_viewport(
    camera_t* camera,
    const float width,
    const float height)
{
    assert(camera);
    assert(width > 0.0f);
    assert(height > 0.0f);
    camera->width = width;
    camera->height = height;
}

void camera_get_vector(
    const camera_t* camera,
    float* x,
    float* y,
    float* z)
{
    assert(camera);
    assert(x);
    assert(y);
    assert(z);
    const float c = cosf(camera->pitch);
    *x = cosf(camera->yaw - rad(90.0f)) * c;
    *y = sinf(camera->pitch);
    *z = sinf(camera->yaw - rad(90.0f)) * c;
}

void camera_project( 
    const camera_t* camera,
    float* x,
    float* z,
    const float y)
{
    assert(camera);
    assert(camera->type == CAMERA_TYPE_PERSPECTIVE);
    assert(x);
    assert(z);
    float near[4] = { *x, *z, 0.0f, 1.0f };
    float far[4] = { *x, *z, 1.0f, 1.0f };
    float direction[3];
    multiply2(near, camera->inverse, near);
    multiply2(far, camera->inverse, far);
    for (int i = 0; i < 3; ++i)
    {
        near[i] /= near[3];
        far[i] /= far[3];
        direction[i] = far[i] - near[i];
    }
    const float t = (y - near[1]) / direction[1];
    *x = near[0] + t * direction[0];
    *z = near[2] + t * direction[2];
}

void camera_get_bounds(
    const camera_t* camera,
    float* x1,
    float* z1,
    float* x2,
    float* z2)
{
    assert(camera);
    assert(camera->type == CAMERA_TYPE_PERSPECTIVE);
    assert(x1);
    assert(z1);
    assert(x2);
    assert(z2);
    *x1 = camera->bounds[TOP_LEFT][0];
    *z1 = camera->bounds[TOP_LEFT][1];
    *x2 = camera->bounds[TOP_RIGHT][0];
    *z2 = camera->bounds[TOP_RIGHT][1];
    *x1 = min(*x1, camera->bounds[BOTTOM_LEFT][0]);
    *z1 = min(*z1, camera->bounds[BOTTOM_LEFT][1]);
    *x2 = max(*x2, camera->bounds[BOTTOM_RIGHT][0]);
    *z2 = max(*z2, camera->bounds[BOTTOM_RIGHT][1]);
}