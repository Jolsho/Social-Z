/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef RENDERER_H
#define RENDERER_H

#pragma once
#include <stdint.h>

/* ========================================================================= */
/* Handles                                                                   */
/* ========================================================================= */

typedef uint32_t Buffer;
typedef uint32_t Texture;
typedef uint32_t TextureView;
typedef uint32_t Sampler;
typedef uint32_t Shader;
typedef uint32_t DescriptorLayout;
typedef uint32_t DescriptorSet;
typedef uint32_t PipelineLayout;
typedef uint32_t Pipeline;
typedef uint32_t RenderPass;

/* ========================================================================= */
/* Formats                                                                   */
/* ========================================================================= */

#define FORMAT_UNDEFINED            0
#define FORMAT_RGBA8_UNORM          1
#define FORMAT_BGRA8_UNORM          2
#define FORMAT_RGBA16_FLOAT         3
#define FORMAT_RGBA32_FLOAT         4
#define FORMAT_R32_FLOAT            5
#define FORMAT_R32_UINT             6
#define FORMAT_D24_UNORM_S8_UINT    7
#define FORMAT_D32_FLOAT            8


/* ========================================================================= */
/* Buffer                                                                    */
/* ========================================================================= */

#define BUFFER_USAGE_VERTEX       (1u << 0)
#define BUFFER_USAGE_INDEX        (1u << 1)
#define BUFFER_USAGE_UNIFORM      (1u << 2)
#define BUFFER_USAGE_STORAGE      (1u << 3)
#define BUFFER_USAGE_INDIRECT     (1u << 4)

typedef struct {
    uint64_t size;
    uint32_t usage;
} BufferDesc;


/* ========================================================================= */
/* Texture                                                                   */
/* ========================================================================= */

#define TEXTURE_1D          0
#define TEXTURE_2D          1
#define TEXTURE_3D          2

#define SAMPLE_1            1
#define SAMPLE_2            2
#define SAMPLE_4            4
#define SAMPLE_8            8

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t depth;

    uint32_t mip_levels;
    uint32_t layers;

    uint32_t type;
    uint32_t format;
    uint32_t samples;

    uint32_t usage;

    char    *path;
    uint8_t  path_len;
} TextureDesc;


/* ========================================================================= */
/* Texture view                                                              */
/* ========================================================================= */

typedef struct {
    Texture texture;

    uint32_t format;

    uint32_t base_mip;
    uint32_t mip_count;

    uint32_t base_layer;
    uint32_t layer_count;
} TextureViewDesc;


/* ========================================================================= */
/* Sampler                                                                   */
/* ========================================================================= */

#define FILTER_NEAREST     0
#define FILTER_LINEAR      1

#define ADDRESS_REPEAT             0
#define ADDRESS_MIRRORED_REPEAT    1
#define ADDRESS_CLAMP              2

typedef struct {
    uint32_t min_filter;
    uint32_t mag_filter;
    uint32_t mip_filter;

    uint32_t address_u;
    uint32_t address_v;
    uint32_t address_w;

    float mip_lod_bias;
    float min_lod;
    float max_lod;

    uint32_t anisotropy;
} SamplerDesc;


/* ========================================================================= */
/* Shader                                                                    */
/* ========================================================================= */

#define SHADER_VERTEX       (1u << 0)
#define SHADER_FRAGMENT     (1u << 1)
#define SHADER_COMPUTE      (1u << 2)

typedef struct {
    const char *path;
    uint32_t    stage;
} ShaderDesc;


/* ========================================================================= */
/* Descriptors                                                               */
/* ========================================================================= */

#define DESCRIPTOR_UNIFORM_BUFFER     0
#define DESCRIPTOR_STORAGE_BUFFER     1
#define DESCRIPTOR_TEXTURE            2
#define DESCRIPTOR_STORAGE_TEXTURE    3
#define DESCRIPTOR_SAMPLER            4

typedef struct {
    uint32_t binding;
    uint32_t type;
    uint32_t count;
    uint32_t stages;
} DescriptorBinding;

typedef struct {
    uint32_t binding_count;
    const DescriptorBinding *bindings;
} DescriptorLayoutDesc;


/* ========================================================================= */
/* Descriptor set                                                            */
/* ========================================================================= */

typedef struct {
    uint32_t binding;
    uint32_t type;

    union {
        Buffer buffer;
        TextureView texture;
        Sampler sampler;
    };
} DescriptorWrite;

typedef struct {
    DescriptorLayout layout;

    uint32_t write_count;
    const DescriptorWrite *writes;
} DescriptorSetDesc;


/* ========================================================================= */
/* Pipeline layout                                                            */
/* ========================================================================= */

typedef struct {
    uint32_t set_count;
    const DescriptorLayout *sets;
} PipelineLayoutDesc;


/* ========================================================================= */
/* Vertex input                                                              */
/* ========================================================================= */

#define VERTEX_FLOAT        0
#define VERTEX_FLOAT2       1
#define VERTEX_FLOAT3       2
#define VERTEX_FLOAT4       3

#define VERTEX_INT          4
#define VERTEX_INT2         5
#define VERTEX_INT3         6
#define VERTEX_INT4         7

#define VERTEX_UINT         8
#define VERTEX_UINT2        9
#define VERTEX_UINT3        10
#define VERTEX_UINT4        11

#define INDEX_UINT16 1
#define INDEX_UINT32 2

typedef struct {
    uint32_t location;
    uint32_t format;
    uint32_t offset;
    uint32_t binding;
} VertexAttribute;

typedef struct {
    uint32_t binding;
    uint32_t stride;

    uint32_t attribute_count;
    const VertexAttribute *attributes;
} VertexBufferLayout;

typedef struct {
    uint32_t binding_count;
    const VertexBufferLayout *bindings;
} VertexInputDesc;


/* ========================================================================= */
/* Pipeline                                                                   */
/* ========================================================================= */

#define TOPOLOGY_POINT             0
#define TOPOLOGY_LINE              1
#define TOPOLOGY_LINE_STRIP        2
#define TOPOLOGY_TRIANGLE          3
#define TOPOLOGY_TRIANGLE_STRIP    4

#define CULL_NONE                  0
#define CULL_FRONT                 1
#define CULL_BACK                  2
#define CULL_FRONT_AND_BACK        3

#define FRONT_CCW                  0
#define FRONT_CW                   1

#define COMPARE_NEVER              0
#define COMPARE_LESS               1
#define COMPARE_EQUAL              2
#define COMPARE_LESS_EQUAL         3
#define COMPARE_GREATER            4
#define COMPARE_NOT_EQUAL          5
#define COMPARE_GREATER_EQUAL      6
#define COMPARE_ALWAYS             7

typedef struct {
    uint32_t cull_mode;
    uint32_t front_face;
} RasterizationDesc;

typedef struct {
    uint32_t test;
    uint32_t write;
    uint32_t compare;
} DepthStencilDesc;


/* ========================================================================= */
/* Blending                                                                  */
/* ========================================================================= */

#define BLEND_ZERO                  0
#define BLEND_ONE                   1
#define BLEND_SRC_COLOR             2
#define BLEND_ONE_MINUS_SRC_COLOR   3
#define BLEND_SRC_ALPHA             4
#define BLEND_ONE_MINUS_SRC_ALPHA   5
#define BLEND_DST_COLOR             6
#define BLEND_ONE_MINUS_DST_COLOR   7
#define BLEND_DST_ALPHA             8
#define BLEND_ONE_MINUS_DST_ALPHA   9

typedef struct {
    uint32_t enable;

    uint32_t src_color;
    uint32_t dst_color;

    uint32_t src_alpha;
    uint32_t dst_alpha;
} BlendDesc;


/* ========================================================================= */
/* Pipeline                                                                   */
/* ========================================================================= */

typedef struct {
    Shader vertex_shader;
    Shader fragment_shader;

    PipelineLayout layout;

    VertexInputDesc vertex_input;

    uint32_t topology;

    RasterizationDesc rasterization;
    DepthStencilDesc depth_stencil;
    BlendDesc blend;

    uint32_t color_format;
    uint32_t depth_format;
    uint32_t samples;
} PipelineDesc;


/* ========================================================================= */
/* Render pass                                                               */
/* ========================================================================= */

#define LOAD_LOAD           0
#define LOAD_CLEAR          1
#define LOAD_DONT_CARE      2

#define STORE_STORE         0
#define STORE_DONT_CARE     1

typedef struct {
    uint32_t format;
    uint32_t samples;

    uint32_t load;
    uint32_t store;

    float clear[4];
} ColorAttachmentDesc;

typedef struct {
    uint32_t format;
    uint32_t samples;

    uint32_t load;
    uint32_t store;

    float clear_depth;
    uint32_t clear_stencil;
} DepthAttachmentDesc;

typedef struct {
    uint32_t color_count;
    const ColorAttachmentDesc *colors;

    const DepthAttachmentDesc *depth;
} RenderPassDesc;

#endif
