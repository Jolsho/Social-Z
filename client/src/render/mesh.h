/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef GLTF_LOADER_H
#define GLTF_LOADER_H

#pragma once
#include "render/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// DEFINED in render.h
struct Renderer;

typedef struct {
    Buffer vertex_buffer;
    Buffer index_buffer;

    uint32_t vertex_count;
    uint32_t index_count;

    uint32_t vertex_stride;
    uint32_t index_format;
} Mesh;

int mesh_load_gltf(struct Renderer* renderer, const char *path, Mesh *out_mesh);
void mesh_destroy(struct Renderer* renderer, Mesh *mesh);

#ifdef __cplusplus
}
#endif

#endif /* GLTF_LOADER_H */
