/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "render/mesh.h"
#include "render/backend.h"
#include "render/renderer.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"


typedef struct {
    float position[3];
    float normal[3];
    float uv[2];
} Vertex;


// LINEAR SEARCH THROUGH ATTRIBUTES
static cgltf_accessor * find_attribute(
    struct Renderer* renderer,
    const cgltf_primitive *primitive,
    cgltf_attribute_type type,
    cgltf_int index
)
{
    for (cgltf_size i = 0; i < primitive->attributes_count; ++i) {
        const cgltf_attribute *attribute =
            &primitive->attributes[i];

        if (attribute->type == type &&
            attribute->index == index)
        {
            return attribute->data;
        }
    }
    return NULL;
}


static int build_mesh_from_primitive(
    struct Renderer* renderer,
    const cgltf_primitive *primitive,
    Mesh *out_mesh
)
{
    cgltf_accessor *position =
        find_attribute(
            renderer,
            primitive,
            cgltf_attribute_type_position,
            0
        );

    if (!position)
        return 0;

    cgltf_accessor *normal =
        find_attribute(
            renderer,
            primitive,
            cgltf_attribute_type_normal,
            0
        );

    cgltf_accessor *uv =
        find_attribute(
            renderer,
            primitive,
            cgltf_attribute_type_texcoord,
            0
        );

    uint32_t vertex_count = (uint32_t)position->count;

    Vertex *vertices = calloc(vertex_count, sizeof(Vertex));

    if (!vertices)
        return 0;

    /*
     * Extract vertex attributes.
     */
    for (uint32_t i = 0; i < vertex_count; ++i) {

        if (!cgltf_accessor_read_float(
                position,
                i,
                vertices[i].position,
                3))
        {
            free(vertices);
            return 0;
        }

        if (normal) {
            if (!cgltf_accessor_read_float(
                    normal,
                    i,
                    vertices[i].normal,
                    3))
            {
                free(vertices);
                return 0;
            }
        }

        if (uv) {
            if (!cgltf_accessor_read_float(
                    uv,
                    i,
                    vertices[i].uv,
                    2))
            {
                free(vertices);
                return 0;
            }
        }
    }


    /*
     * Extract indices.
     *
     * For now, normalize everything to uint32_t.
     */
    uint32_t index_count = 0;
    uint32_t *indices = NULL;

    if (primitive->indices) {

        index_count = (uint32_t)primitive->indices->count;

        indices = malloc(sizeof(uint32_t) * index_count);

        if (!indices) {
            free(vertices);
            return 0;
        }

        for (uint32_t i = 0; i < index_count; ++i) {
            indices[i] = (uint32_t)cgltf_accessor_read_index(primitive->indices, i);
        }
    }


    /*
     * Create vertex buffer.
     */
    BufferDesc vertex_desc = {
        .size = sizeof(Vertex) * vertex_count,
        .usage = BUFFER_USAGE_VERTEX
    };

    Buffer vertex_buffer = backend_buffer_create(renderer->backend, &vertex_desc);

    if (!vertex_buffer) {
        free(vertices);
        free(indices);
        return 0;
    }

    backend_buffer_upload(
        renderer->backend,
        vertex_buffer,
        0,
        (uint8_t*) vertices,
        sizeof(Vertex) * vertex_count
    );


    /*
     * Create index buffer.
     */
    Buffer index_buffer = 0;

    if (index_count > 0) {

        BufferDesc index_desc = {
            .size = sizeof(uint32_t) * index_count,
            .usage = BUFFER_USAGE_INDEX
        };

        index_buffer = backend_buffer_create(renderer->backend, &index_desc);

        if (!index_buffer) {
            backend_buffer_destroy(renderer->backend, vertex_buffer);

            free(vertices);
            free(indices);

            return 0;
        }

        backend_buffer_upload(
            renderer->backend,
            index_buffer,
            0,
            (uint8_t*)indices,
            sizeof(uint32_t) * index_count
        );
    }


    free(vertices);
    free(indices);


    *out_mesh = (Mesh) {
        .vertex_buffer = vertex_buffer,
        .index_buffer = index_buffer,

        .vertex_count = vertex_count,
        .index_count = index_count,

        .vertex_stride = sizeof(Vertex),
        .index_format = INDEX_UINT32
    };

    return 1;
}


int mesh_load_gltf(struct Renderer* renderer, const char *path, Mesh *out_mesh) 
{
    if (!path || !out_mesh)
        return 0;

    *out_mesh = (Mesh) {0};

    cgltf_options options = {0};
    cgltf_data *data = NULL;


    /*
     * Parse the glTF/GLB file.
     */
    cgltf_result result =
        cgltf_parse_file(
            &options,
            path,
            &data
        );

    if (result != cgltf_result_success)
        return 0;


    /*
     * Load external .bin files referenced
     * by the glTF.
     *
     * For GLB this also resolves the embedded
     * binary buffer.
     */
    result =
        cgltf_load_buffers(
            &options,
            data,
            path
        );

    if (result != cgltf_result_success) {
        cgltf_free(data);
        return 0;
    }


    /*
     * Validate the parsed document.
     */
    result = cgltf_validate(data);

    if (result != cgltf_result_success) {
        cgltf_free(data);
        return 0;
    }


    /*
     * First version:
     * use the first mesh and first primitive.
     */
    if (data->meshes_count == 0) {
        cgltf_free(data);
        return 0;
    }

    cgltf_mesh *mesh =
        &data->meshes[0];

    if (mesh->primitives_count == 0) {
        cgltf_free(data);
        return 0;
    }

    int success =
        build_mesh_from_primitive(
            renderer,
            &mesh->primitives[0],
            out_mesh
        );

    cgltf_free(data);

    return success;
}


void mesh_destroy(struct Renderer* renderer, Mesh *mesh)
{
    if (!mesh)
        return;

    if (mesh->vertex_buffer)
        backend_buffer_destroy(renderer->backend, mesh->vertex_buffer);

    if (mesh->index_buffer)
        backend_buffer_destroy(renderer->backend, mesh->index_buffer);

    *mesh = (Mesh) {0};
}
