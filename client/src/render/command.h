/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef RENDERER_CMD_H
#define RENDERER_CMD_H

#pragma once
#include "render/types.h"
#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t *data;
    size_t size;
    size_t capacity;
} CommandBuffer;

/* ------------------------------------------------------------------------- */
/* Commands                                                                  */
/* ------------------------------------------------------------------------- */

#define CMD_BEGIN_RENDERPASS       0x01
#define CMD_END_RENDERPASS         0x02
#define CMD_SET_PIPELINE           0x03
#define CMD_SET_DESCRIPTOR_SET     0x04
#define CMD_SET_VERTEX_BUFFER      0x05
#define CMD_SET_INDEX_BUFFER       0x06
#define CMD_DRAW                   0x07
#define CMD_DRAW_INDEXED           0x08
#define CMD_DISPATCH               0x09

typedef struct {
    uint8_t opcode;
    uint8_t reserved;
    uint16_t size;
} CmdHeader;

/* ------------------------------------------------------------------------- */
/* Command payloads                                                          */
/* ------------------------------------------------------------------------- */

typedef struct {
    uint32_t color_count;
    uint32_t has_depth;
} CmdBeginRenderPass;

typedef struct {
    Pipeline pipeline;
} CmdSetPipeline;

typedef struct {
    uint32_t slot;
    DescriptorSet set;
} CmdSetDescriptorSet;

typedef struct {
    uint32_t binding;
    Buffer buffer;
    uint64_t offset;
} CmdSetVertexBuffer;

typedef struct {
    Buffer buffer;
    uint64_t offset;
} CmdSetIndexBuffer;

typedef struct {
    uint32_t vertex_count;
    uint32_t instance_count;
    uint32_t first_vertex;
    uint32_t first_instance;
} CmdDraw;

typedef struct {
    uint32_t index_count;
    uint32_t instance_count;
    uint32_t first_index;
    int32_t  vertex_offset;
    uint32_t first_instance;
} CmdDrawIndexed;

typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t z;
} CmdDispatch;

/* ------------------------------------------------------------------------- */
/* Command buffer                                                            */
/* ------------------------------------------------------------------------- */

void cmd_init(
    CommandBuffer *cmd, 
    void *memory, 
    size_t capacity
);

void cmd_reset(
    CommandBuffer *cmd
);

void *cmd_push(
    CommandBuffer *cmd,
    uint8_t opcode,
    size_t payload_size
);

void cmd_begin_renderpass(
    CommandBuffer *cmd,
    const RenderPassDesc *desc
);
void cmd_end_renderpass(
    CommandBuffer *cmd
);

void cmd_set_pipeline(
    CommandBuffer *cmd,
    Pipeline pipeline
);

void cmd_set_descriptor_set(
    CommandBuffer *cmd,
    uint32_t slot,
    DescriptorSet set
);

void cmd_set_vertex_buffer(
    CommandBuffer *cmd,
    uint32_t binding,
    Buffer buffer,
    uint64_t offset
);

void cmd_set_index_buffer(
    CommandBuffer *cmd,
    Buffer buffer,
    uint64_t offset
);

void cmd_draw(
    CommandBuffer *cmd,
    uint32_t vertex_count,
    uint32_t instance_count,
    uint32_t first_vertex,
    uint32_t first_instance
);

void cmd_draw_indexed(
    CommandBuffer *cmd,
    uint32_t index_count,
    uint32_t instance_count,
    uint32_t first_index,
    int32_t vertex_offset,
    uint32_t first_instance
);

void cmd_dispatch(
    CommandBuffer *cmd,
    uint32_t x,
    uint32_t y,
    uint32_t z
);

#endif
