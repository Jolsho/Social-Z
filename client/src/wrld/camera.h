/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "math/ray.h"
#include "math/vec3.h"

typedef struct {
    Vec3 position;

    // Radians
    float yaw;
    float pitch;

    float fov_y;
    float aspect;

    float move_speed;
    float mouse_sensitivity;

    Vec3 forward;
    Vec3 right;
    Vec3 up;
} Camera;


/* ---------- Camera ---------- */

static inline void camera_update_basis(Camera *camera)
{
    float cp = cosf(camera->pitch);
    float sp = sinf(camera->pitch);

    float cy = cosf(camera->yaw);
    float sy = sinf(camera->yaw);

    /*
     * Coordinate system:
     *
     *       +Y
     *        |
     *        |
     *        +---- +X
     *
     * Camera looks toward -Z when yaw = pitch = 0.
     */

    camera->forward = (Vec3){
        sy * cp,
        sp,
        -cy * cp
    };

    camera->forward = vec3_normalize(camera->forward);

    /*
     * World up.
     */
    Vec3 world_up = {0.0f, 1.0f, 0.0f};

    camera->right =
        vec3_normalize(
            vec3_cross(camera->forward, world_up)
        );

    camera->up =
        vec3_cross(camera->right, camera->forward);
}


static inline void camera_init(
    Camera *camera,
    Vec3 position,
    float fov_y,
    float aspect
)
{
    *camera = (Camera){
        .position = position,

        .yaw = 0.0f,
        .pitch = 0.0f,

        .fov_y = fov_y,
        .aspect = aspect,

        .move_speed = 5.0f,
        .mouse_sensitivity = 0.0025f
    };

    camera_update_basis(camera);
}


/* ---------- Mouse ---------- */

static inline void camera_mouse(
    Camera *camera,
    float dx,
    float dy
)
{
    camera->yaw   += dx * camera->mouse_sensitivity;
    camera->pitch -= dy * camera->mouse_sensitivity;

    /*
     * Prevent looking past straight up/down.
     */
    const float limit = 1.5707963f - 0.001f;

    if (camera->pitch > limit)
        camera->pitch = limit;

    if (camera->pitch < -limit)
        camera->pitch = -limit;

    camera_update_basis(camera);
}


/* ---------- Movement ---------- */

static inline void camera_move_forward(
    Camera *camera,
    float dt
)
{
    camera->position =
        vec3_add(
            camera->position,
            vec3_mul(camera->forward,
                     camera->move_speed * dt)
        );
}

static inline void camera_move_backward(
    Camera *camera,
    float dt
)
{
    camera->position =
        vec3_sub(
            camera->position,
            vec3_mul(camera->forward,
                     camera->move_speed * dt)
        );
}

static inline void camera_move_right(
    Camera *camera,
    float dt
)
{
    camera->position =
        vec3_add(
            camera->position,
            vec3_mul(camera->right,
                     camera->move_speed * dt)
        );
}

static inline void camera_move_left(
    Camera *camera,
    float dt
)
{
    camera->position =
        vec3_sub(
            camera->position,
            vec3_mul(camera->right,
                     camera->move_speed * dt)
        );
}

static inline void camera_move_up(
    Camera *camera,
    float dt
)
{
    camera->position.v[1] += camera->move_speed * dt;
}

static inline void camera_move_down(
    Camera *camera,
    float dt
)
{
    camera->position.v[1] -= camera->move_speed * dt;
}


/* ---------- Ray generation ---------- */

static inline Ray camera_ray(
    const Camera *camera,
    float screen_x,
    float screen_y
)
{
    /*
     * screen_x / screen_y are normalized:
     *
     * x: -1 = left,  +1 = right
     * y: -1 = bottom, +1 = top
     */

    float tan_half_fov = tanf(camera->fov_y * 0.5f);

    float x = screen_x *
              camera->aspect *
              tan_half_fov;

    float y = screen_y *
              tan_half_fov;

    Vec3 direction =
        vec3_add(
            camera->forward,
            vec3_add(
                vec3_mul(camera->right, x),
                vec3_mul(camera->up, y)
            )
        );

    direction = vec3_normalize(direction);

    return (Ray){
        .origin = camera->position,
        .direction = direction
    };
}
