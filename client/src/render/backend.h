/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef RENDERER_BACKEND_H
#define RENDERER_BACKEND_H

#pragma once
#include "render/types.h"
#include "render/command.h"

struct Backend;


/* ========================================================================= */
/* Initialization / shutdown                                                 */
/* ========================================================================= */

extern struct Backend* backend_init(void);
extern void backend_shutdown(struct Backend*);


/* ========================================================================= */
/* Buffers                                                                    */
/* ========================================================================= */

extern Buffer backend_buffer_create(struct Backend*, const BufferDesc *desc);
extern void backend_buffer_destroy(struct Backend*, Buffer buffer);
extern void backend_buffer_upload(struct Backend*, 
    Buffer buffer,
    uint64_t offset,
    uint8_t *data,
    uint64_t size
);


/* ========================================================================= */
/* Textures                                                                   */
/* ========================================================================= */

extern Texture backend_texture_create(struct Backend*, const TextureDesc *desc);
extern void backend_texture_destroy(struct Backend*, Texture texture);
extern void backend_texture_upload(struct Backend*, 
    Texture texture,
    uint32_t mip_level,
    uint32_t layer,
    uint8_t *data,
    uint64_t size
);


/* ========================================================================= */
/* Texture views                                                              */
/* ========================================================================= */

extern TextureView backend_texture_view_create(struct Backend*, const TextureViewDesc *desc);
extern void backend_texture_view_destroy(struct Backend*, TextureView view);


/* ========================================================================= */
/* Samplers                                                                   */
/* ========================================================================= */

extern Sampler backend_sampler_create(struct Backend*, const SamplerDesc *desc);
extern void backend_sampler_destroy(struct Backend*, Sampler sampler);


/* ========================================================================= */
/* Shaders                                                                    */
/* ========================================================================= */

extern Shader backend_shader_create(struct Backend*, const ShaderDesc *desc);
extern void backend_shader_destroy(struct Backend*, Shader shader);


/* ========================================================================= */
/* Descriptor layouts                                                         */
/* ========================================================================= */

extern DescriptorLayout backend_descriptor_layout_create(struct Backend*, const DescriptorLayoutDesc *desc);
extern void backend_descriptor_layout_destroy(struct Backend*, DescriptorLayout layout);


/* ========================================================================= */
/* Descriptor sets                                                            */
/* ========================================================================= */

extern DescriptorSet backend_descriptor_set_create(struct Backend*, const DescriptorSetDesc *desc);
extern void backend_descriptor_set_destroy(struct Backend*, DescriptorSet set);


/* ========================================================================= */
/* Pipeline layouts                                                           */
/* ========================================================================= */

extern PipelineLayout backend_pipeline_layout_create(struct Backend*, const PipelineLayoutDesc *desc);
extern void backend_pipeline_layout_destroy(struct Backend*, PipelineLayout layout);


/* ========================================================================= */
/* Pipelines                                                                  */
/* ========================================================================= */

extern Pipeline backend_pipeline_create(struct Backend*, const PipelineDesc *desc);
extern void backend_pipeline_destroy(struct Backend*, Pipeline pipeline);


/* ========================================================================= */
/* Commands                                                                  */
/* ========================================================================= */

extern void backend_exectute_commands(struct Backend*, uint8_t* cmd_buffer, size_t size, size_t cap);

#endif
