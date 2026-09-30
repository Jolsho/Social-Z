/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */


#pragma once
#include "render/mesh.h"

enum MeshID : uint8_t {
    BOX_1,
    BOX_2,

    MESH_COUNT
};

typedef struct Renderer {
    struct Backend* backend;

    Mesh    meshes[MESH_COUNT];

} Renderer;

int renderer_create_mesh(Renderer* renderer, uint8_t mesh_id);

/* 
 *  create box entity?
 *
 *      Create Textures
 *          Views, Samplers
 *          
 *      Create Descriptor Layout
 *          Using Descriptor Bindings
 *
 *      Create Descriptor Set
 *          Using Descriptor Writes
 *
 *      Create PipelineLayout
 *          Using Descriptor Layouts
*/


